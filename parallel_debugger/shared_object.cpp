#include <inttypes.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ptrace.h>
#include <unistd.h>

#include "shared_object.hh"

/**
* construct a new shared object
* @param file_path  the absolute path of the shared object file
* @param addr_start the starting address where the shared object is loaded in
*                   system memory
* @param addr_end   the end address of the shared object in system memory
*/
shared_obj::shared_obj(std::string file_path, intptr_t addr_start, intptr_t addr_end) {

  int fd = open(file_path.c_str(), O_RDONLY);

  // Check if this is a readable file
  if( fd == -1 ) {
    throw std::invalid_argument{"Cannot open file '" + file_path + "'"};
  }

  // file exists
  this->path = file_path;

  this->addr_start = addr_start;
  this->addr_end = addr_end;

  // Initialize this shared object's fields
  try {
    elf::elf elf(elf::create_mmap_loader(fd));
    dwarf::dwarf dwarf(dwarf::elf::create_loader(elf));
    // ELF type (exec or dynamic)
    this->type = elf.get_hdr().type;
    this->compilation_units = dwarf.compilation_units();
    this->has_compilation_units = true;
  } catch(dwarf::format_error& e) {
    // If file is not a valid dwarf file
    this->has_compilation_units = false;
  }

  // Wrap up
  close(fd);
}

/**
* check whether an insturction address is contained withtin this shared object
* @param  ip the instruction pointer to be checked
* @return    true if ip is contained within this shared object, false otherwise.
*/
bool shared_obj::contains(intptr_t ip) {
  return (addr_start <= ip) && (ip <= addr_end);
}

/**
* translate a system memory address to an offset from the start of a
*   shared object file
* @param  ip an instruction pointer to a system memory address
* @return    the corresponding address relative to the start of the file
*/
intptr_t shared_obj::sys_mem_to_obj_off(intptr_t ip) {
  switch (type) {
    case elf::et::exec:
    // exec files have relative addresses
    return ip;
    break;

    case elf::et::dyn:
    // dynamic files have absolute addresses
    return ip - addr_start;
    break;

    default:
    // We don't consider other types
    return ip;
  }
}

/**
* translate an address within a shared object file to a system memory address
* @param  ip an address relative to the start of the file
* @return    the corresponding instruction pointer to a system memory address
*/
intptr_t shared_obj::obj_off_to_sys_mem(intptr_t ip) {
  switch (type) {
    case elf::et::exec:
    // exec files have relative addresses
    return ip;
    break;

    case elf::et::dyn:
    // dynamic files have absolute addresses
    return ip + addr_start;
    break;

    default:
    // We don't consider other types
    return ip;
  }
}

/**
* get the line table entry corresponding to the given instruction pointer
* @param  ip an instruction pointer within a shared object file
* @return    the corresponding dwarf line table entry
* @throws    std::out_of_range if no line entry is found
* Source:
* https://blog.tartanllama.xyz/writing-a-linux-debugger-source-signal/
*/
dwarf::line_table::iterator shared_obj::get_line_entry_from_ip(intptr_t ip) {
  /* calculate offset of the instruction pointer from the beginning of the file */
  intptr_t file_off = sys_mem_to_obj_off(ip);

  /* walk through the each compilation unit's line table to find the line */
  for (auto &cu : compilation_units) {
    if (die_pc_range(cu.root()).contains(file_off)) {
      auto &lt = cu.get_line_table();
      auto it = lt.find_address(file_off);
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

/**
* get the line table entry corresponding to the first instruction of the
*   given function
* @param  name the name of the function to be looked up
* @return      the corresponding dwarf line table entry
* @throws      std::out_of_range if no line entry is found
* Source:
* https://blog.tartanllama.xyz/writing-a-linux-debugger-source-signal/
*/
dwarf::line_table::iterator shared_obj::get_line_entry_from_function(const std::string& name) {
  /* walk through the each compilation unit's line table to find the line */
  for (const auto& cu : compilation_units) {
    for (const auto& die : cu.root()) {
      if (die.has(dwarf::DW_AT::name) && at_name(die) == name) {
        // Find lowest address for this function DIE
        auto low_ip = at_low_pc(die);
        auto &lt = cu.get_line_table();
        auto entry = lt.find_address(low_ip);
        if (entry == lt.end()) {
          throw std::out_of_range{"Cannot find line entry"};
        }
        else {
          // skip function prologue to point to actual user code
          return ++entry;
        }
      }
    }
  }
  throw std::out_of_range{"Cannot find line entry"};
}

/********************
* TESTING FUNCTIONS *
*********************/
/**
 * Print to stdin each entry of the given line table, in the following format
 *   <file path> <line number> <instruction address>
 * @param lt a line table
 */
void dump_line_table(const dwarf::line_table &lt)
{
  for (auto &line : lt) {
    if (line.end_sequence)
    printf("\n");
    else
    printf("%-40s%8d%#20" PRIx64 "\n", line.file->path.c_str(),
    line.line, line.address);
  }
}

/**
 * Print to stdin the line table of each source file correspoding to this
 *   shared object
 */
void shared_obj::dump_all_line_tables() {
  for (auto cu : compilation_units) {
    printf("--- <%x>\n", (unsigned int)cu.get_section_offset());
    dump_line_table(cu.get_line_table());
    printf("\n");
  }
}

/**
 * Print to stdin a string representation of this shared object, in
 *  the following form:
 *  <start address>-<end address> <ELF type> <file path>
 */
void shared_obj::print_string_form() {
  printf("%lx-%lx\t%hu %s\n", addr_start, addr_end, type, path.c_str());
}
