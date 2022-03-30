#pragma once

#include <hardware/clocks.h>

#include "base_types.hh"

namespace utility {
class SystemClock : public Singleton<SystemClock> {
   public:
    SystemClock() noexcept : _sys_clk_hz(clock_get_hz(clk_sys)) {}

    uint32_t GetClockHz() const { return _sys_clk_hz; }
    uint32_t GetClockKhz() const { return _sys_clk_hz / KHZ; }
    uint32_t GetClockMhz() const { return _sys_clk_hz / MHZ; }

   private:
    const uint32_t _sys_clk_hz;

    DISALLOW_COPY(SystemClock);
    DISALLOW_MOVE(SystemClock);
};
}  // namespace utility