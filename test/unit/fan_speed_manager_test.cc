// SingleFanSpeedManager / FanSpeedManagerWithSelector unit tests.
//
// Pin allocation (static per-pin event slots -- see stub notes):
//   frequency_counter_test: 0-4, fan_speed_helper_test: 5-7,
//   WithSelector internals: tach 13, sel 8-11,
//   SingleFanSpeedManager: pwm 21, spd 22-24, WithSelector pwm 17-20.
#include <gtest/gtest.h>

#include "fan_speed_manager.hh"
#include "stub_control.hh"
#include "temp_helper.hh"

namespace {

using Mode = utility::SingleFanSpeedManager::ControlMode;
using utility::FanCurves;
using utility::FanSpeedManagerWithSelector;
using utility::SingleFanSpeedManager;

// alternate pwr->pwm table (fan "type 1" style): 50 W lands on 3600
constexpr utility::CurvePoint kAltPwrCurve[]{
    {5, 1500}, {50, 3600}, {100, 10000},
};
constexpr FanCurves kAltFanCurves{
    utility::kDefaultTempToPwmCurve,
    kAltPwrCurve,
    utility::kDefaultTempToRpmCurve,
};

class FanSpeedManagerTest : public ::testing::Test {
protected:
    void SetUp() override { pico_stub::reset(); }

