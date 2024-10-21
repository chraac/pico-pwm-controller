#pragma once

#include <array>

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

class SingleFanSpeedManager {
public:
    SingleFanSpeedManager(uint pwm_gpio_pin, uint spd_gpio_pin) noexcept;
    void Next() noexcept;

private:
    PwmHelper pwm_;
    Pid pid_;
    FanSpeedHelper speed_helper_;
    uint target_rpm_;
    uint rpm_tolerance_;

    DISALLOW_COPY(SingleFanSpeedManager);
    DISALLOW_MOVE(SingleFanSpeedManager);
};

class FanSpeedManagerWithSelector {
public:
    FanSpeedManagerWithSelector(uint pwm_gpio_pin1 = kPwm1Pin,
                                uint pwm_gpio_pin2 = kPwm2Pin,
                                uint pwm_gpio_pin3 = kPwm3Pin,
                                uint pwm_gpio_pin4 = kPwm4Pin) noexcept;
    void Next() noexcept;

private:
    std::array<PwmHelper, kPwmPinCount> pwm_array_;
    std::array<Pid, kPwmPinCount> pid_array_;
    FanSpeedHelper speed_helper_;
    FanSpeedSelector selector_;
    uint8_t current_fan_ = 0;
    uint32_t max_fan_speed_ = 0;

    DISALLOW_COPY(FanSpeedManagerWithSelector);
    DISALLOW_MOVE(FanSpeedManagerWithSelector);
};

}  // namespace utility
