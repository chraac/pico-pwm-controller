#pragma once

// Fake pico/types.h for host unit tests -- see test/stubs/stub_control.hh
#include <stdint.h>

#ifndef PICO_ERROR_GENERIC
#define PICO_ERROR_GENERIC (-1)
#endif
#ifndef PICO_ERROR_NO_DATA
#define PICO_ERROR_NO_DATA (-2)
#endif

// pico/platform.h in the real SDK
#ifndef KHZ
#define KHZ 1000ull
#endif
#ifndef MHZ
#define MHZ (1000ull * KHZ)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int uint;

#ifdef __cplusplus
}
#endif
