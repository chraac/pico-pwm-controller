
#include <hardware/timer.h>
#include <pico/runtime_init.h>
#include <pico/stdlib.h>

#include <iterator>

#include "adc_helper.hh"
#include "fan_speed_manager.hh"
#include "ina226_helper.hh"
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

// Current_LSB = kIna226MaxCurrentAmps / 32768 for the on-chip current/power
// registers, see docs/ina226_i2c.md §7b. Must satisfy:
// kIna226MaxCurrentAmps * kShuntOhms (1 mΩ) <= 81.92 mV -> <= 81.92 A
constexpr const float kIna226MaxCurrentAmps = 20.0f;

}  // namespace

int main() {
    stdio_usb_init();
    clocks_init();

    log_info("main.init.finished\n");

    RgbLedHelper rgb_led{kRedPin, kGreenPin, kBluePin};
    Ina226Device ina226{i2c1, Ina226Device::kI2cDefaultSclPin, Ina226Device::kI2cDefaultSdaPin};
    if (!ina226.Probe()) {
        log_info("ina226.probe.failed\n");
    }
    ina226.Configure();
    // on-chip current/power regs stay 0 until the calibration register is set
    ina226.SetCalibration(kIna226MaxCurrentAmps);

    log_info("main.entering.loop\n");
    for (auto next_interval = utility::kPoolIntervalMs;;
         sleep_ms(next_interval)) {
        const auto start_us = time_us_64();

        const auto amps = ina226.GetCurrentAmps();
        const auto watts = ina226.GetPowerWatts();
        const auto volts = ina226.GetBusVolts();
        rgb_led.Next();

        log_info("current amps: %.3fA, power watts: %.3fW, bus volts: %.3fV\n", amps, watts, volts);

        auto consumed_time_ms = (time_us_64() - start_us) / 1000;
        log_debug("current iteration time cost: %dms\n", int(consumed_time_ms));
        next_interval =
            utility::kPoolIntervalMs -
            std::min<uint>(consumed_time_ms, utility::kPoolIntervalMs);
    }

    return 0;
}
