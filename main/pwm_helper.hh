#pragma once

#ifdef PLATFORM_PICO
#include <hardware/pwm.h>
#elif defined(PLATFORM_ESP32_C3)
#include <driver/ledc.h>
#endif

#include "base_types.hh"

namespace utility {
constexpr uint32_t kDefaultPwmTop = 9999;
constexpr uint32_t kDefaultCycleDenom = kDefaultPwmTop + 1;

class PwmHelper : public GpioBase {
public:
    PwmHelper(const uint32_t gpio_pin, const uint32_t freq_khz,
              const uint32_t top = kDefaultPwmTop) noexcept;

    PwmHelper(PwmHelper &&other) noexcept;

    void SetDutyCycle(uint32_t num,
                      uint32_t denom = kDefaultCycleDenom) noexcept;

private:
#ifdef PLATFORM_PICO
    pwm_config pwm_config_;
#elif defined(PLATFORM_ESP32_C3)
    ledc_channel_config_t channel_config_;
    ledc_timer_config_t timer_config_;
#endif

    void operator=(PwmHelper &&) = delete;
    DISALLOW_COPY(PwmHelper);
};

}  // namespace utility
