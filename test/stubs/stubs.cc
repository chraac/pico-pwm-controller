// Fake pico-sdk implementation for host unit tests.
// All state lives in one struct; tests drive it via stub_control.hh.
#include "stub_control.hh"

#include <cstring>

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "pico/critical_section.h"

// The two fake bus instances (identity matters: BusIndex maps pointer->bus).
static i2c_inst_t g_i2c0;
static i2c_inst_t g_i2c1;

// Last value handed out by time_us_64(): reads must be strictly monotonic.
static uint64_t g_last_returned_us = 0;

namespace {

struct StubState {
    // clock
    uint64_t now_us = 1'000'000;
    uint32_t sys_clk_hz = 125'000'000;

    // gpio
    pico_stub::GpioState gpio_state[30];
    uint32_t gpio_irq_mask[30] = {};
    gpio_irq_callback_t irq_callback = nullptr;

    // pwm
    uint32_t pwm_gpio_level[30] = {};
    uint32_t pwm_wrap[8] = {};

    // i2c
    uint i2c_baud[2] = {};
    uint8_t i2c_reg_ptr[2] = {};  // last 1-byte write target per bus
    std::map<uint8_t, pico_stub::FakeI2cDevice> i2c_devices[2];
};

StubState g_state;

int BusIndex(const i2c_inst_t *bus) {
    if (bus == &g_i2c0) return 0;
    if (bus == &g_i2c1) return 1;
    return -1;  // foreign pointer: all transfers fail
}

}  // namespace

namespace pico_stub {

void setTimeUs(uint64_t us) { g_state.now_us = us; }
void advanceUs(uint64_t delta) { g_state.now_us += delta; }
uint64_t nowUs() { return g_state.now_us; }

void fireGpioIrq(uint pin, uint32_t events) {
    if (pin < 30 && (g_state.gpio_irq_mask[pin] & events) &&
        g_state.irq_callback) {
        g_state.irq_callback(pin, events);
    }
}

uint32_t gpioIrqMask(uint pin) { return g_state.gpio_irq_mask[pin]; }

GpioState gpio(uint pin) { return g_state.gpio_state[pin]; }

uint32_t pwmGpioLevel(uint pin) { return g_state.pwm_gpio_level[pin]; }
uint32_t pwmWrap(uint slice) { return g_state.pwm_wrap[slice]; }

void i2cAttach(i2c_inst_t *bus, uint8_t addr) {
    const auto bus_idx = BusIndex(bus);
    if (bus_idx < 0) return;
    auto &dev = g_state.i2c_devices[bus_idx][addr];
    // INA226 manufacturer id ('TI') and die id, so Probe() succeeds
    dev.regs[0xFE] = 0x5449;
    dev.regs[0xFF] = 0x2260;
}

FakeI2cDevice *i2cFind(i2c_inst_t *bus, uint8_t addr) {
    const auto bus_idx = BusIndex(bus);
    if (bus_idx < 0) return nullptr;
    auto &devices = g_state.i2c_devices[bus_idx];
    auto it = devices.find(addr);
    return it == devices.end() ? nullptr : &it->second;
}

uint i2cInitBaud(i2c_inst_t *bus) {
    const auto bus_idx = BusIndex(bus);
    return bus_idx < 0 ? 0 : g_state.i2c_baud[bus_idx];
}

void setSysClockHz(uint32_t hz) { g_state.sys_clk_hz = hz; }

void reset() {
    const auto now = g_state.now_us;
    g_state = StubState{};
    g_state.now_us = now;  // keep the clock monotonic across tests
    g_last_returned_us = 0;
}

}  // namespace pico_stub

//
// C API the firmware talks to
//

