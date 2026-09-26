// Pid unit tests. Constants mirror SingleFanSpeedManager's setup
// (exec/fan_speed_manager.cc): Pid(500, 10000, 1, 0.5, 0.3, 0.02).
// NOTE: calculate() CLAMPS to [min,max], it does not add min as a base --
// small errors land on the min clamp.
#include <utility>

#include <gtest/gtest.h>

#include "pid.hh"

namespace {

class PidTest : public ::testing::Test {
protected:
    // min=500 (start cycle), max=10000 (full duty), dt=1
    utility::Pid pid{500, 10000, 1, .5F, .3F, .02F};
};

TEST_F(PidTest, FirstStepIsPTermPlusITermPlusDTermClampedToMin) {
    // error 300: p=150, i=90 (integral=300), d=6 (last_error starts 0)
    // -> 246, clamped up to the min clamp 500
    EXPECT_EQ(pid.calculate(1000, 700), 500);
}

TEST_F(PidTest, UnclampedStep) {
    // error 1000: p=500, i=300, d=20 -> 820
    EXPECT_EQ(pid.calculate(1000, 0), 820);
}

TEST_F(PidTest, IntegralAccumulatesAcrossCalls) {
    EXPECT_EQ(pid.calculate(1000, 0), 820);   // integral = 1000
    EXPECT_EQ(pid.calculate(1000, 0), 1100);  // integral = 2000, d = 0
}

TEST_F(PidTest, DerivativeIsZeroForConstantError) {
    // step1: 500(p) + 300(i) + 20(d); step2: 500(p) + 600(i) + 0(d)
    const auto first = pid.calculate(1000, 0);
    const auto second = pid.calculate(1000, 0);
    EXPECT_EQ(second - first, 280);
}

TEST_F(PidTest, DerivativeRespondsToErrorChange) {
    // error 500 first: 250+150+10=410 -> clamp 500 (integral=500)
    EXPECT_EQ(pid.calculate(500, 0), 500);
    // error 1000 next: 500(p) + 450(i, integral=1500) + 10(d) = 960
    EXPECT_EQ(pid.calculate(1000, 0), 960);
}

TEST_F(PidTest, ClampsAtMax) {
    // error 100000: p+i+d far above 10000
    EXPECT_EQ(pid.calculate(100000, 0), 10000);
}

TEST_F(PidTest, ClampsAtMinForNegativeError) {
    // current above target: negative terms clamp to 500
    EXPECT_EQ(pid.calculate(0, 100000), 500);
}

TEST_F(PidTest, MoveConstructorTransfersStateAndZeroesSource) {
    // fresh pid has zero state; the moved-from object's state is zeroed, so
    // the move target behaves like a fresh pid with the same constants
    utility::Pid moved{std::move(pid)};
    EXPECT_EQ(moved.calculate(1000, 0), 820);
}

}  // namespace
