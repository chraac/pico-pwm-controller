// GpioFreqencyCounter unit tests. Pin range 0-4 (see stub notes about the
// per-pin static event slots -- each suite owns a disjoint range).
#include <gtest/gtest.h>

#include "frequency_counter.hh"
#include "stub_control.hh"

namespace {

class FrequencyCounterTest : public ::testing::Test {
protected:
    void SetUp() override { pico_stub::reset(); }
};

TEST_F(FrequencyCounterTest, ConstructorEnablesInputAndEdgeRiseIrq) {
    utility::GpioFreqencyCounter counter{0};
    const auto gpio = pico_stub::gpio(0);
    EXPECT_TRUE(gpio.input_enabled);
    EXPECT_TRUE(pico_stub::gpioIrqMask(0) & GPIO_IRQ_EDGE_RISE);
}

TEST_F(FrequencyCounterTest, TwoEdgesPerSecondIs2Hz) {
    utility::GpioFreqencyCounter counter{1};
    pico_stub::fireGpioIrq(1);
    pico_stub::fireGpioIrq(1);
    pico_stub::advanceUs(1'000'000);
    // mHz = count * 1e9 / interval_us = 2 * 1e9 / 1e6 = 2000
    EXPECT_EQ(counter.GetFrequencyMilliHertz(), 2000u);
}

TEST_F(FrequencyCounterTest, NoEdgesIsZero) {
    utility::GpioFreqencyCounter counter{2};
    pico_stub::advanceUs(1'000'000);
    EXPECT_EQ(counter.GetFrequencyMilliHertz(), 0u);
}

TEST_F(FrequencyCounterTest, CountIsConsumedByRead) {
    utility::GpioFreqencyCounter counter{3};
    pico_stub::fireGpioIrq(3);
    pico_stub::advanceUs(1'000'000);
    EXPECT_EQ(counter.GetFrequencyMilliHertz(), 1000u);
    // no new edges: the next window measures zero
    pico_stub::advanceUs(1'000'000);
    EXPECT_EQ(counter.GetFrequencyMilliHertz(), 0u);
}

TEST_F(FrequencyCounterTest, ThousandEdgesPerSecondIs1MHz) {
    utility::GpioFreqencyCounter counter{4};
    for (int i = 0; i < 1000; ++i) {
        pico_stub::fireGpioIrq(4);
    }
    pico_stub::advanceUs(1'000'000);
    EXPECT_EQ(counter.GetFrequencyMilliHertz(), 1'000'000u);
}

TEST_F(FrequencyCounterTest, ResetZeroesCount) {
    utility::GpioFreqencyCounter counter{0};
    pico_stub::fireGpioIrq(0);
    pico_stub::fireGpioIrq(0);
    pico_stub::fireGpioIrq(0);
    counter.Reset();
    pico_stub::advanceUs(1'000'000);
    EXPECT_EQ(counter.GetFrequencyMilliHertz(), 0u);
}

TEST_F(FrequencyCounterTest, IrqOnOtherPinDoesNotCount) {
    utility::GpioFreqencyCounter counter{1};
    pico_stub::fireGpioIrq(2);  // not enabled anyway, and another pin
    pico_stub::advanceUs(1'000'000);
    EXPECT_EQ(counter.GetFrequencyMilliHertz(), 0u);
}

}  // namespace
