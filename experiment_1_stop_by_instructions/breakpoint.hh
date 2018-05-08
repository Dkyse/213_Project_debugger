/**
 * Source:
 * https://blog.tartanllama.xyz/writing-a-linux-debugger-breakpoints/
 * https://blog.tartanllama.xyz/writing-a-linux-debugger-source-break/
 */

#ifndef _BREAKPOINT_HH_
#define _BREAKPOINT_HH_

#include <stdlib.h>
#include <stdint.h>

class breakpoint {
public:
    breakpoint(pid_t pid, intptr_t addr)
        : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_saved_data{}
    {}

    void enable();
    void disable();

    auto is_enabled() const -> bool { return m_enabled; }
    auto get_address() const -> intptr_t { return m_addr; }

private:
    pid_t m_pid;
    intptr_t m_addr;
    bool m_enabled;
    uint8_t m_saved_data; //data which used to be at the breakpoint address
};

#endif /* _BREAKPOINT_HH_ */
