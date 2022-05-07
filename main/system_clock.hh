#pragma once

#include <hardware/clocks.h>

#include "base_types.hh"

namespace utility {
class SystemClock : public Singleton<SystemClock> {
public:
    SystemClock() noexcept : sys_clk_hz_(clock_get_hz(clk_sys)) {}

    uint32_t GetClockHz() const { return sys_clk_hz_; }
    uint32_t GetClockKhz() const { return sys_clk_hz_ / KHZ; }
    uint32_t GetClockMhz() const { return sys_clk_hz_ / MHZ; }

private:
    const uint32_t sys_clk_hz_;

    DISALLOW_COPY(SystemClock);
    DISALLOW_MOVE(SystemClock);
};
}  // namespace utility