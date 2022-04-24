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

class FanSpeedSelector {
public:
    FanSpeedSelector(uint gpio_bit3, uint gpio_bit2, uint gpio_bit1,
                     uint gpio_bit0) noexcept
        : gpio_pin_bit0_(gpio_bit0),
          gpio_pin_bit1_(gpio_bit1),
          gpio_pin_bit2_(gpio_bit2),
          gpio_pin_bit3_(gpio_bit3) {}

    void SelectFan(uint8_t fan_index) noexcept {
        auto SetGpioPinValue = [](uint gpio_pin, bool is_pull_up) {
            if (is_pull_up) {
                gpio_pull_up(gpio_pin);
            } else {
                gpio_pull_down(gpio_pin);
            }
        };

        SetGpioPinValue(gpio_pin_bit0_, fan_index & 1);
        SetGpioPinValue(gpio_pin_bit1_, fan_index & (1 << 1));
        SetGpioPinValue(gpio_pin_bit2_, fan_index & (1 << 2));
        SetGpioPinValue(gpio_pin_bit3_, fan_index & (1 << 3));
    }

private:
    const uint gpio_pin_bit0_;
    const uint gpio_pin_bit1_;
    const uint gpio_pin_bit2_;
    const uint gpio_pin_bit3_;

    DISALLOW_COPY(FanSpeedSelector);
    DISALLOW_MOVE(FanSpeedSelector);
};

}  // namespace utility
