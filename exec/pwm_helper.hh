#pragma once

#include "hardware/pwm.h"
#include "system_clock.hh"

namespace utility {
constexpr uint kDefaultPwmTop = 9999;
constexpr uint kDefaultCycleDenom = kDefaultPwmTop + 1;

class PwmHelper {
   public:
    PwmHelper(const uint gpio_pin, const uint32_t freq_khz,
              const uint32_t top = kDefaultPwmTop) noexcept
        : _gpio_pin(gpio_pin), _pwm_config(pwm_get_default_config()) {
        // For more detail, see:
        // https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf, Page
        // 553
        pwm_config_set_clkdiv_int(
            &_pwm_config,
            SystemClock::GetInstance().GetClockKhz() / freq_khz / top);
        pwm_config_set_clkdiv_mode(&_pwm_config, PWM_DIV_FREE_RUNNING);
        pwm_config_set_wrap(&_pwm_config, top);

        auto slice_num = pwm_gpio_to_slice_num(gpio_pin);
        pwm_init(slice_num, &_pwm_config, true);
        gpio_set_function(_gpio_pin, GPIO_FUNC_PWM);
    }

    PwmHelper(const uint gpio_pin, const pwm_config &pwm_cfg) noexcept
        : _gpio_pin(gpio_pin), _pwm_config(pwm_cfg) {
        auto slice_num = pwm_gpio_to_slice_num(gpio_pin);
        pwm_init(slice_num, &_pwm_config, true);
        gpio_set_function(_gpio_pin, GPIO_FUNC_PWM);
    }

    void SetDutyCycle(uint32_t num,
                      uint32_t denom = kDefaultCycleDenom) noexcept {
        const auto max_cyc = _pwm_config.top;
        pwm_set_gpio_level(_gpio_pin, num * max_cyc / denom);
    }

   private:
    const uint _gpio_pin;
    pwm_config _pwm_config;

    DISALLOW_COPY(PwmHelper);
    DISALLOW_MOVE(PwmHelper);
};

}  // namespace utility
