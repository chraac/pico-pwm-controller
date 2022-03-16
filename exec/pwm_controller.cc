
#include "pico/stdlib.h"
#include "pwm_helper.hh"
#include "button_helper.hh"
#include "pid.hh"
#include "fan_speed_helper.hh"
#include "logger.hh"

using namespace utility;

namespace
{
    constexpr uint kPwmFreqKhz = 25;
    constexpr uint kPwm1Pin = 0;
    constexpr uint kPwm2Pin = 7;
    constexpr uint kPwm3Pin = 27;
    constexpr uint kPwm4Pin = 17;
    constexpr uint kButton1Pin = 13;
    constexpr uint kButton2Pin = 14;
    constexpr uint kButton3Pin = 15;
    constexpr uint kButton4Pin = 16;
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
    constexpr auto kP = 2.F;
    constexpr auto kI = .001F;
    constexpr auto kD = 15.F;
}

int main()
{
    stdio_usb_init();
    clocks_init();

    log_debug("main.init.finished");

    auto pwm1 = PwmHelper(kPwm1Pin, kPwmFreqKhz);
    auto pwm2 = PwmHelper(kPwm2Pin, kPwmFreqKhz);
    auto pwm3 = PwmHelper(kPwm3Pin, kPwmFreqKhz);
    auto pwm4 = PwmHelper(kPwm4Pin, kPwmFreqKhz);

    uint32_t cycle2 = 50;
    uint32_t cycle3 = 75;
    uint32_t cycle4 = 100;
    pwm1.SetDutyCycle(100);
    pwm2.SetDutyCycle(cycle2);
    pwm3.SetDutyCycle(cycle3);
    pwm4.SetDutyCycle(cycle4);

    auto pid1 = Pid(20, 100, 1, kP, kI, kD);

    auto fan_speed6 = FanSpeedHelper(kRpmPin6); 
    
    log_debug("main.entering.loop");
    while (true)
    {
        auto rpm6 = fan_speed6.GetFanSpeedRpm();
        log_debug("main.fanspeed6.%d.rpm", rpm6);
        auto cycle1 = pid1.calculate(kTargetRpm, rpm6);
        log_debug("main.cycle1.%d", cycle1);
        pwm1.SetDutyCycle(cycle1);
        fan_speed6.Reset();
        sleep_ms(1000);
    }

    return 0;
}
