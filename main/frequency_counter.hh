#pragma once

#include "aw9523_helper.hh"
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

class Aw9523bFreqencyCounter {
public:
    typedef enum _FreqPin {
        kPin0 = 0,
        kPin1,
        kPin2,
        kPin3,
        kPin4,
        kPin5,
        kPin6,
        kPin7,
        kPin8,
        kPin9,
        kPin10,
        kPin11,
        kPin12,
        kPin13,
        kPin14,
        kPin15,
        kPinCount,
    } FreqPin;

    explicit Aw9523bFreqencyCounter(const uint32_t gpio_scl,
                                    const uint32_t gpio_sda,
                                    const uint32_t gpio_ad0,
                                    const uint32_t gpio_ad1,
                                    const uint32_t gpio_intr,
                                    const uint32_t gpio_rst) noexcept;

    // Get frequency in MilliHertz
    // See also: https://www.convertworld.com/en/frequency/millihertz.html
    uint32_t GetFrequencyMilliHertz(FreqPin pin) noexcept;

    void Reset(FreqPin pin) noexcept;

private:
    uint64_t last_time_us_[kPinCount] = {};
    Aw9523Helper aw9523_;

    DISALLOW_COPY(Aw9523bFreqencyCounter);
    DISALLOW_MOVE(Aw9523bFreqencyCounter);
};

}  // namespace utility
