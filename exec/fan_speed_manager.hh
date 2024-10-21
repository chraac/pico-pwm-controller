#pragma once

#include "button_helper.hh"
#include "fan_speed_helper.hh"
#include "pid.hh"
#include "pwm_helper.hh"

namespace utility {

constexpr const uint kPwmFreqKhz = 25;
constexpr const uint kPwm1Pin = 0;
constexpr const uint kPwm2Pin = 7;
constexpr const uint kPwm3Pin = 27;
constexpr const uint kPwm4Pin = 17;
constexpr const uint8_t kPwmPinCount = 4;
constexpr const uint kPoolIntervalMs = 400;

class FanSpeedManagerWithSelector {
public:
    FanSpeedManagerWithSelector(uint pwm_gpio_pin1 = kPwm1Pin,
                    uint pwm_gpio_pin2 = kPwm2Pin,
                    uint pwm_gpio_pin3 = kPwm3Pin,
                    uint pwm_gpio_pin4 = kPwm4Pin) noexcept;
    void next() noexcept;

private:
    PwmHelper pwm_array_[kPwmPinCount];
    Pid pid_array_[kPwmPinCount];
    FanSpeedHelper speed_helper_;
    FanSpeedSelector selector_;
    uint8_t current_fan_ = 0;
    uint32_t max_fan_speed_ = 0;

    DISALLOW_COPY(FanSpeedManagerWithSelector);
    DISALLOW_MOVE(FanSpeedManagerWithSelector);
};

}  // namespace utility
