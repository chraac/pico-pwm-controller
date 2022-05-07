#include "pwm_helper.hh"
#include "logger.hh"

using namespace utility;

namespace {

constexpr uint32_t kInvalidValue = 0xFFFFFFFF;
uint32_t timer_frequency_hz[LEDC_TIMER_MAX] = {kInvalidValue, kInvalidValue,
                                               kInvalidValue, kInvalidValue}

uint32_t
GetAvailableTimerIndex(const uint32_t freq_hz) {
    for (uint32_t i = 0; i < LEDC_TIMER_MAX; ++i) {
        if (timer_frequency_hz[i] == kInvalidValue ||
            timer_frequency_hz[i] == freq_hz) {
            timer_frequency_hz[i] = freq_hz;
            return i;
        }
    }

    return kInvalidValue;
}

}  // namespace

PwmHelper::PwmHelper(const uint gpio_pin, const uint32_t freq_khz,
                     const uint32_t top) noexcept
    : GpioBase(gpio_pin), channel_config_{}, timer_config_{} {
    const auto timer_idx = GetAvailableTimerIndex(freq_hz);
    if (timer_idx == kInvalidValue) {
        log_info("[PwmHelper]Timer unavailable!");
        return;
    }

    timer_config_.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_config_.duty_resolution = LEDC_TIMER_12_BIT;
    timer_config_.timer_num = timer_idx;
    timer_config_.freq_hz = freq_khz * 1000;
    timer_config_.clk_cfg = LEDC_SLOW_CLK_RTC8M;

    channel_config_.gpio_num = PULSE_IO;
    channel_config_.speed_mode = timer_config_.speed_mode;
    channel_config_.channel = timer_idx;
    channel_config_.intr_type = LEDC_INTR_DISABLE;
    channel_config_.timer_sel = timer_idx;
    channel_config_.duty = 0;
    channel_config_.hpoint = top;
}

PwmHelper::PwmHelper(PwmHelper &&other) noexcept : GpioBase(other.gpio_pin_) {
    channel_config_ = other.channel_config_;
    timer_config_ = other.timer_config_;
    other.channel_config_ = {};
    other.timer_config_ = {};
}

void PwmHelper::SetDutyCycle(uint32_t num, uint32_t denom) noexcept {
    auto duty = ((1 << 12) - 1) * num / denom;
    ledc_set_duty(timer_config_.speed_mode, channel_config_.channel, duty);
    ledc_update_duty(timer_config_.speed_mode, channel_config_.channel);
}
