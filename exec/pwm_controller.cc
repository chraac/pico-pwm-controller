
#include <pico/stdlib.h>

#include <algorithm>
#include <iterator>

#include "button_helper.hh"
#include "fan_speed_helper.hh"
#include "logger.hh"
#include "pid.hh"
#include "pwm_helper.hh"

using namespace utility;

namespace {
constexpr uint kPwmFreqKhz = 25;
constexpr uint kPwm1Pin = 0;
constexpr uint kPwm2Pin = 7;
constexpr uint kPwm3Pin = 27;
constexpr uint kPwm4Pin = 17;
constexpr uint kPwmPinCount = 4;
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
constexpr auto kTargetRpm = 1900;
constexpr auto kMaxTargetRpm =
    105 * kTargetRpm / 100;  // max tolerance: +5% fan speed
constexpr auto kStartCycle = 500;
constexpr auto kP = .5F;
constexpr auto kI = .3F;
constexpr auto kD = .02F;

class FanSpeedManager {
public:
    FanSpeedManager() noexcept {
        selector_.SelectFan(kFanIndexArray[current_fan_]);
        for (auto &pwm : pwm_array_) {
            pwm.SetDutyCycle(kStartCycle);
        }
    }

    void next() noexcept {
        const auto current_speed = speed_helper_.GetFanSpeedRpm();
        const auto current_fan = current_fan_;
        current_fan_ = (current_fan_ + 1) % kFanCount;
        selector_.SelectFan(kFanIndexArray[current_fan_]);
        max_fan_speed_ = std::max(max_fan_speed_, current_speed);
        log_debug("current.fan.%d.speed.%drpm.max.%drpm\n", int(current_fan),
                  int(current_speed), int(max_fan_speed_));

        const pwm_index = current_fan % std::size(pwm_array_);
        if (pwm_index < (std::size(pwm_array_) - 1)) {
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

        auto cycle = pid_array_[pwm_index].calculate(kTargetRpm, max_fan_speed);
        pwm_array_[pwm_index].SetDutyCycle(cycle);
        log_debug("set.pwm.%d.cycle.%d\n", int(pwm_index), int(cycle));
        return;
    }

private:
    PwmHelper pwm_array_[kPwmPinCount] = {
        PwmHelper(kPwm1Pin, kPwmFreqKhz),
        PwmHelper(kPwm2Pin, kPwmFreqKhz),
        PwmHelper(kPwm3Pin, kPwmFreqKhz),
        PwmHelper(kPwm4Pin, kPwmFreqKhz),
    };
    Pid pid_array_[kPwmPinCount] = {
        Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD),
        Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD),
        Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD),
        Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD),

    };
    FanSpeedHelper speed_helper_ = FanSpeedHelper(kFanSpeedPin);
    FanSpeedSelector selector_ =
        FanSpeedSelector(kFanSelPin3, kFanSelPin2, kFanSelPin1, kFanSelPin0);
    uint8_t current_fan_ = 0;
    decltype(speed_helper_.GetFanSpeedRpm()) max_fan_speed_ = 0;

    DISALLOW_COPY(FanSpeedManager);
    DISALLOW_MOVE(FanSpeedManager);
};

}  // namespace

int main() {
    stdio_usb_init();
    clocks_init();

    log_debug("main.init.finished\n");

    auto fan_manager = FanSpeedManager();

    log_debug("main.entering.loop\n");
    for (;; sleep_ms(50)) {
        fan_manager.next();
    }

    return 0;
}