extern "C" {

// --- timer -------------------------------------------------------------------
// Reads are strictly monotonic: a read colliding with the previous one bumps
// the clock by 1 us, so a frequency read right after construction never
// divides by a zero interval (matches reality, where the crystal never
// stalls) while explicit advanceUs() intervals stay exact.
uint64_t time_us_64(void) {
    if (g_state.now_us <= g_last_returned_us) {
        g_state.now_us = g_last_returned_us + 1;
    }
    g_last_returned_us = g_state.now_us;
    return g_state.now_us;
}
void sleep_us(uint64_t us) { g_state.now_us += us; }
void sleep_ms(uint32_t ms) { g_state.now_us += uint64_t(ms) * 1000; }

// --- critical_section (single-threaded host: no-ops) --------------------------
void critical_section_init(critical_section_t *) {}
void critical_section_deinit(critical_section_t *) {}
void critical_section_enter_blocking(critical_section_t *) {}
void critical_section_exit(critical_section_t *) {}

// --- gpio ----------------------------------------------------------------------
void gpio_init(uint gpio) { (void)gpio; }
void gpio_set_dir(uint gpio, bool out) {
    g_state.gpio_state[gpio].dir_out = out;
}
bool gpio_get(uint gpio) { return g_state.gpio_state[gpio].out_value; }
void gpio_put(uint gpio, bool value) {
    g_state.gpio_state[gpio].out_value = value;
}
void gpio_pull_up(uint gpio) {
    g_state.gpio_state[gpio].pull_up = true;
    g_state.gpio_state[gpio].pull_down = false;
}
void gpio_pull_down(uint gpio) {
    g_state.gpio_state[gpio].pull_down = true;
    g_state.gpio_state[gpio].pull_up = false;
}
void gpio_set_input_enabled(uint gpio, bool enabled) {
    g_state.gpio_state[gpio].input_enabled = enabled;
}
void gpio_set_function(uint gpio, enum gpio_function fn) {
    g_state.gpio_state[gpio].func = int(fn);
}
void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t event_mask,
                                        bool enabled,
                                        gpio_irq_callback_t callback) {
    if (enabled) {
        g_state.gpio_irq_mask[gpio] |= event_mask;
    } else {
        g_state.gpio_irq_mask[gpio] &= ~event_mask;
    }
    // legacy SDK API: one global callback, last registration wins. Every
    // GpioFreqencyCounter registers the same static handler, so this is safe.
    if (callback) {
        g_state.irq_callback = callback;
    }
}
void gpio_acknowledge_irq(uint gpio, uint32_t event_mask) {
    (void)gpio;
    (void)event_mask;
}

// --- pwm ---------------------------------------------------------------------
pwm_config pwm_get_default_config(void) { return pwm_config{}; }
void pwm_config_set_phase_correct(pwm_config *, bool) {}
void pwm_config_set_clkdiv_int(pwm_config *c, uint32_t div) {
    c->div = uint8_t(div & 0xff);
}
void pwm_config_set_clkdiv_mode(pwm_config *, enum pwm_clkdiv_mode) {}
void pwm_config_set_output_polarity(pwm_config *, bool, bool) {}
void pwm_config_set_wrap(pwm_config *c, uint32_t wrap) { c->top = wrap; }

uint pwm_gpio_to_slice_num(uint gpio) { return gpio / 2; }
uint pwm_gpio_to_channel(uint gpio) { return gpio & 1; }
uint pwm_init(uint slice_num, pwm_config *c, bool start) {
    (void)start;
    g_state.pwm_wrap[slice_num] = c->top;
    return slice_num;
}
void pwm_set_gpio_level(uint gpio, uint32_t level) {
    g_state.pwm_gpio_level[gpio] = level;
}
void pwm_set_enabled(uint, bool) {}

// --- clocks --------------------------------------------------------------------
uint32_t clock_get_hz(enum clock_index clk_index) {
    return clk_index == clk_sys ? g_state.sys_clk_hz : 0;
}

// --- i2c ----------------------------------------------------------------------
i2c_inst_t *const i2c0 = &g_i2c0;
i2c_inst_t *const i2c1 = &g_i2c1;

uint i2c_init(i2c_inst_t *i2c, uint baudrate) {
    const auto bus_idx = BusIndex(i2c);
    if (bus_idx >= 0) {
        g_state.i2c_baud[bus_idx] = baudrate;
    }
    return baudrate;
}

int i2c_write_blocking(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src,
                       size_t len, bool nostop) {
    (void)nostop;
    const auto bus_idx = BusIndex(i2c);
    auto *dev = pico_stub::i2cFind(i2c, addr);
    if (bus_idx < 0 || !dev) {
        return PICO_ERROR_GENERIC;
    }
    if (len == 1) {
        // register pointer select (Ina226Device writes it before every read)
        g_state.i2c_reg_ptr[bus_idx] = src[0];
        return int(len);
    }
    if (len == 3) {
        const auto reg = src[0];
        const auto value = uint16_t(uint16_t(src[1]) << 8 | src[2]);
        dev->regs[reg] = value;
        dev->writes.emplace_back(reg, value);
        return int(len);
    }
    return PICO_ERROR_GENERIC;
}

int i2c_read_blocking(i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len,
                      bool nostop) {
    (void)nostop;
    const auto bus_idx = BusIndex(i2c);
    auto *dev = pico_stub::i2cFind(i2c, addr);
    if (bus_idx < 0 || !dev) {
        // nothing on the bus: SDA stays high, reads come back as 0xFF
        std::memset(dst, 0xff, len);
        return PICO_ERROR_GENERIC;
    }
    const uint16_t value = dev->regs[g_state.i2c_reg_ptr[bus_idx]];
    for (size_t i = 0; i < len; ++i) {
        dst[i] = uint8_t(value >> (8 * (len - 1 - i)));  // MSB first
    }
    return int(len);
}

// --- adc -------------------------------------------------------------------------
void adc_init(void) {}
void adc_gpio_init(uint) {}
void adc_select_input(uint) {}
uint16_t adc_read(void) { return 0; }

}  // extern "C"
