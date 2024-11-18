
#include <hardware/timer.h>
#include <pico/runtime_init.h>
#include <pico/stdlib.h>

#include <iterator>

#include "adc_helper.hh"
#include "fan_speed_manager.hh"
#include "lcd_helper.hh"
#include "logger.hh"
#include "rgb_led_helper.hh"
#include "temp_helper.hh"

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

constexpr const uint kDefaultTempPin = 26;

}  // namespace

int main() {
    stdio_usb_init();
    clocks_init();

    log_info("main.init.finished\n");

    SingleFanSpeedManager managers[] = {
        SingleFanSpeedManager{kPwm0Pin, kFanSpd0Pin, true},
        SingleFanSpeedManager{kPwm1Pin, kFanSpd1Pin, true},
        SingleFanSpeedManager{kPwm2Pin, kFanSpd2Pin, false},
        SingleFanSpeedManager{kPwm3Pin, kFanSpd3Pin, false},
    };

    for (auto &fan_manager : managers) {
        fan_manager.SetTargetRpm(kDefaultTargetRpm);
    }

    AdcHelper temp_adc{kDefaultTempPin};
    RgbLedHelper rgb_led{kRedPin, kGreenPin, kBluePin};

    using LiteLcdDrawer = XiaoRp2040LcdDrawer<std::size(managers)>;
    LiteLcdDrawer lcd_drawer{kDefaultLcdWidth, kDefaultLcdHeight};
    lcd_drawer.SetContrast(kDefaultLcdContrast);
    LiteLcdDrawer::TempItemArray drawer_items = {
        LiteLcdDrawer::TempItem{true},
        LiteLcdDrawer::TempItem{true},
        LiteLcdDrawer::TempItem{false},
        LiteLcdDrawer::TempItem{false},
    };

    log_info("main.entering.loop\n");
    for (auto next_interval = utility::kPoolIntervalMs;;
         sleep_ms(next_interval)) {
        const auto start_us = time_us_64();

        const auto adc_read = temp_adc.Read();
        const auto resist = GetResistantValue(adc_read, temp_adc.GetMax());
        const auto temp = GetTemperature(resist, utility::kNtc100k3950);

        static_assert(std::size(managers) == 4);
        for (size_t i = 0; i < std::size(managers); ++i) {
            auto &fan_manager = managers[i];
            auto rpm = fan_manager.Next(temp);
            log_info("fan.pwm_gpio.%d.rpm.%d\n",
                     int(fan_manager.GetPwmGpioPin()), int(rpm));
            auto &draw_item = drawer_items[i];
            draw_item.rpm = rpm;
            draw_item.target = draw_item.is_cycle
                                   ? (fan_manager.GetPwmCycle() / 100)
                                   : kDefaultTargetRpm;
        }

        rgb_led.Next();
        lcd_drawer.DrawTempAndItems(temp, drawer_items);

        log_debug("current adc: %d, r: %dohm, temp: %.2fdeg\n", int(adc_read),
                  int(resist), temp);

        auto consumed_time_ms = (time_us_64() - start_us) / 1000;
        log_debug("current iteration time cost: %dms\n", int(consumed_time_ms));
        next_interval =
            utility::kPoolIntervalMs -
            std::min<uint>(consumed_time_ms, utility::kPoolIntervalMs);
    }

    return 0;
}
