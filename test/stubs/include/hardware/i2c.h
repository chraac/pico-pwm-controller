#pragma once

// Fake hardware/i2c.h for host unit tests. Two bus instances exist; tests
// attach fake devices (register files) via pico_stub::i2cAttach(). Transfers
// to an address with no device attached fail (reads fill 0xFF, like a bus
// with only pull-ups).
#include <stddef.h>

#include "pico/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct i2c_inst {
    int instance;  // fake: identity only
} i2c_inst_t;

extern i2c_inst_t *const i2c0;
extern i2c_inst_t *const i2c1;

uint i2c_init(i2c_inst_t *i2c, uint baudrate);
int i2c_write_blocking(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src,
                       size_t len, bool nostop);
int i2c_read_blocking(i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len,
                      bool nostop);

#ifdef __cplusplus
}
#endif
