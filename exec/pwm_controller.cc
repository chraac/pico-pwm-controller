
#include "pico/stdlib.h"
#include "pwm_helper.hh"
#include "button_helper.hh"

constexpr uint kPwmFreqKhz = 25;
constexpr uint kPwm1Pin = 0;
constexpr uint kPwm2Pin = 7;
constexpr uint kPwm3Pin = 27;
constexpr uint kPwm4Pin = 17;
constexpr uint kButton1Pin = 13;
constexpr uint kButton2Pin = 14;
constexpr uint kButton3Pin = 15;
constexpr uint kButton4Pin = 16;

using namespace utility;

int main()
{
    stdio_init_all();
    clocks_init();

    auto pwm1 = PwmHelper(kPwm1Pin, kPwmFreqKhz);
    auto pwm2 = PwmHelper(kPwm2Pin, kPwmFreqKhz);
    auto pwm3 = PwmHelper(kPwm3Pin, kPwmFreqKhz);
    auto pwm4 = PwmHelper(kPwm4Pin, kPwmFreqKhz);
    auto btn1 = ButtonHelper(kButton1Pin);
    auto btn2 = ButtonHelper(kButton2Pin);
    auto btn3 = ButtonHelper(kButton3Pin);
    auto btn4 = ButtonHelper(kButton4Pin);

    uint32_t cycle1 = 25;
    uint32_t cycle2 = 50;
    uint32_t cycle3 = 75;
    uint32_t cycle4 = 100;
    pwm1.SetDutyCycle(cycle1);
    pwm2.SetDutyCycle(cycle2);
    pwm3.SetDutyCycle(cycle3);
    pwm4.SetDutyCycle(cycle4);
    while (true)
    {
        if (btn1.IsPressed())
        {
            cycle1 = (cycle1 + 1) % 101;
            pwm1.SetDutyCycle(cycle1);
        }

        sleep_ms(100);
    }

    return 0;
}
