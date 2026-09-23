#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

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

// A fan curve: points sorted by x, linearly interpolated between neighbors
// and clamped below the first / above the last point. Tables are constexpr
// and live in flash, no heap involved.
struct CurvePoint {
    uint32_t x;
    uint32_t y;
};

// non-owning view over a constexpr curve table
struct CurveSpan {
    template <size_t __PointCount>
    constexpr CurveSpan(const CurvePoint (&points)[__PointCount])
        : points(points), count(__PointCount) {}

    const CurvePoint *points;
    size_t count;
};

constexpr uint32_t GetCurveValue(const CurveSpan &curve, float input) {
    if (input <= float(curve.points[0].x)) {
        return curve.points[0].y;
    }
    for (size_t i = 1; i < curve.count; ++i) {
        if (input <= float(curve.points[i].x)) {
            const auto &lo = curve.points[i - 1];
            const auto &hi = curve.points[i];
            return lo.y + float(hi.y - lo.y) * (input - float(lo.x)) /
                              float(hi.x - lo.x);
        }
    }
    return curve.points[curve.count - 1].y;
}

// curve set of one fan type: the fan speed manager picks the curve matching
// its control mode; boards pass the set matching the attached fan
struct FanCurves {
    CurveSpan temp_to_pwm;  // °C -> pwm cycle
    CurveSpan pwr_to_pwm;   // watts (INA226) -> pwm cycle
    CurveSpan temp_to_rpm;  // °C -> target rpm
};

constexpr CurvePoint kDefaultTempToPwmCurve[]{
    {10, 1500}, {20, 2000}, {30, 2600}, {35, 3100}, {40, 3600},
    {45, 4400}, {50, 5500}, {55, 6800}, {60, 8100}, {65, 10000},
};

constexpr CurvePoint kDefaultTempToRpmCurve[]{
    {20, 800},  {30, 1000}, {40, 1100}, {50, 1400},
    {60, 1600}, {70, 1800}, {80, 2000},
};

// fan power draw in watts (INA226) -> pwm
constexpr CurvePoint kDefaultPwrToPwmCurve[]{
    {5, 1500},  {10, 2000}, {30, 2600},  {50, 3100},  {70, 3600},
    {80, 4400}, {90, 5500}, {110, 6800}, {120, 8100}, {145, 10000},
};

constexpr FanCurves kDefaultFanCurves{
    kDefaultTempToPwmCurve,
    kDefaultPwrToPwmCurve,
    kDefaultTempToRpmCurve,
};

}  // namespace utility
