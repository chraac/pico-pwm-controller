// clang-format off
#include "frequency_counter.hh"
// clang-format on

#include <driver/gpio.h>

#include <atomic>
#include <chrono>

using namespace utility;
using system_clock = std::chrono::system_clock;

namespace {

constexpr uint32_t kGpioPinCount = GPIO_PIN_COUNT;
std::atomic_uint32_t event_count_[kGpioPinCount];

void IRAM_ATTR GpioIsrHandler(void* arg) {
    const auto gpio_pin = reinterpret_cast<uint32_t>(arg);
    ++event_count_[gpio_pin];
}

uint64_t GetNowInMs() {
    auto now = system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

gpio_num_t ToEnum(const uint32_t gpio_pin) {
    return static_cast<gpio_num_t>(gpio_pin);
}

}  // namespace

GpioFreqencyCounter::GpioFreqencyCounter(const uint32_t gpio_pin) noexcept
    : GpioBase(gpio_pin), last_time_us_(GetNowInMs()) {
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL << gpio_pin;
    // disable pull-down mode
    io_conf.pull_down_en = static_cast<gpio_pulldown_t>(0);
    // disable pull-up mode
    io_conf.pull_up_en = static_cast<gpio_pullup_t>(0);
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    // change gpio interrupt type for one pin
    gpio_set_intr_type(ToEnum(gpio_pin), GPIO_INTR_POSEDGE);

    // install gpio isr service
    gpio_install_isr_service(0);

    gpio_isr_handler_add(ToEnum(gpio_pin), GpioIsrHandler,
                         reinterpret_cast<void*>(gpio_pin_));
}

// Get frequency in MilliHertz
// See also: https://www.convertworld.com/en/frequency/millihertz.html
uint32_t GpioFreqencyCounter::GetFrequencyMilliHertz() noexcept {
    const auto now_us = GetNowInMs();
    const auto count = event_count_[gpio_pin_].exchange(0);
    const auto interval_us = (now_us - last_time_us_);
    last_time_us_ = now_us;
    return uint64_t(count) * 1000000 * 1000 / interval_us;
}

void GpioFreqencyCounter::Reset() noexcept {
    gpio_isr_handler_remove(ToEnum(gpio_pin_));
    last_time_us_ = GetNowInMs();
    event_count_[gpio_pin_] = 0;
    gpio_isr_handler_add(ToEnum(gpio_pin_), GpioIsrHandler,
                         reinterpret_cast<void*>(gpio_pin_));
}
