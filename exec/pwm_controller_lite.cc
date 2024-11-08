
#include <hardware/timer.h>
#include <pico/runtime_init.h>
#include <pico/stdlib.h>

#include <iterator>

#include "fan_speed_manager.hh"
#include "lcd_helper.hh"
#include "logger.hh"
#include "rgb_led_helper.hh"

using namespace utility;

namespace {

#if PICO_LITE_BOARD_VERSION == 90
constexpr const uint kPwm0Pin = 3;
constexpr const uint kPwm1Pin = 2;
constexpr const uint kPwm2Pin = 26;
constexpr const uint kPwm3Pin = 28;

constexpr const uint kFanSpd0Pin = 4;
constexpr const uint kFanSpd1Pin = 1;
constexpr const uint kFanSpd2Pin = 27;
constexpr const uint kFanSpd3Pin = 29;
#else
constexpr const uint kPwm0Pin = 3;
constexpr const uint kPwm1Pin = 2;
constexpr const uint kPwm2Pin = 27;
constexpr const uint kPwm3Pin = 29;

constexpr const uint kFanSpd0Pin = 4;
constexpr const uint kFanSpd1Pin = 1;
constexpr const uint kFanSpd2Pin = 28;
constexpr const uint kFanSpd3Pin = 0;
#endif

constexpr const uint kRedPin = 17;
constexpr const uint kGreenPin = 16;
constexpr const uint kBluePin = 25;

constexpr const uint kDefaultTargetRpm = 1900;
constexpr const uint16_t kDefaultLcdWidth = 128;
constexpr const uint16_t kDefaultLcdHeight = 64;
constexpr const uint8_t kDefaultLcdContrast = 0x3F;

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

    for (auto &fan_manager : managers) {
        fan_manager.SetTargetRpm(kDefaultTargetRpm);
    }

    RgbLedHelper rgb_led{kRedPin, kGreenPin, kBluePin};
    XiaoRp2040LcdDrawer lcd_drawer{kDefaultLcdWidth, kDefaultLcdHeight};
    lcd_drawer.SetContrast(kDefaultLcdContrast);

    log_info("main.entering.loop\n");
    for (auto next_interval = utility::kPoolIntervalMs;;
         sleep_ms(next_interval)) {
        const auto start_us = time_us_64();
        uint speeds[std::size(managers)] = {};
        for (size_t i = 0; i < std::size(managers); ++i) {
            auto &fan_manager = managers[i];
            auto rpm = fan_manager.Next();
            log_info("fan.pwm_gpio.%d.rpm.%d\n",
                     int(fan_manager.GetPwmGpioPin()), int(rpm));
            speeds[i] = rpm;
        }
        rgb_led.Next();
        static_assert(std::size(managers) == 4);
        lcd_drawer.DrawRpm(speeds[0], speeds[1], speeds[2], speeds[3],
                           kDefaultTargetRpm);
        auto consumed_time_ms = (time_us_64() - start_us) / 1000;
        log_debug("current iteration time cost: %dms\n", int(consumed_time_ms));
        next_interval =
            utility::kPoolIntervalMs -
            std::min<uint>(consumed_time_ms, utility::kPoolIntervalMs);
    }

    return 0;
}
