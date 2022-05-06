#include "pwm_helper.hh"

using namespace utility;

PwmHelper::PwmHelper(const uint gpio_pin, const uint32_t freq_khz,
                     const uint32_t top) noexcept
    : GpioBase(gpio_pin), pwm_config_({}) {
    ;
}

PwmHelper::PwmHelper(PwmHelper &&other) noexcept : GpioBase(other.gpio_pin_) {
    pwm_config_ = other.pwm_config_;
    other.pwm_config_ = {};
}

void PwmHelper::SetDutyCycle(uint32_t num,
                             uint32_t denom) noexcept {
    ;
}
