#pragma once

#include <hardware/pwm.h>

#include "system_clock.hh"

namespace utility {
constexpr uint kDefaultPwmTop = 9999;
constexpr uint kDefaultCycleDenom = kDefaultPwmTop + 1;

class PwmHelper : public GpioBase {
public:
    PwmHelper(const uint gpio_pin, const uint32_t freq_khz,
              const uint32_t top = kDefaultPwmTop) noexcept
        : GpioBase(gpio_pin), pwm_config_(pwm_get_default_config()) {
        // For more detail, see:
        // https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf, Page
        // 553
        pwm_config_set_clkdiv_int(
            &pwm_config_,
            SystemClock::GetInstance().GetClockKhz() / freq_khz / top);
        pwm_config_set_clkdiv_mode(&pwm_config_, PWM_DIV_FREE_RUNNING);
        pwm_config_set_wrap(&pwm_config_, top);

        auto slice_num = pwm_gpio_to_slice_num(gpio_pin);
        pwm_init(slice_num, &pwm_config_, true);
        gpio_set_function(gpio_pin_, GPIO_FUNC_PWM);
    }

    PwmHelper(const uint gpio_pin, const pwm_config &pwm_cfg) noexcept
        : GpioBase(gpio_pin), pwm_config_(pwm_cfg) {
        auto slice_num = pwm_gpio_to_slice_num(gpio_pin);
        pwm_init(slice_num, &pwm_config_, true);
        gpio_set_function(gpio_pin_, GPIO_FUNC_PWM);
    }

    PwmHelper(PwmHelper &&other) noexcept : GpioBase(other.gpio_pin_) {
        pwm_config_ = other.pwm_config_;
        other.pwm_config_ = pwm_get_default_config();
    }

    void SetDutyCycle(uint32_t num,
                      uint32_t denom = kDefaultCycleDenom) noexcept {
        const auto max_cyc = pwm_config_.top;
        pwm_set_gpio_level(gpio_pin_, num * max_cyc / denom);
        cycle_ = num;
    }

    uint32_t GetDutyCycle() const noexcept { return cycle_; }

private:
    pwm_config pwm_config_;
    uint32_t cycle_ = 0;

    void operator=(PwmHelper &&) = delete;
    DISALLOW_COPY(PwmHelper);
};

}  // namespace utility
