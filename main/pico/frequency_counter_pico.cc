// clang-format off
#include "frequency_counter.hh"
// clang-format on

#include <hardware/gpio.h>
#include <hardware/timer.h>

#include "critical_section_helper.hh"

using namespace utility;

namespace {

constexpr uint8_t kAw9253Addr0 = 0;
constexpr uint8_t kAw9253Addr1 = 0;
constexpr uint32_t kGpioPinCount = 30;
Aw9523Helper *helper_array_[kGpioPinCount] = {};
uint32_t event_count_[kGpioPinCount] = {};
CriticalSection event_count_critical_section_;

void GpioEventHandler(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        event_count_critical_section_.Lock();
        ++event_count_[gpio];
        event_count_critical_section_.Unlock();
    }
}

void Aw9523bEventHandler(uint gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_FALL)) {
        return;
    }

    event_count_critical_section_.Lock();
    auto *aw9523 = helper_array_[gpio];
    if (!aw9523) {
        event_count_critical_section_.Unlock();
        return;
    }

    uint16_t value = (uint16_t(aw9523->ReadPort(Aw9523Helper::kPort1)) << 8) |
                     aw9523->ReadPort(Aw9523Helper::kPort0);
    for (size_t i = 0; i < Aw9523Helper::kGpioCount; ++i, value = value >> 1) {
        if (~(value & 1)) {
            ++event_count_[i];
        }
    }

    event_count_critical_section_.Unlock();
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

Aw9523bFreqencyCounter::Aw9523bFreqencyCounter(const uint32_t gpio_scl,
                                               const uint32_t gpio_sda,
                                               const uint32_t gpio_ad0,
                                               const uint32_t gpio_ad1,
                                               const uint32_t gpio_intr,
                                               const uint32_t gpio_rst) noexcept
    : aw9523_(gpio_scl, gpio_sda, gpio_intr, kAw9253Addr0, kAw9253Addr1) {
    constexpr auto SetGpioValue = [](const uint32_t gpio,
                                     const bool is_pullup) {
        if (is_pullup) {
            gpio_pull_up(gpio);
        } else {
            gpio_pull_down(gpio);
        }
    };

    gpio_set_dir(gpio_ad0, GPIO_OUT);
    SetGpioValue(gpio_ad0, kAw9253Addr0);
    gpio_set_dir(gpio_ad1, GPIO_OUT);
    SetGpioValue(gpio_ad1, kAw9253Addr1);

    gpio_set_dir(gpio_rst, GPIO_OUT);
    SetGpioValue(gpio_rst, true);

    event_count_critical_section_.Lock();
    helper_array_[gpio_intr] = &aw9523_;
    event_count_critical_section_.Unlock();
    gpio_set_dir(gpio_intr, GPIO_IN);
    gpio_set_input_enabled(gpio_intr, true);
    gpio_set_irq_enabled_with_callback(gpio_intr, GPIO_IRQ_EDGE_FALL, true,
                                       Aw9523bEventHandler);

    aw9523_.Reset();
    aw9523_.SetDirection(Aw9523Helper::kPort0, Aw9523Helper::kDirectionInput);
    aw9523_.SetDirection(Aw9523Helper::kPort1, Aw9523Helper::kDirectionInput);
    aw9523_.SetGpioMode(Aw9523Helper::kPort0, Aw9523Helper::kModeGpio);
    aw9523_.SetGpioMode(Aw9523Helper::kPort1, Aw9523Helper::kModeGpio);
    aw9523_.SetEnableInterrupt(Aw9523Helper::kPort0, true);
    aw9523_.SetEnableInterrupt(Aw9523Helper::kPort1, true);
}

// Get frequency in MilliHertz
// See also: https://www.convertworld.com/en/frequency/millihertz.html
uint32_t Aw9523bFreqencyCounter::GetFrequencyMilliHertz(FreqPin pin) noexcept {
    uint32_t count;
    const auto now_us = time_us_64();
    event_count_critical_section_.Lock();
    count = event_count_[pin];
    event_count_[pin] = 0;
    event_count_critical_section_.Unlock();
    const auto interval_us = (now_us - last_time_us_);
    last_time_us_ = now_us;
    return uint64_t(count) * 1000000 * 1000 / interval_us;
}

void Aw9523bFreqencyCounter::Reset(FreqPin pin) noexcept {
    event_count_critical_section_.Lock();
    event_count_[pin] = 0;
    event_count_critical_section_.Unlock();
    last_time_us_ = time_us_64();
}
