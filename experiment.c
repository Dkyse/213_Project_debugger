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
= To run the program, input "experiment by_sys_call"=
= or "experiment by_instruction". It will run the   =
= program step by step, stopping at each system     =
= calls or instruction. Hit enter to continue       =
=====================================================*/


int main(int argc, char** argv)  {


  if(argc != 3) {
    fprintf(stderr, "Usage: %s <program path> <by_sys_call / by_instruction>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char* command = argv[2];
  char* user_program = argv[1];


  pid_t child;

  if (strcmp(command, "by_sys_call") == 0)  {


    long orig_eax;
    child = fork();
    if(child == 0) {
      ptrace(PTRACE_TRACEME, 0, NULL, NULL);


      /* TODO: add command line arguments */
      execl(user_program, user_program, NULL);
    } else {


      struct user_regs_struct regs;
      while (wait(NULL))  {
        if(ptrace(PTRACE_GETREGS, child, NULL, &regs)) {
          perror("ptrace GETREGS failed");
          exit(2);
        }
        printf("The child made a "
        "system call %llu\n", regs.orig_rax);
        getchar();
        ptrace(PTRACE_SYSCALL, child, NULL, NULL);
      }
    }
  } else if (strcmp(command, "by_instruction") == 0)  {
    long orig_eax;
    child = fork();
    if(child == 0) {
      ptrace(PTRACE_TRACEME, 0, NULL, NULL);

      /* TODO: add command line arguments */
      execl(user_program, user_program, NULL);

    }  else {


      struct user_regs_struct regs;
      while (wait(NULL) != -1)  {
        if(ptrace(PTRACE_GETREGS, child, NULL, &regs)) {
          perror("ptrace GETREGS failed");
          exit(2);
        }
        printf("instruction: %p\n", (void*)regs.rip); /* instruction pointer */
        getchar();
        ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);

      }
    }
  } else {
    fprintf(stderr, "Invalid input. Input options: by_sys_call / by_instruction\n");
    exit(EXIT_FAILURE);
  }
  return 0;
}
