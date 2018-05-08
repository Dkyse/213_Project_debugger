/**
* Sources:
* https://stackoverflow.com/questions/36523584/how-to-see-memory-layout-of-my-program-in-c-during-run-time/36524010
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
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

// TODO remove
#include <inttypes.h>

using dwarf::compilation_unit;
using std::vector;
using std::string;

typedef struct shared_obj {
  intptr_t addr_start; // Start address of object
  intptr_t addr_end;  // End address of object
  string name; // Object name
  bool has_cus;
  vector<compilation_unit> compilation_units;
} shared_obj_t;

/***************************
* BEGIN TESTING FUNCTIONS *
***************************/
void
dump_line_table(const dwarf::line_table &lt)
{
  for (auto &line : lt) {
    if (line.end_sequence)
    printf("\n");
    else
    printf("%-40s%8d%#20" PRIx64 "\n", line.file->path.c_str(),
    line.line, line.address);
  }
}

void
dump_all_line_tables(const std::vector<compilation_unit> &cus) {
  for (auto cu : cus) {
    printf("--- <%x>\n", (unsigned int)cu.get_section_offset());
    dump_line_table(cu.get_line_table());
    printf("\n");
  }
}
/*************************
* END TESTING FUNCTIONS *
*************************/
/**
* [get_line_entry_from_ip description]
* @param  pc [description]
* @return    [description]
* Source:
* https://blog.tartanllama.xyz/writing-a-linux-debugger-source-signal/
*/
dwarf::line_table::iterator get_line_entry_from_ip(const std::vector<compilation_unit> &cus, intptr_t ip) {
  for (auto &cu : cus) {
    if (die_pc_range(cu.root()).contains(ip)) {
      auto &lt = cu.get_line_table();
      auto it = lt.find_address(ip);
      if (it == lt.end()) {
        throw std::out_of_range{"Cannot find line entry"};
      }
      else {
        return it;
      }
    }
  }

  throw std::out_of_range{"Cannot find line entry"};
}

// void set_breakpoint_at_address(pid_t pid, std::intptr_t addr) {
//   breakpoint bp {m_pid, addr};
//   bp.enable();
// }

/**
* [get_line_entry_from_ip description]
* @param  pc [description]
* @return    [description]
* Source:
* https://blog.tartanllama.xyz/writing-a-linux-debugger-source-signal/
*/
dwarf::line_table::iterator get_line_entry_from_function(const std::vector<compilation_unit> compilation_units, const std::string& name) {
  for (const auto& cu : compilation_units) {
    for (const auto& die : cu.root()) {
      if (die.has(dwarf::DW_AT::name) && at_name(die) == name) {
        // Find lowest address for this function DIE
        auto low_ip = at_low_pc(die);
        // auto entry = get_line_entry_from_ip(low_ip);
        auto &lt = cu.get_line_table();
        auto entry = lt.find_address(low_ip);
        if (entry == lt.end()) {
          throw std::out_of_range{"Cannot find line entry"};
        }
        else {
          // skip function prologue to point to actual code
          return ++entry;
        }
      }
    }
  }
  throw std::out_of_range{"Cannot find line entry"};
}

/**
* [print_line_number description]
* @param cus [description]
* @param ip  [description]
*/
bool print_line_info(shared_obj_t &obj, intptr_t rip) {
  // intptr_t file_off = rip-obj.addr_start;
  intptr_t file_off = rip;
  bool found = false;
  dwarf::line_table lt;
  if (obj.has_cus)  {
    printf("File path: %s\n", obj.name.c_str());
    try {
      auto entry = get_line_entry_from_ip(obj.compilation_units, file_off);
      // printf("rip: %p | off: %lx | file: %s\n", (void*)rip, file_off, entry->file->path.c_str());
      printf("Called from line %u\n\n", entry->line);
      found = true;
    } catch(std::out_of_range &e) {
      printf("No line numbers found.\n\n");
    }
  }
  return found;
}

