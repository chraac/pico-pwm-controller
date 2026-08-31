#pragma once

#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include "base_types.hh"

namespace utility {

// TI INA226 current/voltage/power monitor, datasheet:
// https://www.ti.com/lit/ds/symlink/ina226.pdf
// Register/command details: docs/ina226_i2c.md
class Ina226Device {
    constexpr static const uint8_t kI2cAddr = 0x40;
    constexpr static const uint32_t kI2cFreq = 400000;  // 400kHz
    constexpr static const float kShuntOhms = 0.001f;  // R001 (1 mΩ) shunt
    // 16x averaging, 1.1ms both conversions, shunt+bus continuous (~35ms/update)
    constexpr static const uint16_t kDefaultConfig = 0x247F;
    constexpr static const uint16_t kResetCommand = 0x8000;
    constexpr static const uint16_t kManufacturerIdValue = 0x5449;  // 'TI'
    constexpr static const uint16_t kDieIdValue = 0x2260;
    constexpr static const uint16_t kConversionReadyBit = 0x0008;  // CVRF

    enum Reg : uint8_t {
        kConfig = 0x00,
        kShuntVoltage = 0x01,
        kBusVoltage = 0x02,
        kPower = 0x03,
        kCurrent = 0x04,
        kCalibration = 0x05,
        kMaskEnable = 0x06,
        kAlertLimit = 0x07,
        kManufacturerId = 0xFE,
        kDieId = 0xFF,
    };

public:
    constexpr static const uint8_t kI2cDefaultSclPin = 7;
    constexpr static const uint8_t kI2cDefaultSdaPin = 6;

    explicit Ina226Device(i2c_inst_t *i2c, uint8_t i2c_scl_pin,
                           uint8_t i2c_sda_pin) noexcept
        : i2c_inst_(i2c),
          i2c_scl_pin_(i2c_scl_pin),
          i2c_sda_pin_(i2c_sda_pin) {
        // same bus setup as Ssd1306Device, safe to run twice when the INA226
        // shares the LCD's i2c bus
        i2c_init(i2c, kI2cFreq);
        gpio_set_function(i2c_sda_pin_, GPIO_FUNC_I2C);
        gpio_set_function(i2c_scl_pin_, GPIO_FUNC_I2C);
        gpio_pull_up(i2c_sda_pin_);
        gpio_pull_up(i2c_scl_pin_);
    }

    // true if a likely INA226 answers (shares the bus with Ssd1306Device)
    bool Probe() noexcept {
        return Read16(kManufacturerId) == kManufacturerIdValue &&
               Read16(kDieId) == kDieIdValue;
    }

    void Reset() noexcept { Write16(kConfig, kResetCommand); }

    void Configure(uint16_t cfg = kDefaultConfig) noexcept {
        Write16(kConfig, cfg);
    }

    // bus voltage at the VBUS pin, 0-36V range (unsigned)
    float GetBusVolts() noexcept { return Read16(kBusVoltage) * 1.25e-3f; }

    // voltage across the shunt, ±81.92mV full scale (signed)
    float GetShuntMilliVolts() noexcept {
        return static_cast<int16_t>(Read16(kShuntVoltage)) * 2.5e-3f;
    }

    // simplest current path: I = V_shunt / R, no calibration needed
    float GetAmps() noexcept {
        return GetShuntMilliVolts() / (kShuntOhms * 1000.0f);
    }

    // on-chip current register, valid only after SetCalibration()
    float GetCurrentAmps() noexcept {
        return static_cast<int16_t>(Read16(kCurrent)) * current_lsb_amps_;
    }

    // on-chip power register, valid only after SetCalibration()
    float GetPowerWatts() noexcept {
        return Read16(kPower) * (current_lsb_amps_ * 25.0f);
    }

    // programs Calibration (0x05) so Current (0x04) / Power (0x03) fill in,
    // Current_LSB = max_current / 32768, see docs/ina226_i2c.md §7b
    void SetCalibration(float max_current) noexcept {
        current_lsb_amps_ = max_current / 32768.0f;
        uint32_t calib =
            uint32_t(0.00512f / (current_lsb_amps_ * kShuntOhms) + 0.5f);
        while (calib > 0x7FFF) {  // keep calib in 15 bits, coarsen the LSB
            current_lsb_amps_ *= 2.0f;
            calib >>= 1;
        }
        Write16(kCalibration, uint16_t(calib));
    }

    // poll to know the shunt/bus registers hold a fresh sample
    bool IsConversionReady() noexcept {
        return Read16(kMaskEnable) & kConversionReadyBit;
    }

private:
    uint16_t Read16(uint8_t reg) noexcept {
        uint8_t val[2] = {};
        i2c_write_blocking(i2c_inst_, kI2cAddr, &reg, 1, true);
        i2c_read_blocking(i2c_inst_, kI2cAddr, val, 2, false);
        return uint16_t(val[0] << 8) | val[1];
    }

    void Write16(uint8_t reg, uint16_t value) noexcept {
        uint8_t buf[3] = {reg, uint8_t(value >> 8), uint8_t(value & 0xFF)};
        i2c_write_blocking(i2c_inst_, kI2cAddr, buf, 3, false);
    }

    i2c_inst_t *i2c_inst_;
    uint8_t i2c_scl_pin_;
    uint8_t i2c_sda_pin_;
    float current_lsb_amps_ = 0.0f;  // 0 -> on-chip current/power read 0

    DISALLOW_COPY(Ina226Device);
    DISALLOW_MOVE(Ina226Device);
};

class XiaoRp2040Ina226Device : public Ina226Device {
    constexpr static const uint8_t kI2cSclPin = 7;
    constexpr static const uint8_t kI2cSdaPin = 6;

public:
    XiaoRp2040Ina226Device() : Ina226Device(i2c1, kI2cSclPin, kI2cSdaPin) {}
};

}  // namespace utility
