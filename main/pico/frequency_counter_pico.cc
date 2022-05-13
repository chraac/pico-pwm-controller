// clang-format off
#include "frequency_counter.hh"
// clang-format on

#include <hardware/gpio.h>
#include <hardware/timer.h>

#include "critical_section_helper.hh"

using namespace utility;

namespace {

constexpr uint32_t kGpioPinCount = 30;
uint32_t event_count_[kGpioPinCount] = {};
CriticalSection event_count_critical_section_;

void GpioEventHandler(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        event_count_critical_section_.Lock();
        ++event_count_[gpio];
        event_count_critical_section_.Unlock();
    }
}

}  // namespace

GpioFreqencyCounter::GpioFreqencyCounter(const uint32_t gpio_pin) noexcept
    : GpioBase(gpio_pin), last_time_us_(time_us_64()) {
    gpio_set_input_enabled(gpio_pin, true);
    gpio_set_irq_enabled_with_callback(gpio_pin, GPIO_IRQ_EDGE_RISE, true,
                                       GpioEventHandler);
}

// Get frequency in MilliHertz
// See also: https://www.convertworld.com/en/frequency/millihertz.html
uint32_t GpioFreqencyCounter::GetFrequencyMilliHertz() noexcept {
    uint32_t count;
    const auto now_us = time_us_64();
    event_count_critical_section_.Lock();
    count = event_count_[gpio_pin_];
    event_count_[gpio_pin_] = 0;
    event_count_critical_section_.Unlock();
    const auto interval_us = (now_us - last_time_us_);
    last_time_us_ = now_us;
    return uint64_t(count) * 1000000 * 1000 / interval_us;
}

void GpioFreqencyCounter::Reset() noexcept {
    event_count_critical_section_.Lock();
    event_count_[gpio_pin_] = 0;
    event_count_critical_section_.Unlock();
    // Clear stale events which might cause immediate spurious handler entry
    // See also:
    // https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_gpio/gpio.c#L160
    gpio_acknowledge_irq(gpio_pin_, GPIO_IRQ_EDGE_RISE);
    last_time_us_ = time_us_64();
}