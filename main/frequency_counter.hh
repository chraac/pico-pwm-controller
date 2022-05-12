#pragma once

#include "base_types.hh"

namespace utility {

class GpioFreqencyCounter : public GpioBase {
public:
    explicit GpioFreqencyCounter(const uint32_t gpio_pin) noexcept;

    // Get frequency in MilliHertz
    // See also: https://www.convertworld.com/en/frequency/millihertz.html
    uint32_t GetFrequencyMilliHertz() noexcept;

    void Reset() noexcept;

private:
    uint64_t last_time_us_;

    DISALLOW_COPY(GpioFreqencyCounter);
    DISALLOW_MOVE(GpioFreqencyCounter);
};

}  // namespace utility
