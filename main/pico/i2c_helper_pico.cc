
// clang-format off
#include "i2c_helper.hh"
// clang-format on

#include <hardware/gpio.h>

using namespace utility;

namespace {

constexpr i2c_inst_t *kDefaultI2cInstance = &i2c0_inst;

}

I2cHelper::I2cHelper(uint32_t gpio_scl, uint32_t gpio_sda, uint32_t baudrate_hz)
    : gpio_scl_(gpio_scl), gpio_sda_(gpio_sda), baudrate_hz_(baudrate_hz) {
    handler_ = kDefaultI2cInstance;
    i2c_init(handler_, baudrate_hz);
    gpio_set_function(gpio_sda, GPIO_FUNC_I2C);
    gpio_set_function(gpio_scl, GPIO_FUNC_I2C);
    gpio_pull_up(gpio_sda);
    gpio_pull_up(gpio_scl);
}

I2cHelper::~I2cHelper() {
    if (handler_) {
        i2c_deinit(handler_);
    }
}

int I2cHelper::Read(uint8_t addr, uint8_t *dst, size_t len) {
    return i2c_read_blocking(handler_, addr, dst, len, false);
}

int I2cHelper::Write(uint8_t addr, const uint8_t *src, size_t len) {
    return i2c_write_blocking(handler_, addr, src, len, false);
}