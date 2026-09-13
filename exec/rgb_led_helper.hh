#pragma once

#include <hardware/gpio.h>
#include <hardware/pio.h>

#include "ws2812.pio.h"

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

// Single WS2812/SK6812 pixel driven through PIO (see ws2812.pio).
class Ws2812Helper {
public:
    explicit Ws2812Helper(const uint pin, const bool rgbw = true,
                          PIO pio = pio0, const uint freq_hz = 800000) noexcept
        : pio_(pio),
          sm_(pio_claim_unused_sm(pio_, true)),
          offset_(pio_add_program(pio_, &ws2812_program)) {
        ws2812_program_init(pio_, sm_, offset_, pin, freq_hz, rgbw);
        SetRgb(0, 0, 0);
    }

    void SetRgb(const uint8_t red, const uint8_t green,
                const uint8_t blue) noexcept {
        // pixels take rgb on the wire, the white byte rides last (SK6812 RGBW)
        const uint32_t rgbw = (uint32_t(red) << 24) | (uint32_t(green) << 16) |
                              (uint32_t(blue) << 8);
        pio_sm_put_blocking(pio_, sm_, rgbw);
        current_value_ = (red ? 1 : 0) | (green ? 2 : 0) | (blue ? 4 : 0);
    }

    void SetWhite(const uint8_t white) noexcept {
        pio_sm_put_blocking(pio_, sm_, white);
    }

    void SetRed() noexcept { SetRgb(0xff, 0, 0); }
    void SetGreen() noexcept { SetRgb(0, 0xff, 0); }
    void SetBlue() noexcept { SetRgb(0, 0, 0xff); }
    void Off() noexcept { SetRgb(0, 0, 0); }

    // steps through the 8 on/off RGB combinations, like RgbLedHelper::Next()
    void Next() noexcept {
        SetRgb(current_value_ & 1 ? 0xff : 0, current_value_ & 2 ? 0xff : 0,
               current_value_ & 4 ? 0xff : 0);
        current_value_ = (current_value_ + 1) % 8;
    }

private:
    PIO pio_;
    uint sm_;
    uint offset_;
    uint8_t current_value_ = 0;

    DISALLOW_COPY(Ws2812Helper);
    DISALLOW_MOVE(Ws2812Helper);
};

}  // namespace utility
