#pragma once

#include <hardware/i2c.h>

#include "base_types.hh"

#ifdef __cplusplus
extern "C" {
#endif

#include "ssd1306.h"

#ifdef __cplusplus
}
#endif

namespace utility {

class Ssd1306Device {
    constexpr static const uint8_t kI2cAddr = 0x3C;
    constexpr static const uint32_t kI2cFreq = 400000;  // 400kHz

public:
    explicit Ssd1306Device(i2c_inst_t *i2c, uint8_t i2c_scl_pin,
                           uint8_t i2c_sda_pin, uint16_t width,
                           uint16_t height) noexcept
        : i2c_inst_(i2c),
          i2c_scl_pin_(i2c_scl_pin),
          i2c_sda_pin_(i2c_sda_pin),
          width_(width),
          height_(height) {
        i2c_init(i2c, kI2cFreq);
        gpio_set_function(i2c_sda_pin_, GPIO_FUNC_I2C);
        gpio_set_function(i2c_scl_pin_, GPIO_FUNC_I2C);
        gpio_pull_up(i2c_sda_pin_);
        gpio_pull_up(i2c_scl_pin_);

        disp_.external_vcc = false;
        ssd1306_init(&disp_, width_, height_, kI2cAddr, i2c_inst_);
        ssd1306_clear(&disp_);
    }

    void Clear() noexcept { ssd1306_clear(&disp_); }

    void DrawString(const char *str, uint16_t x, uint16_t y) noexcept {
        ssd1306_draw_string(&disp_, x, y, 1,
                            str);  // draw string with builtin 8x5 font
    }

    void EndDraw() noexcept { ssd1306_show(&disp_); }

    uint16_t GetFontHeight() const noexcept { return 5; }

private:
    ssd1306_t disp_;
    i2c_inst_t *i2c_inst_;
    uint8_t i2c_scl_pin_;
    uint8_t i2c_sda_pin_;
    uint16_t width_;
    uint16_t height_;

    DISALLOW_COPY(Ssd1306Device);
    DISALLOW_MOVE(Ssd1306Device);
};

class XiaoRp2040Ssd1306Device : public Ssd1306Device {
    constexpr static const uint8_t kI2cSclPin = 7;
    constexpr static const uint8_t kI2cSdaPin = 6;

public:
    explicit XiaoRp2040Ssd1306Device(uint16_t width, uint16_t height)
        : Ssd1306Device(i2c1, kI2cSclPin, kI2cSdaPin, width, height) {}
};

template <class __DeviceType>
class LcdDrawer {
    using DeviceType = __DeviceType;

public:
    LcdDrawer(uint16_t width, uint16_t height) noexcept
        : device_(width, height) {}

    void DrawRpm(uint32_t rpm0, uint32_t rpm1, uint32_t rpm2, uint32_t rpm3,
                 uint32_t target_rpm) noexcept {
        device_.Clear();
        char buf[128] = {};
        uint16_t y = 0;
        snprintf(buf, sizeof(buf), "Target:%d", (int)target_rpm);
        device_.DrawString(buf, 0, y);

        y += device_.GetFontHeight();
        snprintf(buf, sizeof(buf), "Spd0:%d, Spd1:%d", (int)rpm0, (int)rpm1);
        device_.DrawString(buf, 0, y);

        y += device_.GetFontHeight();
        snprintf(buf, sizeof(buf), "Spd2:%d, Spd3:%d", (int)rpm2, (int)rpm3);
        device_.DrawString(buf, 0, y);

        device_.EndDraw();
    }

private:
    DeviceType device_;

    DISALLOW_COPY(LcdDrawer);
    DISALLOW_MOVE(LcdDrawer);
};

using XiaoRp2040LcdDrawer = LcdDrawer<XiaoRp2040Ssd1306Device>;

}  // namespace utility
