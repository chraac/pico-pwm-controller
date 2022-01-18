
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

constexpr uint kPwmFreqKhz = 25;
constexpr uint kPwmTop = 999;
constexpr uint kPwm1Pin = 0;
constexpr uint kPwm2Pin = 7;
constexpr uint kPwm3Pin = 27;
constexpr uint kPwm4Pin = 17;

/*
 * For more detail, see: https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf, Page 553
 */
void pwm_config_set_freq(pwm_config &pwm_cfg, uint32_t freq_in_khz, uint32_t top)
{
    auto sys_clk_khz = clock_get_hz(clk_sys) / 1000;
    pwm_config_set_clkdiv_int(&pwm_cfg, sys_clk_khz / freq_in_khz / top);
    pwm_config_set_clkdiv_mode(&pwm_cfg, PWM_DIV_FREE_RUNNING);
    pwm_config_set_wrap(&pwm_cfg, top);
}

void init_pwm_with_cfg(uint gpio_pin, const pwm_config &pwm_cfg)
{
    auto slice_num = pwm_gpio_to_slice_num(gpio_pin);
    pwm_init(slice_num, const_cast<pwm_config *>(&pwm_cfg), true);
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);
}

int main()
{
    stdio_init_all();
    clocks_init();

    auto config = pwm_get_default_config();
    pwm_config_set_freq(config, kPwmFreqKhz, kPwmTop);

    // PWM1
    init_pwm_with_cfg(kPwm1Pin, config);
    init_pwm_with_cfg(kPwm2Pin, config);
    init_pwm_with_cfg(kPwm3Pin, config);
    init_pwm_with_cfg(kPwm4Pin, config);

    pwm_set_gpio_level(kPwm1Pin, 250);
    pwm_set_gpio_level(kPwm2Pin, 500);
    pwm_set_gpio_level(kPwm3Pin, 750);
    pwm_set_gpio_level(kPwm4Pin, 999);

    for (size_t i = 0;; i = (i + 1) % 4)
    {
        pwm_set_gpio_level(kPwm1Pin, i * 250);
        sleep_ms(2000);
    }

    return 0;
}
