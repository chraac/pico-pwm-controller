
#include <hardware/timer.h>
#include <pico/runtime_init.h>
#include <pico/stdlib.h>

#include <algorithm>
#include <cmath>
#include <iterator>

#include "ema_smoother.hh"
#include "fan_speed_manager.hh"
#include "ina226_helper.hh"
#include "lcd_helper.hh"
#include "logger.hh"
#include "rgb_led_helper.hh"
#include "temp_helper.hh"

using namespace utility;

namespace {

constexpr const uint kBoardPoolIntervalMs = 500;

constexpr const uint kPwm0Pin = 13;
constexpr const uint kPwm1Pin = 11;

constexpr const uint kFanSpd0Pin = 12;
constexpr const uint kFanSpd1Pin = 10;

constexpr const uint kWs2812LedPin = 16;

constexpr const uint kDefaultTargetRpm = 1900;
constexpr const uint16_t kDefaultLcdWidth = 128;
constexpr const uint16_t kDefaultLcdHeight = 32;
constexpr const uint8_t kDefaultLcdContrast = 0x3F;

// Current_LSB = kIna226MaxCurrentAmps / 32768 for the on-chip current/power
// registers, see docs/ina226_i2c.md §7b. Must satisfy:
// kIna226MaxCurrentAmps * kShuntOhms (1 mΩ) <= 81.92 mV -> <= 81.92 A
constexpr const float kIna226MaxCurrentAmps = 20.0f;
constexpr const uint kI2cDefaultSclPin = 15;
constexpr const uint kI2cDefaultSdaPin = 14;

// fan power draw in watts (INA226) -> pwm, one table per fan type; pick the
// set matching the attached fan below
constexpr CurvePoint kPwrToPwmCurveFanType0[]{
    {5, 1500},  {10, 2000}, {30, 2600},  {50, 3100},  {70, 3600},
    {80, 4400}, {90, 5500}, {110, 6800}, {120, 8100}, {145, 10000},
};

constexpr CurvePoint kPwrToPwmCurveFanType1[]{
    {5, 1500},  {10, 2000}, {30, 2600}, {40, 3100}, {50, 3600},
    {60, 4400}, {70, 5500}, {80, 6800}, {90, 8100}, {100, 10000},
};

constexpr FanCurves kFanType0Curves{
    kDefaultTempToPwmCurve, kPwrToPwmCurveFanType0, kDefaultTempToRpmCurve};
constexpr FanCurves kFanType1Curves{
    kDefaultTempToPwmCurve, kPwrToPwmCurveFanType1, kDefaultTempToRpmCurve};

// led color tracks power between these bounds, see SetPwrLedColor() below
constexpr float kLedGreenW = 20.0f;
constexpr float kLedRedW = 90.0f;

// blends green -> yellow -> red as watts rises from low_w to high_w; the
// sqrt curves keep perceived brightness flat across the blend (WS2812 duty
// cycle is linear in brightness)
void SetPwrLedColor(Ws2812Helper &led, const float watts, const float low_w,
                    const float high_w) noexcept {
    const float t = std::clamp((watts - low_w) / (high_w - low_w), 0.0f, 1.0f);
    const auto red = static_cast<uint8_t>(255.0f * std::sqrt(t));
    const auto green = static_cast<uint8_t>(255.0f * std::sqrt(1.0f - t));
    log_debug(
        "SetPwrLedColor: watts=%.2f, low_w=%.2f, high_w=%.2f, red=%u, "
        "green=%u\n",
        watts, low_w, high_w, red, green);
    led.SetRgb(red, green, 0);
}

}  // namespace

int main() {
    stdio_usb_init();
    clocks_init();

    log_info("main.init.finished\n");

    Ws2812Helper rgb_led{kWs2812LedPin};
    // fast rise (~2s to settle) tracks load steps, slow fall (~20s) rides
    // out burst dips and matches heatsink cooldown; ratios assume the
    // kBoardPoolIntervalMs update period
    EmaSmoother pwr_smoother(/*up_rate=*/0.4f, /*down_rate=*/0.06f,
                             /*idle_val=*/5.0f);
    Ina226Device ina226{i2c1, kI2cDefaultSclPin, kI2cDefaultSdaPin};
    if (!ina226.Probe()) {
        log_info("ina226.probe.failed\n");
    }
    ina226.Configure();
    // on-chip current/power regs stay 0 until the calibration register is set
    ina226.SetCalibration(kIna226MaxCurrentAmps);

    using Mode = SingleFanSpeedManager::ControlMode;
    SingleFanSpeedManager managers[] = {
        SingleFanSpeedManager{kPwm0Pin, kFanSpd0Pin, Mode::kPwrToPwm,
                              kFanType0Curves},
        SingleFanSpeedManager{kPwm1Pin, kFanSpd1Pin, Mode::kPwrToPwm,
                              kFanType1Curves},
    };

    using LcdDrawer = CustomLcdDrawer0<1, 0, std::size(managers)>;
    LcdDrawer lcd_drawer{kDefaultLcdWidth, kDefaultLcdHeight};
    lcd_drawer.SetContrast(kDefaultLcdContrast);
    LcdDrawer::TempItemArray drawer_items = {
        LcdDrawer::TempItem{managers[0].GetControlMode()},
        LcdDrawer::TempItem{managers[1].GetControlMode()},
    };

    bool led_off = false;
    log_info("main.entering.loop\n");
    for (auto next_interval = kBoardPoolIntervalMs;; sleep_ms(next_interval)) {
        const auto start_us = time_us_64();

        const auto amps = ina226.GetCurrentAmps();
        const auto watts = ina226.GetPowerWatts();
        const auto volts = ina226.GetBusVolts();
        const auto smoothed_watts = pwr_smoother.Update(watts);

        log_info(
            "current amps: %.3fA, power: %.3fW, smoothed power: %.3fW, volts: "
            "%.3fV\n",
            amps, watts, smoothed_watts, volts);

        static_assert(std::size(managers) == 2);
        for (size_t i = 0; i < std::size(managers); ++i) {
            auto &fan_manager = managers[i];
            auto rpm = fan_manager.Next(smoothed_watts);
            log_debug("fan.pwm_gpio.%d.rpm.%d\n",
                      int(fan_manager.GetPwmGpioPin()), int(rpm));
            auto &draw_item = drawer_items[i];
            draw_item.rpm = rpm;
            draw_item.target = draw_item.mode != FanControlMode::kTempToRpm
                                   ? (fan_manager.GetPwmCycle() / 100)
                                   : fan_manager.GetTargetRpm();
        }

        if (led_off) {
            rgb_led.Off();
        } else {
            SetPwrLedColor(rgb_led, watts, kLedGreenW, kLedRedW);
        }

        led_off = !led_off;

        lcd_drawer.DrawPwrAndItems(volts, watts, drawer_items);

        auto consumed_time_ms = (time_us_64() - start_us) / 1000;
        log_debug("current iteration time cost: %dms\n", int(consumed_time_ms));
        next_interval = kBoardPoolIntervalMs -
                        std::min<uint>(consumed_time_ms, kBoardPoolIntervalMs);
    }

    return 0;
}
