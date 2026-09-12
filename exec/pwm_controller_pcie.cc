
#include <hardware/timer.h>
#include <pico/runtime_init.h>
#include <pico/stdlib.h>

#include <iterator>

#include "fan_speed_manager.hh"
#include "ina226_helper.hh"
#include "lcd_helper.hh"
#include "logger.hh"
#include "rgb_led_helper.hh"
#include "temp_helper.hh"

using namespace utility;

namespace {

constexpr const uint kPwm0Pin = 13;
constexpr const uint kPwm1Pin = 11;

constexpr const uint kFanSpd0Pin = 12;
constexpr const uint kFanSpd1Pin = 10;

constexpr const uint kWs2812LedPin = 16;

constexpr const uint kDefaultTargetRpm = 1900;
constexpr const uint16_t kDefaultLcdWidth = 128;
constexpr const uint16_t kDefaultLcdHeight = 64;
constexpr const uint8_t kDefaultLcdContrast = 0x3F;

// Current_LSB = kIna226MaxCurrentAmps / 32768 for the on-chip current/power
// registers, see docs/ina226_i2c.md §7b. Must satisfy:
// kIna226MaxCurrentAmps * kShuntOhms (1 mΩ) <= 81.92 mV -> <= 81.92 A
constexpr const float kIna226MaxCurrentAmps = 20.0f;
constexpr const uint kI2cDefaultSclPin = 15;
constexpr const uint kI2cDefaultSdaPin = 14;

}  // namespace

int main() {
    stdio_usb_init();
    clocks_init();

    log_info("main.init.finished\n");

    Ws2812Helper rgb_led{kWs2812LedPin};
    Ina226Device ina226{i2c1, kI2cDefaultSclPin, kI2cDefaultSdaPin};
    if (!ina226.Probe()) {
        log_info("ina226.probe.failed\n");
    }
    ina226.Configure();
    // on-chip current/power regs stay 0 until the calibration register is set
    ina226.SetCalibration(kIna226MaxCurrentAmps);

    using Mode = SingleFanSpeedManager::ControlMode;
    SingleFanSpeedManager managers[] = {
        SingleFanSpeedManager{kPwm0Pin, kFanSpd0Pin, Mode::kPwrToPwm},
        SingleFanSpeedManager{kPwm1Pin, kFanSpd1Pin, Mode::kPwrToPwm},
    };

    using LcdDrawer = CustomLcdDrawer<15, 14, std::size(managers)>;
    LcdDrawer lcd_drawer{kDefaultLcdWidth, kDefaultLcdHeight};
    lcd_drawer.SetContrast(kDefaultLcdContrast);

    log_info("main.entering.loop\n");
    for (auto next_interval = utility::kPoolIntervalMs;;
         sleep_ms(next_interval)) {
        const auto start_us = time_us_64();

        const auto amps = ina226.GetCurrentAmps();
        const auto watts = ina226.GetPowerWatts();
        const auto volts = ina226.GetBusVolts();

        log_info("current amps: %.3fA, power: %.3fW, volts: %.3fV\n", amps,
                 watts, volts);

        static_assert(std::size(managers) == 2);
        for (size_t i = 0; i < std::size(managers); ++i) {
            auto &fan_manager = managers[i];
            auto rpm = fan_manager.Next(watts);
            log_info("fan.pwm_gpio.%d.rpm.%d\n",
                     int(fan_manager.GetPwmGpioPin()), int(rpm));
        }

        rgb_led.Next();

        auto consumed_time_ms = (time_us_64() - start_us) / 1000;
        log_debug("current iteration time cost: %dms\n", int(consumed_time_ms));
        next_interval =
            utility::kPoolIntervalMs -
            std::min<uint>(consumed_time_ms, utility::kPoolIntervalMs);
    }

    return 0;
}
