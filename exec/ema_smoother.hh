#pragma once

#include <algorithm>
#include <cmath>

#include "base_types.hh"

namespace utility {

// Exponential moving average with separate rise/fall rates and an optional
// idle floor the output snaps to instead of slowly decaying towards it
class EmaSmoother {
public:
    // up_rate/down_rate must be in [0, 1] and encode the smoothing time
    // constant of one fixed update period. idle_val is an optional lower
    // bound on the output (e.g. a quiet fan baseline); omit it to disable
    // the clamp.
    // Examples: EmaSmoother(0.25f, 0.05f, 0.0f)
    //           EmaSmoother(0.15f, 0.01f, 35.0f)
    explicit EmaSmoother(float up_rate, float down_rate,
                         float idle_val) noexcept
        : up_ratio_(up_rate),
          down_ratio_(down_rate),
          idle_threshold_(idle_val),
          smoothed_value_(idle_val) {}

    // Blends current_value into the running average and returns it
    float Update(float current_value) {
        // NAN input (failed read): hold the last good value
        if (std::isnan(current_value)) {
            return smoothed_value_;
        }

        // NAN state (first sample): snap instantly
        if (std::isnan(smoothed_value_)) {
            smoothed_value_ = std::max(current_value, idle_threshold_);
            return smoothed_value_;
        }

        // asymmetric ratio: fast rise, slow fall
        const auto ratio =
            (current_value > smoothed_value_) ? up_ratio_ : down_ratio_;
        smoothed_value_ =
            current_value * ratio + smoothed_value_ * (1.0f - ratio);

        // snap to the idle floor instead of decaying towards it for seconds
        smoothed_value_ = std::max(smoothed_value_, idle_threshold_);
        return smoothed_value_;
    }

    // back to uninitialized state
    void Reset() { smoothed_value_ = idle_threshold_; }

private:
    const float up_ratio_;        // blend ratio when rising
    const float down_ratio_;      // blend ratio when falling
    const float idle_threshold_;  // output lower bound
    float smoothed_value_;        // running average, initialized to idle floor

    DISALLOW_COPY(EmaSmoother);
    DISALLOW_MOVE(EmaSmoother);
};

}  // namespace utility
