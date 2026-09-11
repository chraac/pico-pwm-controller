#pragma once

#include <cmath>  // Required for std::isnan

namespace utility {
class EmaSmoothing {
private:
    float smoothed_value;
    const float up_ratio;
    const float down_ratio;
    const float idle_threshold;  // Fixed lower bound for quiet desktop idle

public:
    // Constructor initializes const members and sets initial value to NAN.
    // up_rate/down_rate must be in [0, 1]. idle_val is an optional lower
    // bound on the output (e.g. a quiet fan baseline); omit it to disable
    // the clamp.
    // Examples: EmaSmoothing(0.25f, 0.05f)
    //           EmaSmoothing(0.15f, 0.01f, 35.0f)
    EmaSmoothing(float up_rate, float down_rate, float idle_val = -INFINITY)
        : smoothed_value(NAN),
          up_ratio(up_rate),
          down_ratio(down_rate),
          idle_threshold(idle_val) {}

    // Blends current_value into the running average and returns it.
    // Assumes a fixed update period — the ratios encode the smoothing
    // time constant of that period.
    float update(float current_value) noexcept {
        // NAN input (failed read): hold the last good value
        if (std::isnan(current_value)) {
            return smoothed_value;
        }

        // If NAN, snap instantly to the current value
        if (std::isnan(smoothed_value)) {
            smoothed_value = current_value;
            // Respect the idle floor on the first sample too
            if (smoothed_value < idle_threshold) {
                smoothed_value = idle_threshold;
            }
            return smoothed_value;
        }

        // Choose ratio based on direction
        float ratio = (current_value > smoothed_value) ? up_ratio : down_ratio;

        // Core EMA formula
        smoothed_value =
            (current_value * ratio) + (smoothed_value * (1.0f - ratio));

        // Quiet Optimization: If the smoothed trend sinks below your idle
        // threshold, snap directly to it so the fan drops to its quietest
        // baseline instantly instead of lazily decaying for 20 seconds.
        if (smoothed_value < idle_threshold) {
            smoothed_value = idle_threshold;
        }

        return smoothed_value;
    }

    // Reset back to uninitialized state
    void reset() noexcept { smoothed_value = NAN; }
};

}  // namespace utility