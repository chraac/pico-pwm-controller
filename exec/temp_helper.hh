#pragma once

#include <cmath>
#include <cstdint>

namespace utility {

uint32_t GetResistantValue(uint16_t adc_value, uint16_t adc_max) {
    // 10k resistor
    return 10000 * uint32_t(adc_max - adc_value) / adc_value;
}

struct ThermistorParams {
    const float beta;          // thermistor beta value
    const float beta_over_t0;  // beta / T0
    const float ln_r0;         // ln(R0)

    constexpr explicit ThermistorParams(uint32_t beta, float temp,
                                        uint32_t resist)
        : beta(float(beta)),                 // beta value
          beta_over_t0(float(beta) / temp),  // beta / T0
                                             // ln(R0)
          ln_r0(std::log(float(resist))) {}

    float GetTemperature(uint32_t resist) const {
        // Steinhart-Hart equation
        // 1/T = 1/T0 + 1/B * ln(R/R0)
        // T = 1 / (1/T0 + 1/B * ln(R/R0))
        // T = 1 / (1/T0 + 1/B * (ln(R) - ln(R0)))
        // T = B / (B/T0 + ln(R) - ln(R0))
        return (beta / (beta_over_t0 + std::log(float(resist)) - ln_r0)) -
               273.15f;
    }
};

// 10k resistor, 3435 beta value
constexpr const ThermistorParams kNtc10k3435{
    3435,
    25.f,
    10000,
};

// 10k resistor, 3950 beta value
constexpr const ThermistorParams kNtc10k3950{
    3950,
    25.f,
    10000,
};

// 100k resistor, 3950 beta value
constexpr const ThermistorParams kNtc100k3950{
    3950,
    25.f,
    100000,
};

float GetTemperature(uint32_t resist,
                     const ThermistorParams &params = kNtc10k3435) {
    return params.GetTemperature(resist);
}

}  // namespace utility