    // fire `count` tach edges and let 1 s of fake time pass -> mHz = count*1000
    void spinFan(uint spd_pin, uint count) {
        for (uint i = 0; i < count; ++i) {
            pico_stub::fireGpioIrq(spd_pin);
        }
        pico_stub::advanceUs(1'000'000);
    }
};

//
// SingleFanSpeedManager -- curve-driven modes
//

TEST_F(FanSpeedManagerTest, ConstructorSetsStartCycle) {
    SingleFanSpeedManager mgr{21, 22, Mode::kTempToPwm,
                                    utility::kDefaultFanCurves};
    EXPECT_EQ(mgr.GetPwmCycle(), 500u);
    EXPECT_EQ(mgr.GetPwmGpioPin(), 21u);
    EXPECT_TRUE(mgr.IsControlByPwm());
}

TEST_F(FanSpeedManagerTest, TempToPwmFollowsCurve) {
    SingleFanSpeedManager mgr{21, 22, Mode::kTempToPwm,
                                    utility::kDefaultFanCurves};
    EXPECT_EQ(mgr.Next(30.0f), 0u);  // returns current rpm (no tach yet)
    EXPECT_EQ(mgr.GetPwmCycle(), 2600u);
    mgr.Next(5.0f);
    EXPECT_EQ(mgr.GetPwmCycle(), 1500u);   // below first point clamps
    mgr.Next(70.0f);
    EXPECT_EQ(mgr.GetPwmCycle(), 10000u);  // above last point clamps
}

TEST_F(FanSpeedManagerTest, PwrToPwmFollowsCurve) {
    SingleFanSpeedManager mgr{21, 22, Mode::kPwrToPwm,
                                    utility::kDefaultFanCurves};
    mgr.Next(90.0f);
    EXPECT_EQ(mgr.GetPwmCycle(), 5500u);
    mgr.Next(200.0f);
    EXPECT_EQ(mgr.GetPwmCycle(), 10000u);
    mgr.Next(0.0f);
    EXPECT_EQ(mgr.GetPwmCycle(), 1500u);
    EXPECT_TRUE(mgr.IsControlByPwm());
}

TEST_F(FanSpeedManagerTest, PwrCurveIsPerFanType) {
    // same watts, different attached fan -> different duty cycle table
    SingleFanSpeedManager default_type{
        21, 22, Mode::kPwrToPwm, utility::kDefaultFanCurves};
    SingleFanSpeedManager alt_type{21, 23, Mode::kPwrToPwm,
                                          kAltFanCurves};
    default_type.Next(50.0f);
    alt_type.Next(50.0f);
    EXPECT_EQ(default_type.GetPwmCycle(), 3100u);
    EXPECT_EQ(alt_type.GetPwmCycle(), 3600u);
}

TEST_F(FanSpeedManagerTest, DutyCycleScalesToPwmTop) {
    // cycle 2600 of 10000 over top 9999 -> gpio level 2599
    SingleFanSpeedManager mgr{21, 22, Mode::kTempToPwm,
                                    utility::kDefaultFanCurves};
    mgr.Next(30.0f);
    EXPECT_EQ(pico_stub::pwmGpioLevel(21), 2599u);
}

//
// SingleFanSpeedManager -- PID-to-RPM mode
//

TEST_F(FanSpeedManagerTest, TempToRpmNoTachResetsToStartCycle) {
    SingleFanSpeedManager mgr{21, 24, Mode::kTempToRpm,
                                    utility::kDefaultFanCurves};
    // drive to a non-start cycle first
    spinFan(24, 10);  // rpm 300
    mgr.Next(30.0f);
    EXPECT_EQ(mgr.GetPwmCycle(), 574u);
    // tach went silent: reset to start cycle, return 0
    EXPECT_EQ(mgr.Next(30.0f), 0u);
    EXPECT_EQ(mgr.GetPwmCycle(), 500u);
    // target still tracked from the curve
    EXPECT_EQ(mgr.GetTargetRpm(), 1000u);
    EXPECT_FALSE(mgr.IsControlByPwm());
}

TEST_F(FanSpeedManagerTest, TempToRpmPidDrivesCycle) {
    SingleFanSpeedManager mgr{21, 24, Mode::kTempToRpm,
                                    utility::kDefaultFanCurves};
    spinFan(24, 10);  // 10 edges/s -> rpm 300
    EXPECT_EQ(mgr.Next(30.0f), 300u);
    // error 700: p=350, i=210, d=14 -> 574
    EXPECT_EQ(mgr.GetPwmCycle(), 574u);
}

TEST_F(FanSpeedManagerTest, TempToRpmIntegralAccumulates) {
    SingleFanSpeedManager mgr{21, 24, Mode::kTempToRpm,
                                    utility::kDefaultFanCurves};
    spinFan(24, 10);
    mgr.Next(30.0f);  // 574
    spinFan(24, 10);
    mgr.Next(30.0f);  // error 700 again: p=350, i=420 (integral 1400), d=0
    EXPECT_EQ(mgr.GetPwmCycle(), 770u);
}

TEST_F(FanSpeedManagerTest, TempToRpmDeadBandSkipsPid) {
    SingleFanSpeedManager mgr{21, 24, Mode::kTempToRpm,
                                    utility::kDefaultFanCurves};
    spinFan(24, 10);
    mgr.Next(30.0f);
    EXPECT_EQ(mgr.GetPwmCycle(), 574u);

    // rpm 1020 (34 edges/s), inside [1000, 1060): skip, cycle unchanged
    spinFan(24, 34);
    EXPECT_EQ(mgr.Next(30.0f), 1020u);
    EXPECT_EQ(mgr.GetPwmCycle(), 574u);

    // rpm 1080 (36 edges/s), one past the band: pid runs, negative error
    // clamps to min
    spinFan(24, 36);
    EXPECT_EQ(mgr.Next(30.0f), 1080u);
    EXPECT_EQ(mgr.GetPwmCycle(), 500u);
}

TEST_F(FanSpeedManagerTest, TempToRpmOverspeedClampsToMin) {
    SingleFanSpeedManager mgr{21, 24, Mode::kTempToRpm,
                                    utility::kDefaultFanCurves};
    spinFan(24, 100000);  // rpm 3000
    mgr.Next(30.0f);
    EXPECT_EQ(mgr.GetPwmCycle(), 500u);
}

TEST_F(FanSpeedManagerTest, TargetRpmTracksTempCurve) {
    SingleFanSpeedManager mgr{21, 24, Mode::kTempToRpm,
                                    utility::kDefaultFanCurves};
    mgr.Next(20.0f);
    EXPECT_EQ(mgr.GetTargetRpm(), 800u);
    mgr.Next(75.0f);  // midway between {70,1800} and {80,2000}
    EXPECT_EQ(mgr.GetTargetRpm(), 1900u);
}

//
// FanSpeedManagerWithSelector (mux variant)
//

TEST_F(FanSpeedManagerTest, SelectorAdvancesMuxPerNext) {
    FanSpeedManagerWithSelector mgr{17, 18, 19, 20};
    // ctor selected fan 0: all sel pins pulled down. Mapping (see the .cc):
    // selector_(kFanSelPin3=8, kFanSelPin2=9, kFanSelPin1=10, kFanSelPin0=11)
    // so bit0 drives pin 11, bit1 pin 10, bit2 pin 9, bit3 pin 8.
    EXPECT_FALSE(pico_stub::gpio(8).pull_up);
    EXPECT_FALSE(pico_stub::gpio(9).pull_up);
    EXPECT_FALSE(pico_stub::gpio(10).pull_up);
    EXPECT_FALSE(pico_stub::gpio(11).pull_up);

    mgr.Next();  // measures fan 0, then selects fan 1 (index 1)
    EXPECT_FALSE(pico_stub::gpio(8).pull_up);
    EXPECT_FALSE(pico_stub::gpio(9).pull_up);
    EXPECT_FALSE(pico_stub::gpio(10).pull_up);
    EXPECT_TRUE(pico_stub::gpio(11).pull_up);  // bit0

    mgr.Next();  // selects fan 2 (index 2)
    EXPECT_FALSE(pico_stub::gpio(8).pull_up);
    EXPECT_FALSE(pico_stub::gpio(9).pull_up);
    EXPECT_TRUE(pico_stub::gpio(10).pull_up);  // bit1
    EXPECT_FALSE(pico_stub::gpio(11).pull_up);
}

TEST_F(FanSpeedManagerTest, SelectorWritesPwmOnlyAtGroupBoundary) {
    FanSpeedManagerWithSelector mgr{17, 18, 19, 20};
    const auto initial_level = pico_stub::pwmGpioLevel(17);  // ctor start cycle

    mgr.Next();  // fan 0: not last of group -> no pwm write
    mgr.Next();  // fan 1: not last of group -> no pwm write
    EXPECT_EQ(pico_stub::pwmGpioLevel(17), initial_level);

    mgr.Next();  // fan 2: group boundary -> pid drives pwm_array_[0]
    // all fans read 0 rpm: error 1900 -> p=950, i=570, d=38 -> 1558
    // gpio level = 1558 * 9999 / 10000 = 1557
    EXPECT_EQ(pico_stub::pwmGpioLevel(17), 1557u);
}

TEST_F(FanSpeedManagerTest, SelectorDeadBandSkipsPid) {
    FanSpeedManagerWithSelector mgr{17, 18, 19, 20};
    // run one full group so pwm 0 gets a known level
    mgr.Next();
    mgr.Next();
    mgr.Next();
    const auto level_after_first_group = pico_stub::pwmGpioLevel(17);

    // every fan now reads 1920 rpm (64 edges/s): inside [1900, 1975] band
    for (int fan = 0; fan < 3; ++fan) {
        spinFan(13, 64);  // tach pin is hardcoded to 13 in the .cc
        mgr.Next();
    }
    // group boundary hit with max speed in the band -> no new pwm write
    EXPECT_EQ(pico_stub::pwmGpioLevel(17), level_after_first_group);
}

}  // namespace
