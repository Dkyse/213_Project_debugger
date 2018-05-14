#ifndef _SHARED_OBJECT_HH_
#define _SHARED_OBJECT_HH_

#include <stdlib.h>
#include <stdint.h>

#include "elf++.hh"
#include "dwarf++.hh"

class shared_obj {
public:
  
  /**
  * construct a new shared object
  * @param file_path  the absolute path of the shared object file
  * @param addr_start the starting address where the shared object is loaded in
  *                   system memory
  * @param addr_end   the end address of the shared object in system memory
  */
  shared_obj(std::string file_path, intptr_t addr_start, intptr_t addr_end);

  /**
  * checks whether debugging (line number) information could be obtained for
  *   this shared object
  * @return true if this shared object has associated compilation_units;
  *         false otherwise.
  */
  auto has_cus() const -> bool { return has_compilation_units; }

  /**
  * @return absolute path of the shared object file
  */
  auto get_path() const -> std::string { return path; }

  /**
  * check whether an insturction address is contained withtin this shared object
  * @param  ip the instruction pointer to be checked
  * @return    true if ip is contained within this shared object, false otherwise.
  */
  bool contains(intptr_t ip);

  /**
  * translate a system memory address to an offset from the start of a
  *   shared object file
  * @param  ip an instruction pointer to a system memory address
  * @return    the corresponding address relative to the start of the file
  */
  intptr_t sys_mem_to_obj_off(intptr_t ip);

  /**
  * translate an address within a shared object file to a system memory address
  * @param  ip an address relative to the start of the file
  * @return    the corresponding instruction pointer to a system memory address
  */
  intptr_t obj_off_to_sys_mem(intptr_t ip);

  /**
  * get the line table entry corresponding to the given instruction pointer
  * @param  ip an instruction pointer within a shared object file
  * @return    the corresponding dwarf line table entry
  * @throws    std::out_of_range if no line entry is found
  * Source:
  * https://blog.tartanllama.xyz/writing-a-linux-debugger-source-signal/
  */
  dwarf::line_table::iterator get_line_entry_from_ip(intptr_t ip);

  /**
  * get the line table entry corresponding to the first instruction of the
  *   given function
  * @param  name the name of the function to be looked up
  * @return      the corresponding dwarf line table entry
  * @throws      std::out_of_range if no line entry is found
  * Source:
  * https://blog.tartanllama.xyz/writing-a-linux-debugger-source-signal/
  */
  dwarf::line_table::iterator get_line_entry_from_function(const std::string& name);

  /**
   * Print to stdin the line table of each source file correspoding to this
   *   shared object
   */
  void dump_all_line_tables();

  /**
   * Print to stdin a string representation of this shared object, in
   *  the following form:
   *  <start address>-<end address> <ELF type> <file path>
   */
  void print_string_form();
private:
  intptr_t addr_start;        // Start address of shared object
  intptr_t addr_end;          // End address of shared object
  std::string path;           // Absolute path of the shared object file
  elf::et type;               // Shared object's file ELF type (executable or dynamic object)
  bool has_compilation_units; // Whether this shared object has associated compilation units
  std::vector<dwarf::compilation_unit> compilation_units;
};

#endif /* _SHARED_OBJECT_HH_ */
