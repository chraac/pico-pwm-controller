
#include <hardware/timer.h>
#include <pico/runtime_init.h>
#include <pico/stdlib.h>

#include <iterator>

#include "fan_speed_manager.hh"
#include "logger.hh"

using namespace utility;

namespace {

constexpr const uint kPwm0Pin = 3;
constexpr const uint kPwm1Pin = 2;
constexpr const uint kPwm2Pin = 26;
constexpr const uint kPwm3Pin = 28;

constexpr const uint kFanSpd0Pin = 4;
constexpr const uint kFanSpd1Pin = 1;
constexpr const uint kFanSpd2Pin = 27;
constexpr const uint kFanSpd3Pin = 29;

}  // namespace

int main() {
    stdio_usb_init();
    clocks_init();

    log_info("main.init.finished\n");

    SingleFanSpeedManager managers[] = {
        SingleFanSpeedManager{kPwm0Pin, kFanSpd0Pin},
        SingleFanSpeedManager{kPwm1Pin, kFanSpd1Pin},
        SingleFanSpeedManager{kPwm2Pin, kFanSpd2Pin},
        SingleFanSpeedManager{kPwm3Pin, kFanSpd3Pin},
    };

    log_info("main.entering.loop\n");
    for (auto next_interval = utility::kPoolIntervalMs;;
         sleep_ms(next_interval)) {
        const auto start_us = time_us_64();
        for (auto &fan_manager : managers) {
            auto rpm = fan_manager.Next();
            log_info("fan.pwm_gpio.%d.rpm.%d\n",
                     int(fan_manager.GetPwmGpioPin()), int(rpm));
        }
        auto consumed_time_ms = (time_us_64() - start_us) / 1000;
        log_debug("current iteration time cost: %dms\n", int(consumed_time_ms));
        next_interval =
            utility::kPoolIntervalMs -
            std::min<uint>(consumed_time_ms, utility::kPoolIntervalMs);
    }

    return 0;
}