/**
* [populate_shared_objs description]
* Source:
* https://stackoverflow.com/questions/36523584/how-to-see-memory-layout-of-my-program-in-c-during-run-time/36524010
*/
int populate_shared_objs(pid_t child, vector<shared_obj_t> &objects) {
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

    string name;
    // Check for valid name
    if (name_end > name_start)  {
      name = string (line + name_start, name_end - name_start);

      int fd = open(name.c_str(), O_RDONLY);

      /* Check if entry is a readable file */
      if( fd != -1 ) {
        // file exists
        obj.name = name;

        obj.addr_start = addr_start;
        obj.addr_end = addr_end;

        // Get compilation units for this file
        try {
          elf::elf elf(elf::create_mmap_loader(fd));
          dwarf::dwarf dwarf(dwarf::elf::create_loader(elf));
          obj.compilation_units = dwarf.compilation_units();
          obj.has_cus = true;
        } catch(dwarf::format_error& e) {
          obj.has_cus = false;
        }

        // Add object to vector
        objects.push_back(obj);

      }
    }
  } /* end of while */

  // Wrap up
  fclose(maps);
  free(line);
  return 0;
}

int main(int argc, char** argv)  {

  /* check and store arguments */

  if(argc < 2) {
    fprintf(stderr, "Usage: %s <program path> <program command inputs>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char* user_program = argv[1];

  // Process command line inputs to pass them to execv
  char* inputs[argc];
  inputs[0] = argv[1];
  for(int i = 2; i < argc; i++) {
    inputs[i-1] = argv[i];
  }
  inputs[argc-1] = NULL;

  /* a vector to store information and line-table for all files involved */
  vector<shared_obj_t> shared_objs;

  /* debuggee */
  pid_t child;

  child = fork();

  if (child == -1)  {
    perror("Failed to fork process");
    exit(EXIT_FAILURE);
  } else if (child == 0) {

    /* we are in the child program. Run the debuggee */
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    execv(inputs[0], inputs);

  } else {

    /* we are in the parent program. Run the debugger */

    /* Wait for child to execute new process */
    // TODO ask Derek how to wait for clone/fork/exec
    int status;
    pid_t ret = waitpid(child, &status, 0);

    /* Let child advance to next instruction */
    ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);

    /* Parse child's memory maps */
    /* Store info into shared_objs vector */
    if (populate_shared_objs(child, shared_objs)) {
      perror("Failed to parse child's map file.");
      exit(EXIT_FAILURE);
    }

    // TODO remove
    printf("SHARED OBJECT TABLE:\n");
    for (auto obj : shared_objs) {
      printf("%lx-%lx\t%s\n", obj.addr_start, obj.addr_end, obj.name.c_str());
    }
    getchar();
    printf("\n\n");

    /* Set breakpoint for child's main function. */
    try {
      // We assume the main executable is the first entry of the maps table
      auto entry = get_line_entry_from_function(shared_objs[0].compilation_units, "main");
      breakpoint bp {child, (intptr_t)entry->address};
      bp.enable();
      // Continue until we reach the breakpoint
      ptrace(PTRACE_CONT, child, NULL, NULL);
      waitpid(child, &status, 0);

      // Breakpoint reached, advance to next instruction
      bp.disable();
      ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);
    } catch(std::out_of_range &e) {
      fprintf(stderr, "'%s' main function not found", inputs[0]);
    }

    /* A struct to store debuggee status */
    struct user_regs_struct regs;

    // printf("Loading environment......\n\n\n");

    while (wait(NULL))  {
      if(ptrace(PTRACE_GETREGS, child, NULL, &regs)) {
        perror("ptrace GETREGS failed");
        exit(EXIT_FAILURE);
      }

      /* for each instruction call, check which lib did it come from
      * by walking through the shared_obj table
      */
      for (auto obj : shared_objs) {
        /* if a file is found, check line table for that instruction */
        if (obj.addr_start <= regs.rip && regs.rip <= obj.addr_end) {
          printf("rip: %p | file: %s\n", (void*)regs.rip, obj.name.c_str());
          bool found = print_line_info(obj, regs.rip);
          if (found) {
            getchar();
          }
          break;
        }
      }

      ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);
    }
  }

  return 0;
}
