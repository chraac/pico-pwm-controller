
#include "fan_speed_manager.hh"

#include <iterator>

#include "logger.hh"

namespace {
constexpr uint kButton1Pin = 14;
constexpr uint kButton2Pin = 15;
constexpr uint kButton3Pin = 16;
constexpr uint kFanSpeedPin = 13;
constexpr uint kFanSelPin0 = 11;
constexpr uint kFanSelPin1 = 10;
constexpr uint kFanSelPin2 = 9;
constexpr uint kFanSelPin3 = 8;
constexpr uint8_t kFanIndexArray[] = {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13};
constexpr uint8_t kFanCount = std::size(kFanIndexArray);
constexpr uint8_t kFanCountPerGroup = kFanCount / utility::kPwmPinCount;

// See also:
// https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf
constexpr uint kFanSpeedStepRpm = 30 * 1000 / utility::kPoolIntervalMs;
constexpr uint kTargetRpm = 1900;
constexpr uint kRpmTolerance = 50;
constexpr uint kMaxTargetRpm = kTargetRpm + kFanSpeedStepRpm;
constexpr auto kStartCycle = 500;
constexpr auto kP = .5F;
constexpr auto kI = .3F;
constexpr auto kD = .02F;

}  // namespace

namespace utility {

SingleFanSpeedManager::SingleFanSpeedManager(uint pwm_gpio_pin,
                                             uint spd_gpio_pin)
    : pwm_(pwm_gpio_pin, kPwmFreqKhz),
      pid_(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD),
      speed_helper_(spd_gpio_pin),
      target_rpm_(kTargetRpm),
      rpm_tolerance_(kRpmTolerance) {
    pwm_.SetDutyCycle(kStartCycle);
}

uint SingleFanSpeedManager::Next() noexcept {
    const auto current_speed = speed_helper_.GetFanSpeedRpm();
    if (current_speed == 0) {
        // skip pid if we can't get fan speed.
        log_debug("skip.fan.pwm_gpio.%d.no.speed\n", int(pwm_.GetGpioPin()));
        pwm_.SetDutyCycle(kStartCycle);
        return 0;
    }

    if (current_speed >= target_rpm_ &&
        current_speed < (target_rpm_ + rpm_tolerance_)) {
        // skip pid if we already at target_rpm_.
        log_debug("skip.fan.pwm_gpio.%d.speed.%d\n", int(pwm_.GetGpioPin()),
                  int(current_speed));
        return current_speed;
    }

    auto cycle = pid_.calculate(target_rpm_, current_speed);
    pwm_.SetDutyCycle(cycle);
    return current_speed;
}

FanSpeedManagerWithSelector::FanSpeedManagerWithSelector(uint pwm_gpio_pin1,
                                                         uint pwm_gpio_pin2,
                                                         uint pwm_gpio_pin3,
                                                         uint pwm_gpio_pin4)
    : pwm_array_{
          PwmHelper(pwm_gpio_pin1, kPwmFreqKhz),
          PwmHelper(pwm_gpio_pin2, kPwmFreqKhz),
          PwmHelper(pwm_gpio_pin3, kPwmFreqKhz),
          PwmHelper(pwm_gpio_pin4, kPwmFreqKhz),
      },
      pid_array_{
          Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD),
          Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD),
          Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD),
          Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD),
      },
      speed_helper_(kFanSpeedPin),
      selector_(kFanSelPin3, kFanSelPin2, kFanSelPin1, kFanSelPin0) {
    selector_.SelectFan(kFanIndexArray[current_fan_]);

    for (auto &pwm : pwm_array_) {
        pwm.SetDutyCycle(kStartCycle);
    }
}

void FanSpeedManagerWithSelector::Next() noexcept {
    const auto current_speed = speed_helper_.GetFanSpeedRpm();
    const auto current_fan = current_fan_;
    current_fan_ = (current_fan_ + 1) % kFanCount;
    selector_.SelectFan(kFanIndexArray[current_fan_]);
    speed_helper_.Reset();
    max_fan_speed_ = std::max(max_fan_speed_, current_speed);
    log_debug("current.fan.%d.speed.%drpm.max.%drpm\n", int(current_fan),
              int(current_speed), int(max_fan_speed_));

    if ((current_fan % kFanCountPerGroup) < (kFanCountPerGroup - 1)) {
        // not the last fan of this pwm group, then return.
        return;
    }

    const auto max_fan_speed = max_fan_speed_;
    max_fan_speed_ = 0;
    if (max_fan_speed >= kTargetRpm && max_fan_speed <= kMaxTargetRpm) {
        // skip pid if we already at specified rpm.
        log_debug("skip.fan.%d\n", int(current_fan));
        return;
    }

    const auto pwm_index = current_fan / kFanCountPerGroup;
    auto cycle = pid_array_[pwm_index].calculate(kTargetRpm, max_fan_speed);
    pwm_array_[pwm_index].SetDutyCycle(cycle);
    log_debug("set.pwm.%d.cycle.%d\n", int(pwm_index), int(cycle));
}

}  // namespace utility
