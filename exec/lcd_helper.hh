#pragma once

#include <hardware/i2c.h>

#include <array>

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

class XiaoRp2040Ssd1306Device : public Ssd1306Device {
    constexpr static const uint8_t kI2cSclPin = 7;
    constexpr static const uint8_t kI2cSdaPin = 6;

public:
    explicit XiaoRp2040Ssd1306Device(uint16_t width, uint16_t height)
        : Ssd1306Device(i2c1, kI2cSclPin, kI2cSdaPin, width, height) {}
};

template <class __DeviceType, size_t __ItemCount>
class LcdDrawer {
    using DeviceType = __DeviceType;

public:
    struct TempItem {
        bool is_cycle;
        uint32_t target;
        uint32_t rpm;
    };

    using TempItemArray = std::array<TempItem, __ItemCount>;

    LcdDrawer(uint16_t width, uint16_t height) noexcept
        : device_(width, height) {}

    void SetContrast(uint8_t val) noexcept { device_.SetContrast(val); }

    void DrawTempAndItems(float temp, const TempItemArray &items) noexcept {
        device_.Clear();
        char buf[128] = {};
        uint16_t y = 0;

        for (size_t i = 0; i < items.size(); ++i) {
            y += DrawSpeed(i, items[i], 0, y);
        }

        snprintf(buf, sizeof(buf), "Temp:%.2fdeg", temp);
        device_.DrawString(buf, 0, y);

        device_.EndDraw();
    }

private:
    uint16_t DrawSpeed(size_t index, const TempItem &item, uint16_t x,
                       uint16_t y) noexcept {
        char buf[128] = {};
        if (item.is_cycle) {
            snprintf(buf, sizeof(buf), "Spd%d: %d, Cyc: %d%%", (int)index,
                     (int)item.rpm, (int)item.target);
        } else {
            snprintf(buf, sizeof(buf), "Spd%d: %d, Tag: %d", (int)index,
                     (int)item.rpm, (int)item.target);
        }

        device_.DrawString(buf, x, y);
        return device_.GetFontHeight();
    }

    DeviceType device_;

    DISALLOW_COPY(LcdDrawer);
    DISALLOW_MOVE(LcdDrawer);
};

template <size_t __ItemCount>
using XiaoRp2040LcdDrawer = LcdDrawer<XiaoRp2040Ssd1306Device, __ItemCount>;

}  // namespace utility
