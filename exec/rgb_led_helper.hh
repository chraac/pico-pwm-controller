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

    // global dimmer, 0..0xff: the pixel has no brightness register, so every
    // channel of every frame is scaled on the way out. Linear in LED current,
    // so perceived brightness tracks ~sqrt (0x40 reads about half as bright,
    // see docs/pico_ws2812.md §4). Re-emits the last colour.
    void SetBrightness(const uint8_t brightness) noexcept {
        brightness_ = brightness;
        Write();
    }

    void SetRgb(const uint8_t red, const uint8_t green,
                const uint8_t blue) noexcept {
        red_ = red;
        green_ = green;
        blue_ = blue;
        white_ = 0;
        Write();
    }

    void SetWhite(const uint8_t white) noexcept {
        red_ = green_ = blue_ = 0;
        white_ = white;
        Write();
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
    void Write() noexcept {
        // pixels take rgb on the wire, the white byte rides last (SK6812 RGBW)
        const uint32_t rgbw = (uint32_t(Scale(red_)) << 24) |
                              (uint32_t(Scale(green_)) << 16) |
                              (uint32_t(Scale(blue_)) << 8) | Scale(white_);
        pio_sm_put_blocking(pio_, sm_, rgbw);
        current_value_ = (red_ ? 1 : 0) | (green_ ? 2 : 0) | (blue_ ? 4 : 0);
    }

    // rounded 0..brightness_ rescale; identity at brightness_ == 0xff
    static uint8_t Scale(const uint8_t value, const uint8_t brightness) noexcept {
        return static_cast<uint8_t>((uint32_t(value) * brightness + 127) / 255);
    }

    uint8_t Scale(const uint8_t value) const noexcept {
        return Scale(value, brightness_);
    }

    PIO pio_;
    uint sm_;
    uint offset_;
    uint8_t red_ = 0;
    uint8_t green_ = 0;
    uint8_t blue_ = 0;
    uint8_t white_ = 0;
    uint8_t brightness_ = 0xff;
    uint8_t current_value_ = 0;

    DISALLOW_COPY(Ws2812Helper);
    DISALLOW_MOVE(Ws2812Helper);
};

}  // namespace utility
