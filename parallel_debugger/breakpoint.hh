/**
* Largely based on code from:
* https://blog.tartanllama.xyz/writing-a-linux-debugger-breakpoints/
* https://blog.tartanllama.xyz/writing-a-linux-debugger-source-break/
*/

#ifndef _BREAKPOINT_HH_
#define _BREAKPOINT_HH_

#include <stdlib.h>
#include <stdint.h>

class breakpoint {
public:
  /**
  * construct a new breakpoint
  * @param pid  process id where breakpoint will be inserted
  * @param addr address of the new breakpoint
  */
  breakpoint (pid_t pid, intptr_t addr)
  : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_saved_data{}
  {}

  /**
   * Enable this breakpoint
   */
  void enable();

  /**
   * Disable this breakpoint
   */
  void disable();

  /**
   * @return true if the breakpoint is enabled, false otherwise.
   */
  auto is_enabled() const -> bool { return m_enabled; }

  /**
   * @return the address of this breakpoint
   */
  auto get_address() const -> intptr_t { return m_addr; }

private:
  pid_t m_pid;          // the pid of the process where this breakpoint is located
  intptr_t m_addr;      // the address of the breakpoint
  bool m_enabled;       // whether this breakpoint is enabled
  uint8_t m_saved_data; // data originally at the breakpoint address
};

#endif /* _BREAKPOINT_HH_ */
