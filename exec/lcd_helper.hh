#pragma once

#include <hardware/i2c.h>

#include <array>

#include "base_types.hh"
#include "fan_control_mode.hh"

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
    constexpr static const uint8_t kDefaultContrast = 0x7F;

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
        ssd1306_contrast(&disp_, kDefaultContrast);
    }

    void Clear() noexcept { ssd1306_clear(&disp_); }

    void SetContrast(uint8_t val) noexcept { ssd1306_contrast(&disp_, val); }

    void DrawString(const char *str, uint16_t x, uint16_t y) noexcept {
        ssd1306_draw_string(&disp_, x, y, 1,
                            str);  // draw string with builtin 8x5 font
    }

    void EndDraw() noexcept { ssd1306_show(&disp_); }

    uint16_t GetFontHeight() const noexcept { return 8; }

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

template <uint8_t __SclPin, uint8_t __SdaPin>
class CustomSsd1306Device : public Ssd1306Device {
public:
    explicit CustomSsd1306Device(uint16_t width, uint16_t height)
        : Ssd1306Device(i2c1, __SclPin, __SdaPin, width, height) {}
};

template <class __DeviceType, size_t __ItemCount>
class LcdDrawer {
    using DeviceType = __DeviceType;

public:
    struct TempItem {
        FanControlMode mode;
        uint32_t target;
        uint32_t rpm;
    };

    using TempItemArray = std::array<TempItem, __ItemCount>;

    LcdDrawer(uint16_t width, uint16_t height) noexcept
        : device_(width, height) {}

    void SetContrast(uint8_t val) noexcept { device_.SetContrast(val); }

    void DrawTempAndItems(float temp, const TempItemArray &items) noexcept {
        DrawItemsAndFooter("Temp:%.2fdeg", temp, items);
    }

    void DrawPwrAndItems(float pwr, const TempItemArray &items) noexcept {
        DrawItemsAndFooter("Pwr:%.2fW", pwr, items);
    }

private:
    void DrawItemsAndFooter(const char *footer_fmt, float footer_value,
                            const TempItemArray &items) noexcept {
        device_.Clear();
        char buf[128] = {};
        uint16_t y = 0;

        for (size_t i = 0; i < items.size(); ++i) {
            y += DrawSpeed(i, items[i], 0, y);
        }

        snprintf(buf, sizeof(buf), footer_fmt, footer_value);
        device_.DrawString(buf, 0, y);

        device_.EndDraw();
    }

    uint16_t DrawSpeed(size_t index, const TempItem &item, uint16_t x,
                       uint16_t y) noexcept {
        char buf[128] = {};
        if (item.mode == FanControlMode::kTempToRpm) {
            snprintf(buf, sizeof(buf), "Spd%d: %d, Tag: %d", (int)index,
                     (int)item.rpm, (int)item.target);
        } else {
            // both pwm modes drive the duty cycle straight from their curve
            snprintf(buf, sizeof(buf), "Spd%d: %d, Cyc: %d%%", (int)index,
                     (int)item.rpm, (int)item.target);
        }

        device_.DrawString(buf, x, y);
        return device_.GetFontHeight();
    }

    DeviceType device_;

    DISALLOW_COPY(LcdDrawer);
    DISALLOW_MOVE(LcdDrawer);
};

template <uint8_t __SclPin, uint8_t __SdaPin, size_t __ItemCount>
using CustomLcdDrawer = LcdDrawer<CustomSsd1306Device<__SclPin, __SdaPin>, __ItemCount>;

template <size_t __ItemCount>
using XiaoRp2040LcdDrawer = LcdDrawer<CustomSsd1306Device<7, 6>, __ItemCount>;

}  // namespace utility
