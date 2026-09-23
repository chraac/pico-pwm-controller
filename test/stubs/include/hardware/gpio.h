#pragma once

// Fake hardware/gpio.h for host unit tests. Records configuration, keeps one
// global IRQ callback (mirrors the SDK legacy API: last registration wins),
// tests fire edges via pico_stub::fireGpioIrq().
#include "pico/types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum gpio_direction { GPIO_IN = 0, GPIO_OUT = 1 };

enum gpio_function {
    GPIO_FUNC_XIP = 0,
    GPIO_FUNC_SPI = 1,
    GPIO_FUNC_UART = 2,
    GPIO_FUNC_I2C = 3,
    GPIO_FUNC_PWM = 4,
    GPIO_FUNC_SIO = 5,
    GPIO_FUNC_PIO0 = 6,
    GPIO_FUNC_PIO1 = 7,
    GPIO_FUNC_GPCK = 8,
    GPIO_FUNC_USB = 9,
    GPIO_FUNC_NULL = 0xf,
};

#define GPIO_IRQ_EDGE_RISE 0x8u

typedef void (*gpio_irq_callback_t)(uint gpio, uint32_t event_mask);

void gpio_init(uint gpio);
void gpio_set_dir(uint gpio, bool out);
bool gpio_get(uint gpio);
void gpio_put(uint gpio, bool value);
void gpio_pull_up(uint gpio);
void gpio_pull_down(uint gpio);
void gpio_set_input_enabled(uint gpio, bool enabled);
void gpio_set_function(uint gpio, enum gpio_function fn);

void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t event_mask,
                                        bool enabled,
                                        gpio_irq_callback_t callback);
void gpio_acknowledge_irq(uint gpio, uint32_t event_mask);

#ifdef __cplusplus
}
#endif
