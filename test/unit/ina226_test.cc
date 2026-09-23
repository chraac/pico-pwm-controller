// Ina226Device unit tests via the fake i2c bus. Register semantics follow
// docs/ina226_i2c.md and the datasheet math in exec/ina226_helper.hh.
#include <gtest/gtest.h>

#include "ina226_helper.hh"
#include "stub_control.hh"

namespace {

using utility::Ina226Device;

class Ina226Test : public ::testing::Test {
protected:
    void SetUp() override {
        pico_stub::reset();
        pico_stub::i2cAttach(i2c1, 0x40);
        dev_ = pico_stub::i2cFind(i2c1, 0x40);
    }

    // Ina226Device is move-only? No: DISALLOW_COPY/MOVE. Construct in place.
    Ina226Device make_device() { return Ina226Device{i2c1, 5, 4}; }

    pico_stub::FakeI2cDevice *dev_ = nullptr;

    // register numbers from the datasheet
    static constexpr uint8_t kConfig = 0x00;
    static constexpr uint8_t kShuntVoltage = 0x01;
    static constexpr uint8_t kBusVoltage = 0x02;
    static constexpr uint8_t kPower = 0x03;
    static constexpr uint8_t kCurrent = 0x04;
    static constexpr uint8_t kCalibration = 0x05;
    static constexpr uint8_t kMaskEnable = 0x06;
};

TEST_F(Ina226Test, ConstructorSetsUpBusAndPins) {
    auto device = make_device();
    EXPECT_EQ(pico_stub::i2cInitBaud(i2c1), 400000u);
    EXPECT_EQ(pico_stub::gpio(4).func, GPIO_FUNC_I2C);  // sda
    EXPECT_EQ(pico_stub::gpio(5).func, GPIO_FUNC_I2C);  // scl
    EXPECT_TRUE(pico_stub::gpio(4).pull_up);
    EXPECT_TRUE(pico_stub::gpio(5).pull_up);
}

TEST_F(Ina226Test, ProbeSucceedsWithIds) {
    auto device = make_device();
    EXPECT_TRUE(device.Probe());
}

TEST_F(Ina226Test, ProbeFailsWithoutDevice) {
    pico_stub::reset();  // no attach: bus empty, reads return 0xFFFF
    auto device = make_device();
    EXPECT_FALSE(device.Probe());
}

TEST_F(Ina226Test, ConfigureWritesDefaultConfig) {
    auto device = make_device();
    device.Configure();
    // 16x averaging, 1.1ms conversions, shunt+bus continuous
    ASSERT_EQ(dev_->writes.size(), 1u);
    EXPECT_EQ(std::get<0>(dev_->writes[0]), kConfig);
    EXPECT_EQ(std::get<1>(dev_->writes[0]), 0x247Fu);
}

TEST_F(Ina226Test, ResetWritesResetCommand) {
    auto device = make_device();
    device.Reset();
    ASSERT_EQ(dev_->writes.size(), 1u);
    EXPECT_EQ(std::get<0>(dev_->writes[0]), kConfig);
    EXPECT_EQ(std::get<1>(dev_->writes[0]), 0x8000u);
}

TEST_F(Ina226Test, CalibrationFor20A) {
    auto device = make_device();
    device.SetCalibration(20.0f);
    // Current_LSB = 20/32768 A; calib = 0.00512/(LSB * 0.001) + 0.5 = 8389
    ASSERT_EQ(dev_->writes.size(), 1u);
    EXPECT_EQ(std::get<0>(dev_->writes[0]), kCalibration);
    EXPECT_EQ(std::get<1>(dev_->writes[0]), 0x20C5u);  // 8389, fits 15 bits
}

TEST_F(Ina226Test, CalibrationCoarsensWhenOver15Bits) {
    auto device = make_device();
    // 5 A: raw calib = 33554 > 0x7FFF -> double LSB, halve the register
    device.SetCalibration(5.0f);
    ASSERT_EQ(dev_->writes.size(), 1u);
    EXPECT_EQ(std::get<0>(dev_->writes[0]), kCalibration);
    EXPECT_EQ(std::get<1>(dev_->writes[0]), 0x4189u);  // 16777 = 33554/2

    // coarsened LSB = 10/32768 A: power register scales with it
    dev_->regs[kPower] = 2000;
    // watts = 2000 * (10/32768) * 25 = 15.259
    EXPECT_NEAR(device.GetPowerWatts(), 15.258789f, 1e-3f);
}

TEST_F(Ina226Test, PowerWattsFor20ACalibration) {
    auto device = make_device();
    device.SetCalibration(20.0f);
    dev_->regs[kPower] = 2000;
    // watts = 2000 * (20/32768) * 25 = 30.518
    EXPECT_NEAR(device.GetPowerWatts(), 30.517578f, 1e-3f);
}

TEST_F(Ina226Test, PowerIsZeroWithoutCalibration) {
    auto device = make_device();
    dev_->regs[kPower] = 2000;
    EXPECT_FLOAT_EQ(device.GetPowerWatts(), 0.0f);  // LSB still 0
}

TEST_F(Ina226Test, BusVolts) {
    auto device = make_device();
    dev_->regs[kBusVoltage] = 800;
    EXPECT_NEAR(device.GetBusVolts(), 1.0f, 1e-6f);  // 800 * 1.25 mV
}

TEST_F(Ina226Test, ShuntMilliVoltsIsSigned) {
    auto device = make_device();
    dev_->regs[kShuntVoltage] = 0xFFFF;  // -1 as int16
    EXPECT_NEAR(device.GetShuntMilliVolts(), -0.0025f, 1e-7f);
}

TEST_F(Ina226Test, AmpsFromShunt) {
    auto device = make_device();
    dev_->regs[kShuntVoltage] = 0xFFFF;  // -2.5 mV over 1 mOhm
    EXPECT_NEAR(device.GetAmps(), -0.0025f, 1e-7f);
}

TEST_F(Ina226Test, CurrentAmpsIsSigned) {
    auto device = make_device();
    device.SetCalibration(20.0f);
    dev_->regs[kCurrent] = 0x8000;  // -32768 * (20/32768) = -20 A
    EXPECT_NEAR(device.GetCurrentAmps(), -20.0f, 1e-4f);
}

TEST_F(Ina226Test, ConversionReadyFlag) {
    auto device = make_device();
    dev_->regs[kMaskEnable] = 0x0008;
    EXPECT_TRUE(device.IsConversionReady());
    dev_->regs[kMaskEnable] = 0x0000;
    EXPECT_FALSE(device.IsConversionReady());
}

}  // namespace
