#pragma once

#include <hardware/gpio.h>
#include <hardware/timer.h>

#include <atomic>

#include "base_types.hh"
#include "critical_section_helper.hh"

namespace utility {

constexpr uint32_t kGpioPinCount = 30;

class FanSpeedHelper {
   public:
    FanSpeedHelper(const uint gpio_pin) noexcept : gpio_pin_(gpio_pin) {
        gpio_set_irq_enabled_with_callback(gpio_pin, GPIO_IRQ_EDGE_RISE, true,
                                           &FanSpeedHelper::GpioEventHandler);
    }

    uint32_t GetFanSpeedRpm() noexcept {
        const auto now_us = time_us_64();
        uint32_t count;
        event_count_critical_section_.Lock();
        count = event_count_[gpio_pin_];
        event_count_[gpio_pin_] = 0;
        event_count_critical_section_.Unlock();
        const auto interval_us = (now_us - last_time_us_);
        last_time_us_ = now_us;

        // fan speed [rpm] = frequency [Hz] × 60 ÷ 2
        // See also:
        // https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf
        return uint64_t(count) * 30 * 1000000 / interval_us;
    }

    void Reset() noexcept {
        event_count_critical_section_.Lock();
        event_count_[gpio_pin_] = 0;
        event_count_critical_section_.Unlock();
        last_time_us_ = time_us_64();
    }

    uint GetGpioPin() const noexcept { return gpio_pin_; }

   private:
    static void GpioEventHandler(uint gpio, uint32_t events) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            event_count_critical_section_.Lock();
            ++event_count_[gpio];
            event_count_critical_section_.Unlock();
        }
    }

    static uint32_t event_count_[kGpioPinCount];
    static CriticalSection event_count_critical_section_;

    const uint gpio_pin_;
    uint64_t last_time_us_ = time_us_64();

    DISALLOW_COPY(FanSpeedHelper);
    DISALLOW_MOVE(FanSpeedHelper);
};

uint32_t FanSpeedHelper::event_count_[] = {};
CriticalSection FanSpeedHelper::event_count_critical_section_;

}  // namespace utility
