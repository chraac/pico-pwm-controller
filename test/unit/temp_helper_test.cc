// temp_helper unit tests -- pure logic (GetResistantValue, ThermistorParams,
// GetCurveValue over the constexpr CurvePoint tables).
#include <gtest/gtest.h>

#include "temp_helper.hh"

namespace {

TEST(GetResistantValueTest, HalfScaleAdc) {
    // 10000 * (4095-2048) / 2048 = 9995 (integer division)
    EXPECT_EQ(utility::GetResistantValue(2048, 4095), 9995u);
}

TEST(GetResistantValueTest, MonotonicallyDecreasingInAdc) {
    uint32_t last = UINT32_MAX;
    for (uint16_t adc = 1; adc <= 4095; adc += 64) {
        const auto r = utility::GetResistantValue(adc, 4095);
        EXPECT_LE(r, last);
        last = r;
    }
}

TEST(GetResistantValueTest, FullScaleIsZero) {
    EXPECT_EQ(utility::GetResistantValue(4095, 4095), 0u);
}

// NOTE: adc=0 divides by zero -- a production robustness gap, documented
// here rather than papered over by a test workaround.

TEST(ThermistorTest, NominalResistanceIs25C) {
    EXPECT_NEAR(utility::kNtc10k3435.GetTemperature(10000), 25.0f, 0.01f);
    EXPECT_NEAR(utility::kNtc10k3950.GetTemperature(10000), 25.0f, 0.01f);
    EXPECT_NEAR(utility::kNtc100k3950.GetTemperature(100000), 25.0f, 0.01f);
}

TEST(ThermistorTest, HigherResistanceIsColder) {
    // beta equation: R=18k on the 3435/10k part is ~10.5 C
    const auto t = utility::kNtc10k3435.GetTemperature(18000);
    EXPECT_NEAR(t, 10.5f, 0.1f);
    EXPECT_LT(t, 25.0f);
}

TEST(GetCurveValueTest, BelowFirstClampsToFirstY) {
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultTempToPwmCurve, 5.0f),
              1500u);
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultPwrToPwmCurve, 0.0f),
              1500u);
}

TEST(GetCurveValueTest, ExactPointReturnsY) {
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultTempToPwmCurve, 30.0f),
              2600u);
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultTempToRpmCurve, 30.0f),
              1000u);
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultPwrToPwmCurve, 50.0f),
              3100u);
}

TEST(GetCurveValueTest, InterpolatesBetweenPoints) {
    // 25 is midway between {20,2000} and {30,2600} -> 2300
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultTempToPwmCurve, 25.0f),
              2300u);
    // 10.5 is 5% of the way from {10,1500} to {20,2000} -> 1525
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultTempToPwmCurve, 10.5f),
              1525u);
    // 75 is midway between {70,1800} and {80,2000} -> 1900
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultTempToRpmCurve, 75.0f),
              1900u);
    // 45 is 75% of the way from {30,2600} to {50,3100} -> 2975
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultPwrToPwmCurve, 45.0f),
              2975u);
}

TEST(GetCurveValueTest, AboveLastClampsToLastY) {
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultTempToPwmCurve, 70.0f),
              10000u);
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultTempToRpmCurve, 90.0f),
              2000u);
    EXPECT_EQ(utility::GetCurveValue(utility::kDefaultPwrToPwmCurve, 200.0f),
              10000u);
}

TEST(GetCurveValueTest, SinglePointTableAlwaysReturnsThatY) {
    constexpr utility::CurvePoint table[]{{42, 4242}};
    EXPECT_EQ(utility::GetCurveValue(table, 0.0f), 4242u);
    EXPECT_EQ(utility::GetCurveValue(table, 42.0f), 4242u);
    EXPECT_EQ(utility::GetCurveValue(table, 100.0f), 4242u);
}

TEST(GetCurveValueTest, TwoPointTableInterpolates) {
    constexpr utility::CurvePoint table[]{{0, 0}, {10, 100}};
    EXPECT_EQ(utility::GetCurveValue(table, -1.0f), 0u);
    EXPECT_EQ(utility::GetCurveValue(table, 5.0f), 50u);
    EXPECT_EQ(utility::GetCurveValue(table, 11.0f), 100u);
}

TEST(FanCurvesTest, DefaultSetBindsTheThreeTables) {
    EXPECT_EQ(
        utility::GetCurveValue(utility::kDefaultFanCurves.temp_to_pwm, 30.0f),
        2600u);
    EXPECT_EQ(
        utility::GetCurveValue(utility::kDefaultFanCurves.temp_to_rpm, 30.0f),
        1000u);
    EXPECT_EQ(
        utility::GetCurveValue(utility::kDefaultFanCurves.pwr_to_pwm, 90.0f),
        5500u);
}

}  // namespace
