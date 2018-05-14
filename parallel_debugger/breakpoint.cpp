#include <stdlib.h>
#include <stdint.h>
#include <sys/ptrace.h>

#include "breakpoint.hh"

/**
 * @return true if the breakpoint is enabled, false otherwise.
 */
void breakpoint::enable() {
  // Read the data stored at m_addr
  auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
  m_saved_data = static_cast<uint8_t>(data & 0xff); //save bottom byte
  // 0xcc is the 'send SIGTRAP' instruction
  uint64_t int3 = 0xcc;
  uint64_t data_with_int3 = ((data & ~0xff) | int3); //set bottom byte to 0xcc

  // Overwrite traced program's data
  ptrace(PTRACE_POKEDATA, m_pid, m_addr, data_with_int3);

  m_enabled = true;
}

/**
 * @return the address of this breakpoint
 */
void breakpoint::disable() {
  // Read the data stored at m_addr
  auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
  auto restored_data = ((data & ~0xff) | m_saved_data);
  // Restore the traced program's data
  ptrace(PTRACE_POKEDATA, m_pid, m_addr, restored_data);

  m_enabled = false;
}
