#pragma once

// Fake hardware/adc.h for host unit tests (reserved for the lite-variant
// tests; nothing host-compiled uses it yet).
#include "pico/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void adc_init(void);
void adc_gpio_init(uint gpio);
void adc_select_input(uint input);
uint16_t adc_read(void);

#ifdef __cplusplus
}
#endif
