/**
* Sources:
* https://stackoverflow.com/questions/36523584/how-to-see-memory-layout-of-my-program-in-c-during-run-time/36524010
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
#include <link.h>

#include <vector>
#include <string>

#include "elf++.hh"
#include "dwarf++.hh"

using dwarf::compilation_unit;
using std::vector;
using std::string;

/*=====================================================
= This is a toy program.                            =
= I made this to experiment with ptrace.            =
= To run the program, input "experiment by_sys_call"=
= or "experiment by_instruction". It will run the   =
= program step by step, stopping at each system     =
= calls or instruction. Hit enter to continue       =
=====================================================*/

typedef struct shared_obj {
  unsigned long addr_start; // Start address of object
  unsigned long addr_end;  // End address of object
  string name; // Object name
} shared_obj_t;

/**
* [print_line_number description]
* @param cus [description]
* @param ip  [description]
*/
void print_line_info(const vector<compilation_unit> cus, void* ip)  {
  bool found = false;
  dwarf::line_table lt;
  for (auto cu : cus)  {
    lt = cu.get_line_table();
    auto entry = lt.find_address((unsigned long)ip);
    if (entry != lt.end())  {
      printf("File path: %s\n", entry->file->path.c_str());
      printf("Called from line %u\n\n", entry->line);
      found = true;
      break;
    }
  }
  if (!found) {
    // printf("Instruction %p not found", ip);
  }
}



// static int callback(struct dl_phdr_info *info, size_t size, void *data) {
//   int j;
//   // printf("name=%s (%d segments)\n", info->dlpi_name,
//   // info->dlpi_phnum);
//
//   shared_obj_t obj;
//   obj.name = info->dlpi_name;
//   obj.addr = info->dlpi_addr;
//
//   for (j = 0; j < info->dlpi_phnum; j++)
//   printf("\t\t header %2d: address=%10p\n", j,
//   (void *) (info->dlpi_addr + info->dlpi_phdr[j].p_vaddr));
//   return 0;
// }
//
//


enum COMMAND { SYS, INST };

/**
* [populate_maps description]
* Source:
* https://stackoverflow.com/questions/36523584/how-to-see-memory-layout-of-my-program-in-c-during-run-time/36524010
*/
int populate_maps(pid_t child, vector<shared_obj_t> &objects) {
  char* line = NULL;
  size_t size = 0;
  char maps_path[128];
  snprintf(maps_path, 128, "/proc/%d/maps", child);
  FILE* maps = fopen(maps_path, "r");
  if (maps == NULL) {
    fprintf(stderr, "Failed to open %s\n", maps_path);
    exit(EXIT_FAILURE);
  }

  // Parse maps file
  while (getline(&line, &size, maps) > 0) {

    shared_obj_t obj;

    // Temporary variables to store parsed data
    char           perms[8];
    unsigned int   devmajor, devminor;
    unsigned long  addr_start, addr_end, offset, inode;
    int            name_start = 0;
    int            name_end = 0;

    // Parse line
    if (sscanf(line, "%lx-%lx %7s %lx %u:%u %lu %n%*[^\n]%n",
    &addr_start, &addr_end, perms, &offset,
    &devmajor, &devminor, &inode,
    &name_start, &name_end) < 7) {
      fclose(maps);
      free(line);
      return -1;
    }

    // Check for valid name
    if (name_end > name_start)  {
      string name (line + name_start, name_end - name_start);
      obj.name = name;
    }

    obj.addr_start = addr_start;
    obj.addr_end = addr_end;

    // Add object to vector
    objects.push_back(obj);
  } /* end of while */

  // Wrap up
  fclose(maps);
  free(line);
  return 0;
}

int main(int argc, char** argv)  {

  /* check and store arguments */

  if(argc < 3) {
    fprintf(stderr, "Usage: %s <by_sys_call / by_instruction> <program path> <program command inputs>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char* command_str = argv[1];
  char* user_program = argv[2];

  // Parse command line inputs
  char* inputs[argc - 1];
  inputs[0] = argv[2];
  for(int i = 3; i < argc; i++){
    inputs[i-2] = argv[i];
  }
  inputs[argc-2] = NULL;

  vector<shared_obj_t> shared_objs;
  //
  //
  //
  // /* get lib info */
  // dl_iterate_phdr(callback, vector);

  pid_t child;

  long orig_eax;

  /* Using libelfin to trace line numbers. */
  int fd = open(inputs[0], O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "%s: %s\n", inputs[0], strerror(errno));
    return 1;
  }

  elf::elf elf(elf::create_mmap_loader(fd));
  dwarf::dwarf dwarf(dwarf::elf::create_loader(elf));

  const std::vector<compilation_unit> compilation_units = dwarf.compilation_units();
  /* end of libelfin preparations */

  child = fork();
  if (child == -1)  {
    perror("Failed to fork process");
    exit(EXIT_FAILURE);
  } else if (child == 0) {

    /* we are in the child program. Run the debuggee */
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    //added command line arguments
    execv(inputs[0], inputs);

  } else {
    /* we are in the parent program. Run the debugger */

    // Parse debugee's memory maps
    int ret = populate_maps(child, shared_objs);
    if (ret) {
      perror("Failed to parse child's map file.");
      exit(EXIT_FAILURE);
    }

    /* A struct to store debuggee status */
    struct user_regs_struct regs;

    enum COMMAND command;
    if (strcmp(command_str, "by_sys_call") == 0) {
      command = SYS;
    } else if (strcmp(command_str, "by_instruction") == 0) {
      command = INST;
    } else {
      fprintf(stderr, "Invalid input. Input options: by_sys_call / by_instruction\n");
      exit(EXIT_FAILURE);
    }

    while (wait(NULL))  {

      if(ptrace(PTRACE_GETREGS, child, NULL, &regs)) {
        perror("ptrace GETREGS failed");
        exit(EXIT_FAILURE);
      }

      /* parse user instructions */
      if (command == SYS)  {

        printf("The child made a "
        "system call %llu\n", regs.orig_rax);
        getchar();
        ptrace(PTRACE_SYSCALL, child, NULL, NULL);
      } else if (command == INST)  {
        /* for each instruction call, check which lib did it come from */
        for (auto obj : shared_objs) {
          if (obj.addr_start <= regs.rip && regs.rip <= obj.addr_end) {
            printf("instruction: %p | file: %s\n", (void*)regs.rip, obj.name.c_str()); /* instruction pointer */

            if (regs.rip < 0x700000000000) {
              print_line_info(compilation_units, (void*)(regs.rip-obj.addr_start));
              getchar();
            }
          }
        }



        ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);
      }
    }
  }

  return 0;
}
