#pragma once

// Test-control surface for the fake pico-sdk (test/stubs/stubs.cc).
//
// Statics caveat: production code keeps process-wide statics
// (GpioFreqencyCounter::event_count_ per pin slot, the SystemClock singleton).
// reset() clears all FAKE state, but not those. Tests therefore:
//   - own a disjoint GPIO pin range per suite (documented in each test file),
//   - leave the system clock at its 125 MHz default.
#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

namespace pico_stub {

// Fake clock. Starts at 1'000'000 us: a zero interval would divide by zero
// in GpioFreqencyCounter::GetFrequencyMilliHertz().
void setTimeUs(uint64_t us);
void advanceUs(uint64_t delta);
uint64_t nowUs();

// Fire a GPIO IRQ: invokes the registered callback if the pin has the event
// enabled -- exactly what the tach input does on a rising edge.
void fireGpioIrq(uint pin, uint32_t events = GPIO_IRQ_EDGE_RISE);

uint32_t gpioIrqMask(uint pin);

struct GpioState {
    bool pull_up = false;
    bool pull_down = false;
    bool input_enabled = false;
    bool out_value = false;
    bool dir_out = false;
    int func = -1;  // last gpio_set_function value, -1 = never set
};
GpioState gpio(uint pin);

// PWM recording: last pwm_set_gpio_level / wrap per pin / slice.
uint32_t pwmGpioLevel(uint pin);
uint32_t pwmWrap(uint slice);

// Fake I2C bus devices: a register file plus a write log. Attaching at an
// address pre-fills the INA226 manufacturer/die IDs (regs 0xFE/0xFF).
struct FakeI2cDevice {
    std::map<uint8_t, uint16_t> regs;
    std::vector<std::tuple<uint8_t, uint16_t>> writes;  // (reg, value)
};
void i2cAttach(i2c_inst_t *bus, uint8_t addr);
FakeI2cDevice *i2cFind(i2c_inst_t *bus, uint8_t addr);  // nullptr = detached
uint i2cInitBaud(i2c_inst_t *bus);  // baudrate of the last i2c_init, 0 if none

// System clock for clkdiv math (only visible before the SystemClock
// singleton's first construction).
void setSysClockHz(uint32_t hz);

// Clear ALL fake state. Does not touch production statics (see above).
void reset();

}  // namespace pico_stub
