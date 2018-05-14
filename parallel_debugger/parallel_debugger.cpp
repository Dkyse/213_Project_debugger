#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <link.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "elf++.hh"
#include "dwarf++.hh"
#include "breakpoint.hh"
#include "shared_object.hh"

using dwarf::compilation_unit;
using std::vector;
using std::string;

/**
 * Parses a /proc/<pid>/maps file to determine the virtual memory locations of
 * the shared objects linked to the main executable, and stores that information
 * in the given shared_obj vector.
 * @param  child   the pid of the process traced being
 * @param  objects a vector to store the parsed information
 * @return         0 if the maps file was processed correctly, 0 on failure.
 * Source:
 * https://stackoverflow.com/questions/36523584/how-to-see-memory-layout-of-my-program-in-c-during-run-time/36524010
 */
int populate_shared_objs(pid_t child, vector<shared_obj> &objects) {
  char* line = NULL;
  size_t size = 0;

  // Open maps file
  char maps_path[128];
  snprintf(maps_path, 128, "/proc/%d/maps", child);
  FILE* maps = fopen(maps_path, "r");
  if (maps == NULL) {
    fprintf(stderr, "Failed to open %s\n", maps_path);
    exit(EXIT_FAILURE);
  }

  // Parse maps file
  while (getline(&line, &size, maps) > 0) {

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

    string name;
    // Check for valid name
    if (name_end > name_start)  {
      name = string (line + name_start, name_end - name_start);

      /* Create a new shared object from this entry */
      try {
        shared_obj obj (name, addr_start, addr_end);

        // Add object to vector
        objects.push_back(obj);
      } catch(std::invalid_argument &e) {
        // shared_obj will throw an exception when name is not a valid file,
        //   which we can safely ignore
      }
    }
  } /* end of while */

  // Wrap up
  fclose(maps);
  free(line);
  return 0;
}

/**
* Sets a break point at the child's main function, and advances the child's
* execution until the main function is reached.
* @param child    the pid of the traced process
* @param main_obj the shared object corresponding to the file containing the
*                 child's main function
*/
void break_at_main(pid_t child, shared_obj &main_obj) {
  // To store debuggee status
  struct user_regs_struct regs;

  dwarf::line_table::entry main_entry;

  // Get main function line table entry
  try {
    main_entry = *main_obj.get_line_entry_from_function("main");
  } catch(std::out_of_range &e) {
    // Continue execution if no main function is not found
    fprintf(stderr, "'%s' main function not found\n", main_obj.get_name().c_str());
    return;
  }

  // Set breakpoint
  breakpoint bp {child, main_obj.absolute_ip_offset(main_entry.address)};
  bp.enable();

  // Continue until we reach the breakpoint
  intptr_t prev_ip;
  do {
    // Allow child to continue until the next state change
    if (ptrace(PTRACE_CONT, child, NULL, NULL) == -1) {
      perror("Error in ptrace with PTRACE_CONT");
      exit(EXIT_FAILURE);
    }
    // Wait for child to change state
    if (waitpid(child, NULL, 0) == -1) {
      perror("Error in waitpid");
      exit(EXIT_FAILURE);
    }
    // Get child's register contents
    if (ptrace(PTRACE_GETREGS, child, NULL, &regs) == -1) {
      perror("Error in ptrace with PTRACE_GETREGS");
      exit(EXIT_FAILURE);
    }
    // possible breakpoint location
    prev_ip = main_obj.relative_ip_offset(regs.rip - 1);
  } while(main_entry.address != prev_ip);

  // Breakpoint reached, reset ip to that location
  regs.rip =  main_obj.absolute_ip_offset(prev_ip);
  ptrace(PTRACE_SETREGS, child, NULL, &regs);

  // restore breakpoint instruction
  bp.disable();
}

