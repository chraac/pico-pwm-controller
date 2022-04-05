
#include <pico/stdlib.h>

#include <algorithm>

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
constexpr uint kButton1Pin = 14;
constexpr uint kButton2Pin = 15;
constexpr uint kButton3Pin = 16;
constexpr uint kRpmPin1 = 1;
constexpr uint kRpmPin2 = 2;
constexpr uint kRpmPin3 = 3;
constexpr uint kRpmPin4 = 4;
constexpr uint kRpmPin5 = 5;
constexpr uint kRpmPin6 = 6;
constexpr uint kRpmPin7 = 26;
constexpr uint kRpmPin8 = 22;
constexpr uint kRpmPin9 = 21;
constexpr uint kRpmPin10 = 20;
constexpr uint kRpmPin11 = 19;
constexpr uint kRpmPin12 = 18;
constexpr auto kTargetRpm = 2000;
constexpr auto kMaxTargetRpm =
    103 * kTargetRpm / 100;  // max tolerance: +3% fan speed
constexpr auto kStartCycle = 500;
constexpr auto kP = .5F;
constexpr auto kI = .3F;
constexpr auto kD = .02F;

class FanSpeedManager {
   public:
    FanSpeedManager(uint gpio_pwm, uint gpio_speed1, uint gpio_speed2,
                    uint gpio_speed3) noexcept
        : pwm_(gpio_pwm, kPwmFreqKhz),
          speed1_(gpio_speed1),
          speed2_(gpio_speed2),
          speed3_(gpio_speed3) {
        pwm_.SetDutyCycle(kStartCycle);
    }

    bool next() noexcept {
        auto speed1 = speed1_.GetFanSpeedRpm();
        auto speed2 = speed2_.GetFanSpeedRpm();
        auto speed3 = speed3_.GetFanSpeedRpm();
        log_debug("next.gpio.%d.fanspeed.%drpm\n", int(speed1_.GetGpioPin()),
                  int(speed1));
        log_debug("next.gpio.%d.fanspeed.%drpm\n", int(speed2_.GetGpioPin()),
                  int(speed2));
        log_debug("next.gpio.%d.fanspeed.%drpm\n", int(speed3_.GetGpioPin()),
                  int(speed3));
        auto max_speed = std::max(speed1, std::max(speed2, speed3));
        if (max_speed >= kTargetRpm && max_speed <= kMaxTargetRpm) {
            log_debug("next.gpio.%d.skip\n", int(pwm_.GetGpioPin()));
            speed1_.Reset();
            speed2_.Reset();
            speed3_.Reset();
            return false;
        }

        auto cycle = pid_.calculate(kTargetRpm, max_speed);
        pwm_.SetDutyCycle(cycle);
        speed1_.Reset();
        speed2_.Reset();
        speed3_.Reset();
        log_debug("next.gpio.%d.cycle.%d\n", int(pwm_.GetGpioPin()),
                  int(cycle));
        return true;
    }

   private:
    PwmHelper pwm_;
    FanSpeedHelper speed1_;
    FanSpeedHelper speed2_;
    FanSpeedHelper speed3_;
    auto pid_ = Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD);

    DISALLOW_COPY(FanSpeedManager);
    DISALLOW_MOVE(FanSpeedManager);
};

}  // namespace

int main() {
    stdio_usb_init();
    clocks_init();

    log_debug("main.init.finished\n");

    auto fan_manager2 = FanSpeedManager(kPwm1Pin, kRpmPin4, kRpmPin5, kRpmPin6);

    log_debug("main.entering.loop\n");
    for (;; sleep_ms(200)) {
        fan_manager2.next();
    }

    return 0;
}
