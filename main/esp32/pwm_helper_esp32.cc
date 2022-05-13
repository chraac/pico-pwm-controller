// clang-format off
#include "pwm_helper.hh"
// clang-format on

#include <driver/ledc.h>

#include <atomic>

#include "logger.hh"

using namespace utility;

namespace {

constexpr uint32_t kResolutionBit = 11;
constexpr uint32_t kInvalidValue = LEDC_TIMER_MAX;
uint32_t timer_frequency_hz[LEDC_TIMER_MAX] = {kInvalidValue, kInvalidValue,
                                               kInvalidValue, kInvalidValue};
std::atomic_uint32_t current_channel_index(0);

ledc_timer_t GetAvailableTimerIndex(const uint32_t freq_hz) {
    for (uint32_t i = 0; i < LEDC_TIMER_MAX; ++i) {
        if (timer_frequency_hz[i] == kInvalidValue ||
            timer_frequency_hz[i] == freq_hz) {
            timer_frequency_hz[i] = freq_hz;
            return ledc_timer_t(i);
        }
    }

    return ledc_timer_t(kInvalidValue);
}

}  // namespace

PwmHelper::PwmHelper(const uint32_t gpio_pin, const uint32_t freq_khz,
                     const uint32_t top) noexcept
    : GpioBase(gpio_pin) {
    const auto timer_idx = GetAvailableTimerIndex(freq_khz * 1000);
    if (timer_idx == ledc_timer_t(kInvalidValue)) {
        log_info("[PwmHelper]Timer unavailable!");
        return;
    }

    timer_config_.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_config_.duty_resolution =
        static_cast<ledc_timer_bit_t>(kResolutionBit);  // LEDC_TIMER_11_BIT
    timer_config_.timer_num = timer_idx;
    timer_config_.freq_hz = freq_khz * 1000;
    timer_config_.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timer_config_);

    channel_config_.gpio_num = gpio_pin;
    channel_config_.speed_mode = timer_config_.speed_mode;
    channel_config_.channel = ledc_channel_t(current_channel_index++);
    channel_config_.intr_type = LEDC_INTR_DISABLE;
    channel_config_.timer_sel = timer_idx;
    channel_config_.duty = 0;
    channel_config_.hpoint = 0;
    ledc_channel_config(&channel_config_);
}

PwmHelper::PwmHelper(PwmHelper &&other) noexcept : GpioBase(other.gpio_pin_) {
    channel_config_ = other.channel_config_;
    timer_config_ = other.timer_config_;
    other.channel_config_ = {};
    other.timer_config_ = {};
}

void PwmHelper::SetDutyCycle(uint32_t num, uint32_t denom) noexcept {
    auto duty = ((1L << kResolutionBit) - 1) * num / denom;
    ledc_set_duty(timer_config_.speed_mode, channel_config_.channel, duty);
    ledc_update_duty(timer_config_.speed_mode, channel_config_.channel);
}
