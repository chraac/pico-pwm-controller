
#include <hardware/timer.h>
#include <pico/runtime_init.h>
#include <pico/stdlib.h>

#include <algorithm>
#include <cmath>
#include <iterator>

#include "ema_smoother.hh"
#include "fan_speed_manager.hh"
#include "ina226_helper.hh"
#include "integ_test.hh"
#include "lcd_helper.hh"
#include "logger.hh"
#include "rgb_led_helper.hh"
#include "temp_helper.hh"

using namespace utility;

namespace {

// pcie board polls at 500 ms; the emulator build shortens this
// (exec/integ_test.hh)
constexpr const uint kBoardPoolIntervalMs = kIntegPoolIntervalMs;

constexpr const uint kPwm0Pin = 13;
constexpr const uint kPwm1Pin = 11;

constexpr const uint kFanSpd0Pin = 12;
constexpr const uint kFanSpd1Pin = 10;

constexpr const uint kWs2812LedPin = 16;
// power-indicator dimmer: linear in current, ~half as bright to the eye
constexpr const uint8_t kWs2812Brightness = 0x10;

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
constexpr CurvePoint kPwrToPwmCurveFanType260WSunonPFC0321B3[]{
    {30, 1500},  {60, 2000},  {90, 2600},  {120, 3100}, {150, 3600},
    {180, 4400}, {210, 5500}, {240, 6800}, {250, 8100}, {260, 10000},
};

constexpr CurvePoint kPwrToPwmCurveFanType200WSunonPFC0321B3[]{
    {40, 1500},  {58, 1728},  {83, 2266},  {102, 2849}, {118, 3431},
    {142, 4597}, {160, 5500}, {177, 6613}, {192, 7890}, {200, 10000},
};

constexpr CurvePoint kPwrToPwmCurveFanType170WSunonPFC0321B3[]{
    {40, 1500},  {58, 1728},  {84, 2221},  {103, 2602}, {122, 3252},
    {137, 4171}, {149, 5358}, {157, 6501}, {164, 8070}, {170, 10000},
};

constexpr CurvePoint kPwrToPwmCurveFanType170WSunonMFC0251V1[]{
    {45, 3500},  {70, 3678},  {100, 3900}, {120, 4200}, {133, 4600},
    {145, 5067}, {152, 5600}, {157, 6500}, {164, 8050}, {170, 10000},
};

constexpr CurvePoint kPwrToPwmCurveFanType100WSunonPFC0321B3[]{
    {5, 1500},  {10, 2000}, {30, 2600}, {40, 3100}, {50, 3600},
    {60, 4400}, {70, 5500}, {80, 6800}, {90, 8100}, {100, 10000},
};

constexpr CurvePoint kPwrToPwmCurveFanType213WW6800[]{
    {45, 1500},  {70, 1600},  {100, 1900}, {118, 2356},
    {131, 2916}, {140, 3633}, {148, 4664}, {155, 6031},
};

constexpr FanCurves kFanType0Curves{kDefaultTempToPwmCurve,
                                    kPwrToPwmCurveFanType170WSunonMFC0251V1,
                                    kDefaultTempToRpmCurve};
constexpr FanCurves kFanType1Curves{kDefaultTempToPwmCurve,
                                    kPwrToPwmCurveFanType213WW6800,
                                    kDefaultTempToRpmCurve};

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
    // clocks first: matches runtime_init order, and stdio (uart or usb)
    // needs the final clock tree for its divisor setup
    clocks_init();
#ifdef UART_STDIO
    // stdio over UART: deterministic to capture in the emulator (and fixes
    // linking with USB_STDIO=false, when pico_stdio_usb is not linked)
    stdio_uart_init();
#else
    stdio_usb_init();
#endif

    log_info("main.init.finished\n");

    Ws2812Helper rgb_led{kWs2812LedPin};
    rgb_led.SetBrightness(kWs2812Brightness);
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
    [[maybe_unused]] int test_iter = 0;  // log_integ_test only, emu builds
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
        [[maybe_unused]] uint loop_rpm[std::size(managers)] =
            {};  // log_integ_test
        for (size_t i = 0; i < std::size(managers); ++i) {
            auto &fan_manager = managers[i];
            auto rpm = fan_manager.Next(smoothed_watts);
            loop_rpm[i] = rpm;
            log_debug("fan.pwm_gpio.%d.rpm.%d\n",
                      int(fan_manager.GetPwmGpioPin()), int(rpm));
            auto &draw_item = drawer_items[i];
            draw_item.rpm = rpm;
            draw_item.target = draw_item.mode != FanControlMode::kTempToRpm
                                   ? (fan_manager.GetPwmCycle() / 100)
                                   : fan_manager.GetTargetRpm();
        }

        // int() casts: uint32_t is `unsigned long` on arm-none-eabi, so %u
        // alone trips -Wformat (same convention as the log_debug line above)
        log_integ_test(
            "TEST: it=%d w=%.3f sw=%.3f pwm0=%d rpm0=%d pwm1=%d rpm1=%d\n",
            test_iter++, watts, smoothed_watts, int(managers[0].GetPwmCycle()),
            int(loop_rpm[0]), int(managers[1].GetPwmCycle()), int(loop_rpm[1]));

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