/**
* Given an instruction pointer and its object, find line info
* @param  obj a shared object entry
* @param  rip instruction pointer
* @return     true if the line is found, false otherwise
*/
bool print_line_info(shared_obj &obj, intptr_t rip) {
  /* return value set to false */
  bool found = false;

  /* if the object has line table */
  if (obj.has_cus())  {
    try {
      auto entry = obj.get_line_entry_from_ip(rip);
      /* If we find the line, print it */
      printf("File path: %s\n", entry->file->path.c_str());
      printf("Called from line %u\n\n", entry->line);
      found = true;
    } catch(std::out_of_range &e) {
      /* Line was not found */
      printf("File path: %s\n", obj.get_name().c_str());
      printf("No line numbers found.\n\n");
    }
  } else {
    printf("File path: %s\n", obj.get_name().c_str());
    printf("No debug information available.\n\n");
  }
  return found;
}

int main(int argc, char** argv)  {

  /* Parse command line arguments */
  if(argc < 2) {
    fprintf(stderr, "Usage: %s <program path> <program command inputs>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Process command line inputs to pass them to execv
  char* inputs[argc];
  inputs[0] = argv[1];
  for(int i = 2; i < argc; i++) {
    inputs[i-1] = argv[i];
  }
  inputs[argc-1] = NULL;

  /* a vector to store information and line-table for all files involved */
  vector<shared_obj> shared_objs;

  /* debuggee */
  pid_t child;
  child = fork();

  if (child == -1)  {
    perror("Failed to fork process");
    exit(EXIT_FAILURE);
  } else if (child == 0) {

    /* In the child program. Run the debuggee */
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    execv(inputs[0], inputs);

  } else {

    /* In the parent program. Run the debugger */

    /* Wait for child to execute new process */
    int status;
    if (waitpid(child, &status, 0) == -1) {
      perror("Error in waitpid");
      exit(EXIT_FAILURE);
    }

    // Enable tracing new threads
    ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE);

    /* Parse child's memory maps */
    /* Store info into shared_objs vector */
    if (populate_shared_objs(child, shared_objs)) {
      perror("Failed to parse child's map file.");
      exit(EXIT_FAILURE);
    }

    // TODO remove
    printf("SHARED OBJECT TABLE:\n");
    for (auto obj : shared_objs) {
      obj.print_string_form();
    }
    getchar();
    printf("\n\n");

    /* Set breakpoint for child's main function. */
    // We assume the main executable is the first entry of the maps table
    break_at_main(child, shared_objs[0]);

    /* A struct to store debuggee status */
    struct user_regs_struct regs;

    /* Advance to child's next instruction */
    if (ptrace(PTRACE_SINGLESTEP, child, NULL, NULL) == -1) {
      perror("Error in ptrace with PTRACE_SINGLESTEP");
      exit(EXIT_FAILURE);
    }

    while (true)  {
      // Wait for any of the child's threads to change status
      pid_t current = waitpid(-1, &status, 0);

      // Check whether all threads haves exited
      if (current == -1){
        break;
      }

      // Get current thread's register contents
      if (ptrace(PTRACE_GETREGS, current, NULL, &regs) == -1) {
        perror("Error in ptrace with PTRACE_GETREGS");
        exit(EXIT_FAILURE);
      }

      /*For each instruction call, determine which source file it comes from
      * by walking through the shared_obj vector
      */
      for (auto obj : shared_objs) {
        /* if a file is found, check line table for that instruction */
        if (obj.contains(regs.rip)) {
          printf("Thread ID (PID): %d | Instruction address: %llx\n", current, regs.rip);
          bool found = print_line_info(obj, regs.rip);
          if (found) {
            // Stop execution when next line number is found
            getchar();
          }
          break;
        }
      }

      // Advance the current thread a single instruction
      if (ptrace(PTRACE_SINGLESTEP, current, NULL, NULL) == -1) {
        perror("Error in ptrace with PTRACE_SINGLESTEP");
        exit(EXIT_FAILURE);
      }
    }
  }

  printf("\nProgram '%s' terminated.\n", inputs[0]);
  return 0;
}
