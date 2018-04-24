#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/user.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*=====================================================
= This is a toy program.                            =
= I made this to experiment with ptrace.            =
=====================================================*/


int main()
{   pid_t child;
  long orig_eax;
  child = fork();
  if(child == 0) {
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    execl("/bin/ls", "ls", NULL);
  }
  else {


    struct user_regs_struct regs;
    while (wait(NULL))  {
      if(ptrace(PTRACE_GETREGS, child, NULL, &regs)) {
        perror("ptrace GETREGS failed");
        exit(2);
      }
      printf("The child made a "
      "system call %llu\n", regs.orig_rax);
      ptrace(PTRACE_CONT, child, NULL, NULL);
    }
  }
  return 0;
}
