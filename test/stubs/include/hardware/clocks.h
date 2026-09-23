#pragma once

// Fake hardware/clocks.h for host unit tests (system_clock.hh reads clk_sys
// once, at singleton construction -- see stub notes about statics).
#include "pico/types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum clock_index {
    clk_gpout0 = 0,
    clk_gpout1 = 1,
    clk_gpout2 = 2,
    clk_gpout3 = 3,
    clk_ref = 4,
    clk_sys = 5,
    clk_peri = 6,
    clk_usb = 7,
    clk_adc = 8,
    clk_rtc = 9,
};

uint32_t clock_get_hz(enum clock_index clk_index);

#ifdef __cplusplus
}
#endif
