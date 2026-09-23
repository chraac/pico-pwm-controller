// EmaSmoother unit tests -- pure logic, no stubs needed. Constants mirror
// the pcie main's power tracking (exec/pwm_controller_pcie.cc):
// EmaSmoother(0.4f, 0.06f, 5.0f) -- fast rise, slow fall, 5W idle floor.
#include <cmath>

#include <gtest/gtest.h>

#include "ema_smoother.hh"

namespace {

class EmaSmootherTest : public ::testing::Test {
protected:
    utility::EmaSmoother smoother{0.4f, 0.06f, 5.0f};
};

TEST_F(EmaSmootherTest, StartsAtIdleFloor) {
    // first sample below the floor: snaps to the floor, no blend
    EXPECT_FLOAT_EQ(smoother.Update(1.0f), 5.0f);
}

TEST_F(EmaSmootherTest, FirstSampleAboveFloorSnaps) {
    // first sample above the floor blends from the floor (not from zero):
    // 50 * 0.4 + 5 * 0.6 = 23
    EXPECT_FLOAT_EQ(smoother.Update(50.0f), 23.0f);
}

TEST_F(EmaSmootherTest, RisesWithUpRate) {
    EXPECT_FLOAT_EQ(smoother.Update(50.0f), 23.0f);
    EXPECT_FLOAT_EQ(smoother.Update(50.0f), 33.8f);
    EXPECT_FLOAT_EQ(smoother.Update(50.0f), 40.28f);
}

TEST_F(EmaSmootherTest, ConvergesGeometrically) {
    smoother.Update(50.0f);
    for (int i = 0; i < 20; ++i) {
        smoother.Update(50.0f);
    }
    EXPECT_NEAR(smoother.Update(50.0f), 50.0f, 0.01f);
}

TEST_F(EmaSmootherTest, FallsWithDownRate) {
    smoother.Update(50.0f);  // state 23
    // step down to 20 (below state): 20*0.06 + 23*0.94 = 22.82
    EXPECT_FLOAT_EQ(smoother.Update(20.0f), 22.82f);
}

TEST_F(EmaSmootherTest, OutputNeverBelowIdleFloor) {
    smoother.Update(50.0f);
    // far below the floor: raw blend = 1*0.06 + 50*0.94 = 47.06... then a
    // long decay still stays above 5; a tiny value snaps to the floor
    for (int i = 0; i < 200; ++i) {
        smoother.Update(1.0f);
    }
    EXPECT_FLOAT_EQ(smoother.Update(1.0f), 5.0f);
}

TEST_F(EmaSmootherTest, NanInputHoldsLastValue) {
    smoother.Update(50.0f);  // state 23
    const auto held = smoother.Update(std::nanf(""));
    EXPECT_FLOAT_EQ(held, 23.0f);
}

TEST_F(EmaSmootherTest, ResetReturnsToIdleFloor) {
    smoother.Update(50.0f);
    smoother.Reset();
    EXPECT_FLOAT_EQ(smoother.Update(1.0f), 5.0f);
    // after Reset the next above-floor sample blends from the floor again
    EXPECT_FLOAT_EQ(smoother.Update(50.0f), 23.0f);
}

}  // namespace
