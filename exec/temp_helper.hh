#pragma once

#include <cmath>
#include <cstdint>

namespace utility {

uint32_t GetResistantValue(uint16_t adc_value, uint16_t adc_max) {
    // 10k resistor
    return 10000 * uint32_t(adc_max - adc_value) / adc_value;
}

float GetTemperature(uint32_t resist, uint32_t therm_beta,
                     float cal_temp = 25.f, uint32_t cal_resist = 10000) {
    // Steinhart-Hart equation
    // 1/T = 1/T0 + 1/B * ln(R/R0)
    // T = 1 / (1/T0 + 1/B * ln(R/R0))
    cal_temp += 273.15f;  // Convert to Celsius
    return (1.f / (1.f / cal_temp +
                   std::log(float(resist) / float(cal_resist)) / therm_beta)) -
           273.15f;
}

}  // namespace utility