#pragma once

// clang-format off
#include "i2c_helper.hh"
// clang-format on

#include <array>

namespace utility {

class Aw9523Helper {
    typedef enum _Addrs {
        kI2cBaseAddr = 0x58,  // I2C base address for AW9523B
        kInput0 = 0x00,       // Register for reading input values
        kInput1 = 0x01,       // Register for reading input values
        kOutput0 = 0x02,      // Register for writing output values
        kOutput1 = 0x03,      // Register for writing output values
        kDirection0 = 0x04,   // Register for configuring direction
        kDirection1 = 0x05,   // Register for configuring direction
        kIntEnable0 = 0x06,   // Register for enabling interrupt
        kIntEnable1 = 0x07,   // Register for enabling interrupt
        kChipId = 0x10,  // Register for hardcode chip ID, The ID read value of
                         // AW9523B is 23H
        kGlobalControl = 0x11,  // Register for general configuration
        kGpioMode0 =
            0x12,  // Register for configuring const current mode (led mode)
        kGpioMode1 =
            0x13,  // Register for configuring const current mode (led mode)
        kSoftReset = 0x7F,  // Soft reset register
    } Addrs;

public:
    typedef enum {
        kGpioCount = 16,
    } Constants;

    typedef enum _Port {
        kPort0 = 0x00,  // Port 0
        kPort1 = 0x01,  // Port 1
    } Port;

    typedef enum _PortMode {
        kOpenDrain = 0x00,  // Port 0 open drain mode
        kPushPull = 1 << 4  // Port 0 push pull mode
    } PortMode;

    typedef enum _PortDirection {
        kDirectionOutput = 0x00,  // Output mode
        kDirectionInput = 0xFF,   // Input mode
    } PortDirection;

    typedef enum _GpioMode {
        kModeLed = 0x00,   // Led mode
        kModeGpio = 0xFF,  // Gpio mode
    } GpioMode;

    typedef enum _LedsDim {
        kMax = 0x00,  // 0~IMAX 37mA(typical)
        kMid = 0x01,  // 0~(IMAX×3/4)
        kLow = 0x02,  // 0~(IMAX×2/4)
        kMin = 0x03,  // 0~(IMAX×1/4)
    } LedsDim;

    /** AW9523B LED dimm current control registers*/
    typedef enum _LedDimCtrl {
        kP10 = 0x20,  // DIM0
        kP11 = 0x21,  // DIM1
        kP12 = 0x22,  // DIM2
        kP13 = 0x23,  // DIM3
        kP00 = 0x24,  // DIM4
        kP01 = 0x25,  // DIM5
        kP02 = 0x26,  // DIM6
        kP03 = 0x27,  // DIM7
        kP04 = 0x28,  // DIM8
        kP05 = 0x29,  // DIM9
        kP06 = 0x2A,  // DIM10
        kP07 = 0x2B,  // DIM11
        kP14 = 0x2C,  // DIM12
        kP15 = 0x2D,  // DIM13
        kP16 = 0x2E,  // DIM14
        kP17 = 0x2F,  // DIM15
    } LedDimCtrl;

    Aw9523Helper(uint32_t gpio_scl, uint32_t gpio_sda, uint32_t gpio_intr,
                 uint8_t ad0, uint8_t ad1) noexcept
        : gpio_scl_(gpio_scl),
          gpio_sda_(gpio_sda),
          gpio_intr_(gpio_intr),
          i2c_ad0_(ad0),
          i2c_ad1_(ad1),
          i2c_(gpio_scl, gpio_sda, 115200) {}

    void Reset() noexcept { WriteByte(kSoftReset, 0); }

    void SetPortMode(PortMode mode) {
        WriteByte(kGlobalControl, mode | leds_dim_);
        port_mode_ = mode;
    }

    void SetLedsDim(LedsDim dim) {
        WriteByte(kGlobalControl, port_mode_ | dim);
        leds_dim_ = dim;
    }

    void SetDirection(Port port, PortDirection direction) {
        WriteByte(port == kPort0 ? kDirection0 : kDirection1, direction);
    }

    void SetEnableInterrupt(Port port, bool enabled) {
        WriteByte(port == kPort0 ? kIntEnable0 : kIntEnable1,
                  enabled ? 0 : 0xFF);
    }

    void SetGpioMode(Port port, GpioMode mode) {
        WriteByte(port == kPort0 ? kGpioMode0 : kGpioMode1, mode);
    }

    uint8_t ReadPort(Port port) {
        return ReadByte(port == kPort0 ? kInput0 : kInput1);
    }

    void WritePort(Port port, uint8_t data) {
        return WriteByte(port == kPort0 ? kOutput0 : kOutput1, data);
    }

    uint8_t GetId() { return ReadByte(kChipId); }

private:
    uint8_t GetI2cBaseAddr() const noexcept {
        return Addrs::kI2cBaseAddr + (i2c_ad1_ << 1) + i2c_ad0_;
    }

    void WriteByte(uint8_t addr, uint8_t data) {
        const uint8_t buffer[] = {addr, data};
        i2c_.Write(GetI2cBaseAddr(), buffer, std::size(buffer));
    }

    uint8_t ReadByte(uint8_t addr) {
        const auto base_addr = GetI2cBaseAddr();
        i2c_.Write(base_addr, &addr, 1);
        uint8_t buffer = 0;
        i2c_.Read(base_addr, &buffer, 1);
        return buffer;
    }

    uint32_t gpio_scl_;
    uint32_t gpio_sda_;
    uint32_t gpio_intr_;
    uint8_t i2c_ad0_;
    uint8_t i2c_ad1_;
    I2cHelper i2c_;
    PortMode port_mode_ = kOpenDrain;
    LedsDim leds_dim_ = kMax;
};

}  // namespace utility
