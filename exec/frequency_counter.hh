#pragma once

#include <hardware/timer.h>

#include "base_types.hh"
#include "critical_section_helper.hh"

namespace utility {

constexpr const uint32_t kGpioPinCount = 30;

class GpioFreqencyCounter : public GpioBase {
public:
    GpioFreqencyCounter(const uint gpio_pin) noexcept;

    uint32_t GetFrequencyMilliHertz() noexcept;
    void Reset() noexcept;

private:
    static void GpioEventHandler(uint gpio, uint32_t events);

    static uint32_t event_count_[kGpioPinCount];
    static CriticalSection event_count_critical_section_;

    uint64_t last_time_us_ = time_us_64();

    DISALLOW_COPY(GpioFreqencyCounter);
    DISALLOW_MOVE(GpioFreqencyCounter);
};

}  // namespace utility
