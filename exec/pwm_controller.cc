
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
    constexpr auto kStartCycle = 500;
    constexpr auto kP = .5F;
    constexpr auto kI = .3F;
    constexpr auto kD = .02F;
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

    pwm1.SetDutyCycle(kStartCycle);
    pwm2.SetDutyCycle(kStartCycle);
    pwm3.SetDutyCycle(kStartCycle);
    pwm4.SetDutyCycle(kStartCycle);

    auto pid1 = Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD);
    auto pid2 = Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD);
    auto pid3 = Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD);
    auto pid4 = Pid(kStartCycle, kDefaultCycleDenom, 1, kP, kI, kD);

    auto fan_speed3 = FanSpeedHelper(kRpmPin3); 
    auto fan_speed6 = FanSpeedHelper(kRpmPin6); 
    auto fan_speed9 = FanSpeedHelper(kRpmPin9); 
    auto fan_speed12 = FanSpeedHelper(kRpmPin12); 
    
    log_debug("main.entering.loop");
    while (true)
    {
        auto rpm6 = fan_speed6.GetFanSpeedRpm();
        log_debug("main.fanspeed6.%d.rpm", int(rpm6));
        auto cycle1 = pid1.calculate(kTargetRpm, rpm6);
        log_debug("main.cycle1.%d", int(cycle1));
        pwm1.SetDutyCycle(cycle1);

        fan_speed6.Reset();
        sleep_ms(200);
    }

    return 0;
}
