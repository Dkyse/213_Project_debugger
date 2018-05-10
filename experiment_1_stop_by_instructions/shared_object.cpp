#include <inttypes.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ptrace.h>

#include "shared_object.hh"

shared_obj::shared_obj(std::string file_path, int file_descriptor, intptr_t addr_start, intptr_t addr_end) {
  // file exists
  this->name = file_path;

  this->addr_start = addr_start;
  this->addr_end = addr_end;

  // Get compilation units for this file
  try {
    elf::elf elf(elf::create_mmap_loader(file_descriptor));
    dwarf::dwarf dwarf(dwarf::elf::create_loader(elf));
    this->type = elf.get_hdr().type;
    this->compilation_units = dwarf.compilation_units();
    this->has_compilation_units = true;
  } catch(dwarf::format_error& e) {
    this->has_compilation_units = false;
  }
}

/**
* [get_line_entry_from_ip description]
* @param  pc [description]
* @return    [description]
* Source:
* https://blog.tartanllama.xyz/writing-a-linux-debugger-source-signal/
*/
dwarf::line_table::iterator shared_obj::get_line_entry_from_ip(intptr_t ip) {
  /* walk through the each compilation unit's line table to find the line */
  for (auto &cu : compilation_units) {
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

/**
* [shared_obj::get_line_entry_from_ip description]
* @param  pc [description]
* @return    [description]
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

// TODO these names are terrible! change them!
intptr_t shared_obj::relative_ip_offset(intptr_t ip) {
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
*
* @param  obj [description]
* @param  ip  instruction pointer address relative to object start address
* @return     [description]
*/
// TODO these names are terrible! change them!
intptr_t shared_obj::absolute_ip_offset(intptr_t ip) {
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

bool shared_obj::contains(intptr_t ip) {
  return (addr_start <= ip) && (ip <= addr_end);
}

/***************************
* BEGIN TESTING FUNCTIONS *
***************************/
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

void shared_obj::dump_all_line_tables() {
  for (auto cu : compilation_units) {
    printf("--- <%x>\n", (unsigned int)cu.get_section_offset());
    dump_line_table(cu.get_line_table());
    printf("\n");
  }
}

void shared_obj::print_string_form() {
  printf("%lx-%lx\t%hu %s\n", addr_start, addr_end, type, name.c_str());
}
/*************************
* END TESTING FUNCTIONS *
*************************/
