#pragma once

#include <hardware/gpio.h>

namespace utility {

class RgbLedHelper {
public:
    RgbLedHelper(const uint red_pin, const uint green_pin,
                 const uint blue_pin) noexcept
        : red_pin_(red_pin), green_pin_(green_pin), blue_pin_(blue_pin) {
        gpio_init(red_pin_);
        gpio_init(green_pin_);
        gpio_init(blue_pin_);
        gpio_set_dir(red_pin_, GPIO_OUT);
        gpio_set_dir(green_pin_, GPIO_OUT);
        gpio_set_dir(blue_pin_, GPIO_OUT);
    }

    void SetRed() noexcept { SetRgb(1); }
    void SetGreen() noexcept { SetRgb(2); }
    void SetBlue() noexcept { SetRgb(4); }
    void Next() noexcept {
        SetRgb(current_value_);
        current_value_ = (current_value_ + 1) % 8;
    }

private:
    void SetRgb(uint8_t rgb) noexcept {
        gpio_put(red_pin_, rgb & 1);
        gpio_put(green_pin_, rgb & 2);
        gpio_put(blue_pin_, rgb & 4);
    }

    uint red_pin_;
    uint green_pin_;
    uint blue_pin_;
    uint current_value_ = 0;

    DISALLOW_COPY(RgbLedHelper);
    DISALLOW_MOVE(RgbLedHelper);
};

}  // namespace utility
