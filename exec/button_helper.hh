
#pragma once

#include "hardware/gpio.h"
#include "base_types.hh"

namespace utility
{
    class ButtonHelper
    {
    public:
        ButtonHelper(const uint gpio_pin) noexcept
            : _gpio_pin(gpio_pin)
        {
            gpio_init(_gpio_pin);
            gpio_set_dir(_gpio_pin, GPIO_IN);
            // We are using the button to pull down to 0v when pressed, so ensure that when
            // unpressed, it uses internal pull ups. Otherwise when unpressed, the input will
            // be floating.
            gpio_pull_up(_gpio_pin);
        }

        bool IsPressed()
        {
            return !gpio_get(_gpio_pin);
        }

    private:
        const uint _gpio_pin;

        DISALLOW_COPY(ButtonHelper);
        DISALLOW_MOVE(ButtonHelper);
    };
}
