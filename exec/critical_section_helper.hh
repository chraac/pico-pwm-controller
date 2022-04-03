#pragma once

#include <pico/critical_section.h>

#include "base_types.hh"

namespace utility {

class CriticalSection {
   public:
    CriticalSection() noexcept { critical_section_init(&critical_section_); }

    ~CriticalSection() noexcept { critical_section_deinit(&critical_section_); }

    void Lock() noexcept {
        critical_section_enter_blocking(&critical_section_);
    }

    void Unlock() noexcept { critical_section_exit(&critical_section_); }

   private:
    critical_section_t critical_section_;

    DISALLOW_COPY(CriticalSection);
    DISALLOW_MOVE(CriticalSection);
};

}  // namespace utility
