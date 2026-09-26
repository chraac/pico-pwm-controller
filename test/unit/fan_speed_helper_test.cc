// FanSpeedHelper unit tests (mHz -> rpm conversion). Pin range 5-7.
#include <gtest/gtest.h>

#include "fan_speed_helper.hh"
#include "stub_control.hh"

namespace {

class FanSpeedHelperTest : public ::testing::Test {
protected:
    void SetUp() override { pico_stub::reset(); }
};

TEST_F(FanSpeedHelperTest, HundredTachEdgesPerSecondIs3000Rpm) {
    utility::FanSpeedHelper helper{5};
    for (int i = 0; i < 100; ++i) {
        pico_stub::fireGpioIrq(5);
    }
    pico_stub::advanceUs(1'000'000);
    // rpm = mHz * 30 / 1000 (2 pulses per revolution)
    EXPECT_EQ(helper.GetFanSpeedRpm(), 3000u);
}

TEST_F(FanSpeedHelperTest, IdleFanReadsZero) {
    utility::FanSpeedHelper helper{6};
    pico_stub::advanceUs(1'000'000);
    EXPECT_EQ(helper.GetFanSpeedRpm(), 0u);
}

TEST_F(FanSpeedHelperTest, ResetClearsTheCount) {
    utility::FanSpeedHelper helper{7};
    for (int i = 0; i < 100; ++i) {
        pico_stub::fireGpioIrq(7);
    }
    helper.Reset();
    pico_stub::advanceUs(1'000'000);
    EXPECT_EQ(helper.GetFanSpeedRpm(), 0u);
}

}  // namespace
