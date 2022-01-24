
#include "hardware/pwm.h"
#include "hardware/clocks.h"

namespace utility
{

    class PwmHelper
    {
    private:
        const uint _gpio_pin;
        pwm_config _pwm_config;

    public:
        PwmHelper(uint gpio_pin, const pwm_config &pwm_cfg) noexcept
            : _gpio_pin(gpio_pin), _pwm_config(pwm_cfg)
        {
            auto slice_num = pwm_gpio_to_slice_num(gpio_pin);
            pwm_init(slice_num, &_pwm_config, true);
            gpio_set_function(_gpio_pin, GPIO_FUNC_PWM);
        }

        void SetDutyCycle(uint32_t num, uint32_t denom = 100) noexcept
        {
            const auto max_cyc = _pwm_config.top + 1;
            pwm_set_gpio_level(_gpio_pin, num * max_cyc / denom);
        }
    };

}
