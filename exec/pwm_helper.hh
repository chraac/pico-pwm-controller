#pragma once

#ifdef PLATFORM_PICO
#include <hardware/pwm.h>
namespace utility {
using pwm_cfg = pwm_config;
}
#elif defined(PLATFORM_ESP32_C3)
#include <driver/ledc.h>
#endif

#include "base_types.hh"

namespace utility {
constexpr uint kDefaultPwmTop = 9999;
constexpr uint kDefaultCycleDenom = kDefaultPwmTop + 1;

class PwmHelper : public GpioBase {
public:
    PwmHelper(const uint gpio_pin, const uint32_t freq_khz,
              const uint32_t top = kDefaultPwmTop) noexcept;

    PwmHelper(PwmHelper &&other) noexcept;

    void SetDutyCycle(uint32_t num,
                      uint32_t denom = kDefaultCycleDenom) noexcept;

private:
    pwm_cfg pwm_config_;

    void operator=(PwmHelper &&) = delete;
    DISALLOW_COPY(PwmHelper);
};

}  // namespace utility
