
#include "pico/stdlib.h"
#include "pwm_helper.hh"

constexpr uint kPwmFreqKhz = 25;
constexpr uint kPwm1Pin = 0;
constexpr uint kPwm2Pin = 7;
constexpr uint kPwm3Pin = 27;
constexpr uint kPwm4Pin = 17;

using namespace utility;

int main()
{
    stdio_init_all();
    clocks_init();

    auto pwm1 = PwmHelper(kPwm1Pin, kPwmFreqKhz);
    auto pwm2 = PwmHelper(kPwm2Pin, kPwmFreqKhz);
    auto pwm3 = PwmHelper(kPwm3Pin, kPwmFreqKhz);
    auto pwm4 = PwmHelper(kPwm4Pin, kPwmFreqKhz);

    pwm1.SetDutyCycle(25);
    pwm2.SetDutyCycle(50);
    pwm3.SetDutyCycle(75);
    pwm4.SetDutyCycle(100);

    for (size_t i = 0;; i = (i + 1) % 4)
    {
        pwm1.SetDutyCycle(i * 25);
        sleep_ms(2000);
    }

    return 0;
}
