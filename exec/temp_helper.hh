#pragma once

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <map>

#include "base_types.hh"

namespace utility {

inline uint32_t GetResistantValue(uint16_t adc_value, uint16_t adc_max) {
    // 10k resistor
    return 10000 * uint32_t(adc_max - adc_value) / adc_value;
}

class ThermistorParams {
public:
    constexpr explicit ThermistorParams(uint32_t beta, float temp,
                                        uint32_t resist)
        : beta_(float(beta)),                             // beta value
          beta_over_t0_(float(beta) / (temp + 273.15f)),  // beta / T0
                                                          // ln(R0)
          ln_r0_(std::log(float(resist))) {}

    float GetTemperature(uint32_t resist) const {
        // Steinhart-Hart equation
        // 1/T = 1/T0 + 1/B * ln(R/R0)
        // T = 1 / (1/T0 + 1/B * ln(R/R0))
        // T = 1 / (1/T0 + 1/B * (ln(R) - ln(R0)))
        // T = B / (B/T0 + ln(R) - ln(R0))
        return (beta_ / (beta_over_t0_ + std::log(float(resist)) - ln_r0_)) -
               273.15f;
    }

private:
    const float beta_;          // thermistor beta value
    const float beta_over_t0_;  // beta / T0
    const float ln_r0_;         // ln(R0)

    DISALLOW_COPY(ThermistorParams);
    DISALLOW_MOVE(ThermistorParams);
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

template <class __CurveInterpolator>
class CurveCalculator {
    using CurveInterpolator = __CurveInterpolator;
    using InputValueType = float;
    using CurveValueType = uint32_t;
    using CurveMap = std::map<CurveValueType, CurveValueType>;

public:
    explicit CurveCalculator(std::initializer_list<CurveMap::value_type> init)
        : input_to_curve_value_(init) {}

    CurveValueType GetCurveValue(InputValueType input) const {
        auto r = input_to_curve_value_.lower_bound(CurveValueType(input));
        if (r == input_to_curve_value_.end()) {
            r = std::prev(input_to_curve_value_.end());
        } else if (r == input_to_curve_value_.begin() &&
                   InputValueType(r->first) > input) {
            // below the first point, clamp to it (prev(begin) is UB)
            return r->second;
        }

        auto l = std::prev(r);
        if (InputValueType(r->first) == input) {
            l = r;
        }

        return CurveInterpolator()(l->first, l->second, r->first, r->second,
                                   input);
    }

private:
    const CurveMap input_to_curve_value_;
};

struct LinearInterpolator {
    uint32_t operator()(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
                        float x) const {
        return y0 + float(y1 - y0) * (x - float(x0)) / float(x1 - x0);
    }
};

struct LowerBoundInterpolator {
    uint32_t operator()(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
                        float x) const {
        (void)x0;
        (void)x1;
        (void)y1;
        (void)x;
        return y0;
    }
};

using LinearCurveCalculator = CurveCalculator<LinearInterpolator>;

using LowerBoundCurveCalculator = CurveCalculator<LowerBoundInterpolator>;

const LinearCurveCalculator kLinearFanTempToPwmCurve{
    {10, 1500}, {20, 2000}, {30, 2600}, {35, 3100}, {40, 3600},
    {45, 4400}, {50, 5500}, {55, 6800}, {60, 8100}, {65, 10000},
};

const LinearCurveCalculator kLinearFanTempToRpmCurve{
    {20, 800},  {30, 1000}, {40, 1100}, {50, 1400},
    {60, 1600}, {70, 1800}, {80, 2000},
};

// fan power draw in watts (INA226) -> pwm
const LinearCurveCalculator kLinearFanPwrToPwmCurve{
    {5, 1500}, {10, 2000}, {30, 2600}, {50, 3100}, {70, 3600},
    {80, 4400}, {90, 5500}, {110, 6800}, {120, 8100}, {145, 10000},
};

}  // namespace utility
