#pragma once

#include <cstdint>

#include "base_types.hh"
#include "hardware/adc.h"

namespace utility {
class AdcHelper : public GpioBase {
public:
    explicit AdcHelper(uint gpio_pin) noexcept : GpioBase(gpio_pin) {
        adc_init();
        adc_gpio_init(gpio_pin);
        adc_select_input(gpio_pin - 26);  // 26 is the first ADC pin.
    }

    uint16_t Read() noexcept {
        return adc_read();  // 12-bit ADC
    }

    uint16_t GetMax() noexcept { return ((uint16_t)1 << 12) - 1; }

private:
    DISALLOW_COPY(AdcHelper);
    DISALLOW_MOVE(AdcHelper);
};
}  // namespace utility
