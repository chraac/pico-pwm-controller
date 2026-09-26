#pragma once

#include <pico/stdio.h>

// Emulator-build knobs (the INTEGRATION_TEST firmware target used by
// test/integration -- see docs/testing.md). Everything INTEGRATION_TEST-
// specific lives in this ONE header so the firmware sources stay
// #ifdef-free: in normal builds the log line is a no-op and the loop
// interval is the board default.
#ifdef INTEGRATION_TEST
// machine-parsable state line asserted by the scenarios
#define log_integ_test(format, args...) printf(format, ##args)
// 100 ms loop keeps scenarios short in simulated time
constexpr const unsigned kIntegPoolIntervalMs = 100;
#else
#define log_integ_test(format, args...) (void)0
constexpr const unsigned kIntegPoolIntervalMs = 500;  // board default
#endif
