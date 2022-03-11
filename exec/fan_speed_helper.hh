#pragma once

#include <atomic>
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "base_types.hh"

namespace utility {

    constexpr uint32_t kGpioPinCount = 30; 

    class FanSpeedHelper {
    public:
        FanSpeedHelper(const uint gpio_pin) noexcept
            :_gpio_pin(gpio_pin) {
            gpio_set_irq_enabled_with_callback(gpio_pin, GPIO_IRQ_EDGE_RISE, 
                true, &FanSpeedHelper::GpioEventHandler);
        }

        uint32_t GetFanSpeedRpm() noexcept {
            const auto now_us = time_us_64();
            const auto count = _event_count[_gpio_pin].exchange(0);
            const auto interval_us = (now_us - _last_time_us);
            _last_time_us = now_us;

            // fan speed [rpm] = frequency [Hz] × 60 ÷ 2
            // See also: https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf
            return uint64_t(count) * 30 * 1000000 / interval_us;
        }

        void Reset() noexcept {
            _last_time_us = time_us_64();
        }

    private:
        static void GpioEventHandler(uint gpio, uint32_t events) {
            if (events & GPIO_IRQ_EDGE_RISE) {
                ++_event_count[gpio];
            }
        }

        static std::atomic_uint32_t _event_count[kGpioPinCount];

        const uint _gpio_pin;
        uint64_t _last_time_us = time_us_64();

        DISALLOW_COPY(FanSpeedHelper);
        DISALLOW_MOVE(FanSpeedHelper); 
    };

    std::atomic_uint32_t FanSpeedHelper::_event_count[] = {};

}
