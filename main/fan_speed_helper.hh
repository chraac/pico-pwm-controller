#pragma once

#include <hardware/gpio.h>
#include <hardware/timer.h>

#include "base_types.hh"
#include "frequency_counter.hh"

namespace utility {

class FanSpeedHelper : public GpioBase {
public:
    FanSpeedHelper(const uint32_t gpio_pin) noexcept
        : GpioBase(gpio_pin), freq_counter_(gpio_pin) {}

    uint32_t GetFanSpeedRpm() noexcept {
        // fan speed [rpm] = frequency [Hz] × 60 ÷ 2
        // See also:
        // https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf
        return freq_counter_.GetFrequencyMilliHertz() * 30 / 1000;
    }

    void Reset() noexcept { freq_counter_.Reset(); }

private:
    GpioFreqencyCounter freq_counter_;

    DISALLOW_COPY(FanSpeedHelper);
    DISALLOW_MOVE(FanSpeedHelper);
};

class FanSpeedSelector {
public:
    FanSpeedSelector(uint32_t gpio_bit3, uint32_t gpio_bit2, uint32_t gpio_bit1,
                     uint32_t gpio_bit0) noexcept
        : gpio_pin_bit0_(gpio_bit0),
          gpio_pin_bit1_(gpio_bit1),
          gpio_pin_bit2_(gpio_bit2),
          gpio_pin_bit3_(gpio_bit3) {
        SelectFan(0);
    }

    void SelectFan(uint8_t fan_index) noexcept {
        SetGpioPinValue(gpio_pin_bit3_, fan_index & (1 << 3));
        SetGpioPinValue(gpio_pin_bit2_, fan_index & (1 << 2));
        SetGpioPinValue(gpio_pin_bit1_, fan_index & (1 << 1));
        SetGpioPinValue(gpio_pin_bit0_, fan_index & 1);
        // Wait 1ms for the analog switch ready.
        // http://www.utc-ic.com/uploadfile/2011/0923/20110923124731897.pdf
        sleep_ms(1);
    }

private:
    static void SetGpioPinValue(uint32_t gpio_pin, bool is_pull_up) {
        if (is_pull_up) {
            gpio_pull_up(gpio_pin);
        } else {
            gpio_pull_down(gpio_pin);
        }
    }

    const uint32_t gpio_pin_bit0_;
    const uint32_t gpio_pin_bit1_;
    const uint32_t gpio_pin_bit2_;
    const uint32_t gpio_pin_bit3_;

    DISALLOW_COPY(FanSpeedSelector);
    DISALLOW_MOVE(FanSpeedSelector);
};

class Aw9523FanSpeedHelper {
public:
    Aw9523FanSpeedHelper(const uint32_t gpio_scl, const uint32_t gpio_sda,
                         const uint32_t gpio_ad0, const uint32_t gpio_ad1,
                         const uint32_t gpio_intr,
                         const uint32_t gpio_rst) noexcept
        : freq_counter_(gpio_scl, gpio_sda, gpio_ad0, gpio_ad1, gpio_intr,
                        gpio_rst) {}

    uint32_t GetFanSpeedRpm(uint32_t pin) noexcept {
        // fan speed [rpm] = frequency [Hz] × 60 ÷ 2
        // See also:
        // https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf
        return freq_counter_.GetFrequencyMilliHertz(
                   Aw9523bFreqencyCounter::FreqPin(pin)) *
               30 / 1000;
    }

    void Reset(uint32_t pin) noexcept {
        freq_counter_.Reset(Aw9523bFreqencyCounter::FreqPin(pin));
    }

private:
    Aw9523bFreqencyCounter freq_counter_;

    DISALLOW_COPY(Aw9523FanSpeedHelper);
    DISALLOW_MOVE(Aw9523FanSpeedHelper);
};

}  // namespace utility
