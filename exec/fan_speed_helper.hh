#pragma once

#include <hardware/gpio.h>
#include <hardware/timer.h>

#include <atomic>

#include "base_types.hh"
#include "critical_section_helper.hh"

namespace utility {

constexpr uint32_t kGpioPinCount = 30;

class GpioFreqencyCounter : public GpioBase {
public:
    GpioFreqencyCounter(const uint gpio_pin) noexcept : GpioBase(gpio_pin) {
        gpio_set_irq_enabled_with_callback(
            gpio_pin, GPIO_IRQ_EDGE_RISE, true,
            &GpioFreqencyCounter::GpioEventHandler);
    }

    uint32_t GetFrequencyHz() noexcept {
        uint32_t count;
        const auto now_us = time_us_64();
        event_count_critical_section_.Lock();
        count = event_count_[gpio_pin_];
        event_count_[gpio_pin_] = 0;
        event_count_critical_section_.Unlock();
        const auto interval_us = (now_us - last_time_us_);
        last_time_us_ = now_us;
        return uint64_t(count) * 1000000 / interval_us;
    }

    void Reset() noexcept {
        event_count_critical_section_.Lock();
        event_count_[gpio_pin_] = 0;
        event_count_critical_section_.Unlock();
        last_time_us_ = time_us_64();
    }

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

    uint64_t last_time_us_ = time_us_64();

    DISALLOW_COPY(GpioFreqencyCounter);
    DISALLOW_MOVE(GpioFreqencyCounter);
};

uint32_t GpioFreqencyCounter::event_count_[] = {};
CriticalSection GpioFreqencyCounter::event_count_critical_section_;

class FanSpeedHelper : public GpioBase {
public:
    FanSpeedHelper(const uint gpio_pin) noexcept : freq_counter_(gpio_pin) {}

    uint32_t GetFanSpeedRpm() noexcept {
        // fan speed [rpm] = frequency [Hz] × 60 ÷ 2
        // See also:
        // https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf
        return freq_counter_.GetFrequencyHz() * 30;
    }

    void Reset() noexcept { freq_counter_.Reset(); }

private:
    GpioFreqencyCounter freq_counter_;

    DISALLOW_COPY(FanSpeedHelper);
    DISALLOW_MOVE(FanSpeedHelper);
};

}  // namespace utility
