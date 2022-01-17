
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

constexpr uint PWM_FREQ_KHZ = 25;
constexpr uint PWM1_PIN = 0;
constexpr uint PWM2_PIN = 7;
constexpr uint PWM3_PIN = 27;
constexpr uint PWM4_PIN = 17;

int main()
{
    stdio_init_all();

    auto config = pwm_get_default_config();
    auto sys_clk = clock_get_hz(clk_sys) / 1000;
    pwm_config_set_clkdiv(&config, float(sys_clk) / float(PWM_FREQ_KHZ));
    pwm_config_set_clkdiv_mode(&config, PWM_DIV_FREE_RUNNING);
    pwm_config_set_wrap(&config, 100);

    auto slice_num = pwm_gpio_to_slice_num(PWM1_PIN);
    pwm_init(slice_num, &config, true);
    gpio_set_function(PWM1_PIN, GPIO_FUNC_PWM);
    pwm_set_gpio_level(PWM1_PIN, 50);
    return 0;
}
