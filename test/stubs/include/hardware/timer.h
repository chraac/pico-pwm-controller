#pragma once

// Fake hardware/timer.h for host unit tests. Also declares sleep_ms/sleep_us
// (real SDK exposes them from pico/time.h, which the firmware picks up
// transitively) -- both drive the controllable fake clock.
#include "pico/types.h"

#ifdef __cplusplus
extern "C" {
#endif

uint64_t time_us_64(void);
void sleep_us(uint64_t us);
void sleep_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif
