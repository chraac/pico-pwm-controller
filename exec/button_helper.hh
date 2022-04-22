
#pragma once

#include <hardware/gpio.h>

#include "base_types.hh"

namespace utility {
class ButtonHelper {
public:
    ButtonHelper(const uint gpio_pin) noexcept : gpio_pin_(gpio_pin) {
        gpio_init(gpio_pin_);
        gpio_set_dir(gpio_pin_, GPIO_IN);
        // We are using the button to pull down to 0v when pressed, so ensure
        // that when unpressed, it uses internal pull ups. Otherwise when
        // unpressed, the input will be floating.
        gpio_pull_up(gpio_pin_);
    }

    bool IsPressed() { return !gpio_get(gpio_pin_); }

private:
    const uint gpio_pin_;

    DISALLOW_COPY(ButtonHelper);
    DISALLOW_MOVE(ButtonHelper);
};
}  // namespace utility
