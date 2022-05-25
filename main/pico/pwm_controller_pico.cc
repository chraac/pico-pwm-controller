#include <hardware/clocks.h>
#include <hardware/timer.h>
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
constexpr uint32_t kPwmFreqKhz = 25;
constexpr uint32_t kPwm1Pin = 0;
constexpr uint32_t kPwm2Pin = 7;
constexpr uint32_t kPwm3Pin = 27;
constexpr uint32_t kPwm4Pin = 17;
constexpr uint8_t kPwmPinCount = 4;
constexpr uint32_t kButton1Pin = 14;
constexpr uint32_t kButton2Pin = 15;
constexpr uint32_t kButton3Pin = 16;
constexpr uint32_t kFanSpeedSda = 8;
constexpr uint32_t kFanSpeedScl = 9;
constexpr uint32_t kFanSpeedRstn = 11;
constexpr uint32_t kFanSpeedIntr = 13;
constexpr uint32_t kFanSpeedAd0 = 10;
constexpr uint32_t kFanSpeedAd1 = 7;
constexpr uint8_t kFanIndexArray[] = {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13};
constexpr uint8_t kFanCount = std::size(kFanIndexArray);
constexpr uint8_t kFanCountPerGroup = kFanCount / kPwmPinCount;
constexpr uint32_t kPoolIntervalMs = 400;

// See also:
// https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf
constexpr uint32_t kFanSpeedStepRpm = 30 * 1000 / kPoolIntervalMs;
constexpr uint32_t kTargetRpm = 1900;
constexpr uint32_t kMaxTargetRpm = kTargetRpm + kFanSpeedStepRpm;
constexpr auto kStartCycle = 500;
constexpr auto kP = .5F;
constexpr auto kI = .3F;
constexpr auto kD = .02F;

class FanSpeedManager {
public:
    FanSpeedManager() noexcept {
        for (auto &pwm : pwm_array_) {
            pwm.SetDutyCycle(kStartCycle);
        }
    }

    void next() noexcept {
        uint32_t max_fan_speed = 0;
        for (size_t i = 0; i < kFanCount; i++) {
            const auto fan_pin = kFanIndexArray[i];
            const auto current_speed = speed_helper_.GetFanSpeedRpm(fan_pin);
            max_fan_speed = std::max(max_fan_speed, current_speed);
            log_debug("current.fan.%d.speed.%drpm.max.%drpm\n", int(i),
                      int(current_speed), int(max_fan_speed));
            if ((i % kFanCountPerGroup) < (kFanCountPerGroup - 1)) {
                // not the last fan of this pwm group, then return.
                continue;
            }

            const auto pwm_index = i / kFanCountPerGroup;
            if (max_fan_speed >= kTargetRpm && max_fan_speed <= kMaxTargetRpm) {
                // skip pid if we already at specified rpm.
                log_debug("skip.pwm.%d.max.speed.%d\n", int(pwm_index),
                          int(max_fan_speed));
                max_fan_speed = 0;
                continue;
            }

            auto cycle =
                pid_array_[pwm_index].calculate(kTargetRpm, max_fan_speed);
            pwm_array_[pwm_index].SetDutyCycle(cycle);
            log_debug("set.pwm.%d.cycle.%d\n", int(pwm_index), int(cycle));
            max_fan_speed = 0;
            speed_helper_.Reset(fan_pin);
        }
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
    Aw9523FanSpeedHelper speed_helper_ =
        Aw9523FanSpeedHelper(kFanSpeedScl, kFanSpeedSda, kFanSpeedAd0,
                             kFanSpeedAd1, kFanSpeedIntr, kFanSpeedRstn);

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
    for (auto next_interval = kPoolIntervalMs;; sleep_ms(next_interval)) {
        const auto start_us = time_us_64();
        fan_manager.next();
        auto consumed_time_ms = (time_us_64() - start_us) / 1000;
        log_debug("current iteration time cost: %dms\n", int(consumed_time_ms));
        next_interval = kPoolIntervalMs -
                        std::min<uint32_t>(consumed_time_ms, kPoolIntervalMs);
    }

    return 0;
}
