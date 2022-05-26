
// clang-format off
#include "i2c_helper.hh"
// clang-format on

using namespace utility;

namespace {

constexpr auto kDefaultReadWriteTimeoutMs = pdMS_TO_TICKS(10);

int Esp32ErrorToInt(esp_err_t err) { return err > 0 ? -err : err; }

}  // namespace

I2cHelper::I2cHelper(uint32_t gpio_scl, uint32_t gpio_sda, uint32_t baudrate_hz,
                     i2c_port_t i2c_port)
    : gpio_scl_(gpio_scl),
      gpio_sda_(gpio_sda),
      baudrate_hz_(baudrate_hz),
      handler_(i2c_port) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = int(gpio_sda),
        .scl_io_num = int(gpio_scl),
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {baudrate_hz},
        .clk_flags = 0,
    };

    i2c_param_config(handler_, &conf);
    i2c_driver_install(handler_, conf.mode, 0, 0, 0);
}

I2cHelper::~I2cHelper() {
    if (handler_) {
        i2c_driver_delete(handler_);
    }
}

int I2cHelper::Read(uint8_t addr, uint8_t *dst, size_t len) {
    return Esp32ErrorToInt(i2c_master_read_from_device(
        handler_, addr, dst, len, kDefaultReadWriteTimeoutMs));
}

int I2cHelper::Write(uint8_t addr, const uint8_t *src, size_t len) {
    return Esp32ErrorToInt(i2c_master_write_to_device(
        handler_, addr, src, len, kDefaultReadWriteTimeoutMs));
}