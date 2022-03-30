#pragma once

#include <pico/critical_section.h>

#include "base_types.hh"

namespace utility {

class CriticalSection {
   public:
    CriticalSection() noexcept { critical_section_init(&_critical_section); }

    ~CriticalSection() noexcept { critical_section_deinit(&_critical_section); }

    void Lock() noexcept {
        critical_section_enter_blocking(&_critical_section);
    }

    void Unlock() noexcept { critical_section_exit(&_critical_section); }

   private:
    critical_section_t _critical_section;

    DISALLOW_COPY(CriticalSection);
    DISALLOW_MOVE(CriticalSection);
};

}  // namespace utility
