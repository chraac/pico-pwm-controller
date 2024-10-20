
#include <hardware/timer.h>
#include <pico/runtime_init.h>
#include <pico/stdlib.h>

#include <iterator>

#include "fan_speed_manager.hh"
#include "logger.hh"

using namespace utility;

namespace {
constexpr const uint kPwm1Pin = 0;
constexpr const uint kPwm2Pin = 7;
constexpr const uint kPwm3Pin = 27;
constexpr const uint kPwm4Pin = 17;
}  // namespace

int main() {
    stdio_usb_init();
    clocks_init();

    log_debug("main.init.finished\n");

    auto fan_manager =
        FanSpeedManagerWithSelector(kPwm1Pin, kPwm2Pin, kPwm3Pin, kPwm4Pin);

    log_debug("main.entering.loop\n");
    for (auto next_interval = utility::kPoolIntervalMs;;
         sleep_ms(next_interval)) {
        const auto start_us = time_us_64();
        fan_manager.Next();
        auto consumed_time_ms = (time_us_64() - start_us) / 1000;
        log_debug("current iteration time cost: %dms\n", int(consumed_time_ms));
        next_interval =
            utility::kPoolIntervalMs -
            std::min<uint>(consumed_time_ms, utility::kPoolIntervalMs);
    }

    return 0;
}
