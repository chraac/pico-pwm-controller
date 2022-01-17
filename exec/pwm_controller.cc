
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

constexpr uint PWM_FREQ_KHZ = 25;
constexpr uint PWM1_PIN = 0;
constexpr uint PWM2_PIN = 7;
constexpr uint PWM3_PIN = 27;
constexpr uint PWM4_PIN = 17;

void init_pwm_with_cfg(uint gpio_pin, const pwm_config &pwm_cfg)
{
    auto slice_num = pwm_gpio_to_slice_num(gpio_pin);
    pwm_init(slice_num, const_cast<pwm_config *>(&pwm_cfg), true);
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);
}

int main()
{
    stdio_init_all();

    auto config = pwm_get_default_config();
    auto sys_clk = clock_get_hz(clk_sys) / 1000;
    pwm_config_set_clkdiv_int(&config, sys_clk / PWM_FREQ_KHZ);
    pwm_config_set_clkdiv_mode(&config, PWM_DIV_FREE_RUNNING);
    pwm_config_set_wrap(&config, 100);

    // PWM1
    init_pwm_with_cfg(PWM1_PIN, config);
    init_pwm_with_cfg(PWM2_PIN, config);
    init_pwm_with_cfg(PWM3_PIN, config);
    init_pwm_with_cfg(PWM4_PIN, config);

    pwm_set_gpio_level(PWM1_PIN, 25);
    pwm_set_gpio_level(PWM2_PIN, 50);
    pwm_set_gpio_level(PWM3_PIN, 75);
    pwm_set_gpio_level(PWM4_PIN, 100);
    return 0;
}
