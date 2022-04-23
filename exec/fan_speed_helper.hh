#pragma once

#include <hardware/gpio.h>
#include <hardware/timer.h>

#include <atomic>

#include "base_types.hh"
#include "frequency_counter.hh"

namespace utility {

class FanSpeedHelper : public GpioBase {
public:
    FanSpeedHelper(const uint gpio_pin) noexcept
        : GpioBase(gpio_pin), freq_counter_(gpio_pin) {}

    uint32_t GetFanSpeedRpm() noexcept {
        // fan speed [rpm] = frequency [Hz] × 60 ÷ 2
        // See also:
        // https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf
        return freq_counter_.GetFrequencyHz() * 30;
    }

    void Reset() noexcept { freq_counter_.Reset(); }

private:
    GpioFreqencyCounter freq_counter_;

    DISALLOW_COPY(FanSpeedHelper);
    DISALLOW_MOVE(FanSpeedHelper);
};

}  // namespace utility
