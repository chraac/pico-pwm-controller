#pragma once

// Fake hardware/pwm.h for host unit tests. pwm_config mirrors the real
// register-ish struct with a public .top (PwmHelper reads it directly);
// pwm_set_gpio_level records the computed counter level per gpio.
#include "pico/types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum pwm_clkdiv_mode {
    PWM_DIV_FREE_RUNNING = 0,
    PWM_DIV_B_HIGH = 1,
    PWM_DIV_B_RISING = 2,
    PWM_DIV_B_FALLING = 3,
};

typedef struct pwm_config {
    uint8_t csr;
    uint8_t div;
    uint8_t ctr;
    uint32_t top;
} pwm_config_t;

typedef struct pwm_config pwm_config;

pwm_config pwm_get_default_config(void);
void pwm_config_set_phase_correct(pwm_config *c, bool phase_correct);
void pwm_config_set_clkdiv_int(pwm_config *c, uint32_t div);
void pwm_config_set_clkdiv_mode(pwm_config *c, enum pwm_clkdiv_mode mode);
void pwm_config_set_output_polarity(pwm_config *c, bool a, bool b);
void pwm_config_set_wrap(pwm_config *c, uint32_t wrap);

uint pwm_gpio_to_slice_num(uint gpio);
uint pwm_gpio_to_channel(uint gpio);
uint pwm_init(uint slice_num, pwm_config *c, bool start);
void pwm_set_gpio_level(uint gpio, uint32_t level);
void pwm_set_enabled(uint slice_num, bool enabled);

#ifdef __cplusplus
}
#endif
