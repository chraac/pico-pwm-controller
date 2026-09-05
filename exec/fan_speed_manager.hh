#pragma once

#include <array>

#include "button_helper.hh"
#include "fan_speed_helper.hh"
#include "pid.hh"
#include "pwm_helper.hh"

namespace utility {

constexpr const uint kPwmFreqKhz = 25;
constexpr const uint8_t kPwmPinCount = 4;
constexpr const uint kPoolIntervalMs = 400;

class SingleFanSpeedManager {
public:
    enum class ControlMode {
        kTempToPwm = 0,
        kTempToRpm,
        kPwrToPwm,
    };

    explicit SingleFanSpeedManager(uint pwm_gpio_pin, uint spd_gpio_pin,
                                   ControlMode mode) noexcept;
    uint Next(float input) noexcept;
    uint GetTargetRpm() const noexcept { return target_rpm_; }
    uint GetFanSpeedRpm() noexcept { return speed_helper_.GetFanSpeedRpm(); }
    uint GetPwmGpioPin() const noexcept { return pwm_.GetGpioPin(); }
    uint32_t GetPwmCycle() const noexcept { return pwm_.GetDutyCycle(); }
    ControlMode GetControlMode() const noexcept { return mode_; }
    // true -> pwm comes straight from a curve; false -> pid to target rpm
    bool IsControlByPwm() const noexcept {
        return mode_ != ControlMode::kTempToRpm;
    }

private:
    PwmHelper pwm_;
    Pid pid_;
    FanSpeedHelper speed_helper_;
    uint target_rpm_;
    uint rpm_tolerance_;
    const ControlMode mode_;

    DISALLOW_COPY(SingleFanSpeedManager);
    DISALLOW_MOVE(SingleFanSpeedManager);
};

class FanSpeedManagerWithSelector {
public:
    FanSpeedManagerWithSelector(uint pwm_gpio_pin1, uint pwm_gpio_pin2,
                                uint pwm_gpio_pin3,
                                uint pwm_gpio_pin4) noexcept;
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
