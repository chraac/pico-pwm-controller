#pragma once

#include "base_types.hh"

#ifdef PLATFORM_PICO
#include <hardware/i2c.h>
namespace utility {
typedef i2c_inst_t *i2c_handler_t;
}
#elif defined(PLATFORM_ESP32)
#include <driver/i2c.h>
namespace utility {
typedef i2c_cmd_handle_t i2c_handler_t;
}
#endif

namespace utility {

class I2cHelper {
public:
#ifdef PLATFORM_PICO
    I2cHelper(uint32_t gpio_scl, uint32_t gpio_sda,
              uint32_t baudrate_hz) noexcept;
#elif defined(PLATFORM_ESP32)
    I2cHelper(uint32_t gpio_scl, uint32_t gpio_sda, uint32_t baudrate_hz,
              uint32_t i2c_port = 0) noexcept;
#endif

    ~I2cHelper() noexcept;

    int Read(uint8_t addr, uint8_t *dst, size_t len) noexcept;
    int Write(uint8_t addr, const uint8_t *src, size_t len) noexcept;

private:
    uint32_t gpio_scl_;
    uint32_t gpio_sda_;
    uint32_t baudrate_hz_;
    i2c_handler_t handler_ = nullptr;
#ifdef PLATFORM_ESP32
    uint32_t i2c_master_port_ = 0;
#endif

    DISALLOW_COPY(I2cHelper);
    DISALLOW_MOVE(I2cHelper);
};

}  // namespace utility
