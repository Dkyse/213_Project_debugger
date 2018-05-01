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
#include <fcntl.h>
#include "elf++.hh"
#include "dwarf++.hh"

using dwarf::compilation_unit;

/*=====================================================
= This is a toy program.                            =
= I made this to experiment with ptrace.            =
= To run the program, input "experiment by_sys_call"=
= or "experiment by_instruction". It will run the   =
= program step by step, stopping at each system     =
= calls or instruction. Hit enter to continue       =
=====================================================*/


/**
* [print_line_number description]
* @param cus [description]
* @param ip  [description]
*/
void print_line_info(const std::vector<compilation_unit> cus, void* ip)  {
  dwarf::line_table lt;
  for (auto cu : cus)  {
    lt = cu.get_line_table();
    auto entry = lt.find_address((unsigned long)ip);
    if (entry != lt.end())  {
      printf("File path: %s\n", entry->file->path.c_str());
      printf("Called from line %u\n\n", entry->line);
    }
  }
}


int main(int argc, char** argv)  {

  /* check and store arguments */

  if(argc < 2) {
    fprintf(stderr, "Usage: %s <by_sys_call / by_instruction> <program path> <program command inputs>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char* command = argv[1];
  char* user_program = argv[2];

  char* inputs[argc - 2];
  inputs[0] = argv[2];
  for(int i = 3; i < argc; i++){
    inputs[i-2] = argv[i];
  }
  inputs[argc-2] = NULL;

  pid_t child;

  long orig_eax;

  /* Using libelfin to trace line numbers. */
  int fd = open(inputs[0], 'r');
  if (fd < 0) {
    fprintf(stderr, "%s: %s\n", inputs[0], strerror(errno));
    return 1;
  }

  elf::elf elf(elf::create_mmap_loader(fd));
  dwarf::dwarf dwarf(dwarf::elf::create_loader(elf));

  const std::vector<compilation_unit> compilation_units = dwarf.compilation_units();
  /* end of libelfin preparations */

  child = fork();
  if(child == 0) {

    /* we are in the child program. Run the debuggee */
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    //added command line arguments
    execv(inputs[0], inputs);

  } else {

    /* we are in the parent program. Run the debugger */

    /* A struct to store debuggee status */
    struct user_regs_struct regs;

    while (wait(NULL))  {

      if(ptrace(PTRACE_GETREGS, child, NULL, &regs)) {
        perror("ptrace GETREGS failed");
        exit(2);
      }

      /* parse user instructions */
      if (strcmp(command, "by_sys_call") == 0)  {

        printf("The child made a "
        "system call %llu\n", regs.orig_rax);
        getchar();

      } else if (strcmp(command, "by_instruction") == 0)  {

        printf("instruction: %p\n", (void*)regs.rip); /* instruction pointer */
        print_line_info(compilation_units, (void*)regs.rip);
        getchar();

      }  else  {

        fprintf(stderr, "Invalid input. Input options: by_sys_call / by_instruction\n");
        exit(EXIT_FAILURE);

      }

      ptrace(PTRACE_SYSCALL, child, NULL, NULL);
    }
  }

  return 0;
}
