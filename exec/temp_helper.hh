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

inline float GetTemperature(uint32_t resist,
                            const ThermistorParams &params = kNtc10k3435) {
    return params.GetTemperature(resist);
}

template <class __CurveInterpolator>
class TemperatureCurveCalculator {
    using CurveInterpolator = __CurveInterpolator;
    using TemperatureValueType = float;
    using CurveValueType = uint32_t;
    using CurveMap = std::map<CurveValueType, CurveValueType>;

public:
    explicit TemperatureCurveCalculator(
        std::initializer_list<CurveMap::value_type> init)
        : temp_to_curve_value_(init) {}

    CurveValueType GetCurveValue(TemperatureValueType temp) const {
        auto r = temp_to_curve_value_.lower_bound(CurveValueType(temp));
        if (r == temp_to_curve_value_.end()) {
            r = std::prev(temp_to_curve_value_.end());
        }

        auto l = std::prev(r);
        if (TemperatureValueType(r->first) == temp) {
            l = r;
        } else if (l == temp_to_curve_value_.end()) {
            l = temp_to_curve_value_.begin();
        }

        return CurveInterpolator()(l->first, l->second, r->first, r->second,
                                   temp);
    }

private:
    const CurveMap temp_to_curve_value_;
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

using LinearTemperatureCurveCalculator =
    TemperatureCurveCalculator<LinearInterpolator>;

using LowerBoundTemperatureCurveCalculator =
    TemperatureCurveCalculator<LowerBoundInterpolator>;

const LinearTemperatureCurveCalculator kLinearFanPwmCurve{
    {10, 1500}, {20, 2000}, {30, 2600}, {35, 3300}, {40, 4200},
    {45, 5200}, {50, 6400}, {55, 7600}, {60, 8800}, {65, 10000},
};

const LinearTemperatureCurveCalculator kLinearFanRpmCurve{
    {20, 800},  {30, 1000}, {40, 1100}, {50, 1400},
    {60, 1600}, {70, 1800}, {80, 2000},
};

}  // namespace utility
