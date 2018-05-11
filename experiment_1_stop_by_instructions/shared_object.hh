#ifndef _SHARED_OBJECT_HH_
#define _SHARED_OBJECT_HH_

#include <stdlib.h>
#include <stdint.h>

#include "elf++.hh"
#include "dwarf++.hh"

class shared_obj {
public:
  shared_obj(std::string file_path, intptr_t addr_start, intptr_t addr_end);

  auto has_cus() const -> bool { return has_compilation_units; }
  auto get_name() const -> std::string { return name; }
  bool contains(intptr_t ip);

  intptr_t relative_ip_offset(intptr_t ip);
  intptr_t absolute_ip_offset(intptr_t ip);

  dwarf::line_table::iterator get_line_entry_from_ip(intptr_t ip);
  dwarf::line_table::iterator get_line_entry_from_function(const std::string& name);

  void dump_all_line_tables();
  void print_string_form();
private:
  intptr_t addr_start;  // Start address of object
  intptr_t addr_end;    // End address of object
  std::string name;          // Object name
  elf::et type;         // Object file type
  bool has_compilation_units;         // Whether object has associated compilation units
  std::vector<dwarf::compilation_unit> compilation_units; // Object's compilation units
};

#endif /* _SHARED_OBJECT_HH_ */
