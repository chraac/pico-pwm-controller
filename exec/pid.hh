#pragma once

#include <algorithm>

#include "base_types.hh"

namespace utility {

class Pid {
public:
    typedef int32_t ValueType;
    typedef float FloatType;
    Pid(const ValueType min, const ValueType max, const ValueType dt,
        FloatType kp, FloatType ki, FloatType kd) noexcept
        : min_(min), max_(max), dt_(dt), kp_(kp), ki_(ki), kd_(kd) {}

    ValueType calculate(ValueType target, ValueType current) {
        // Proportional term
        const auto error = FloatType(target - current);
        const auto p_out = kp_ * error;

        // Integral term
        integral_ += error * FloatType(dt_);
        const auto i_out = ki_ * integral_;

        // Derivative term
        const auto derivative = (error - last_error_) / FloatType(dt_);
        const auto d_out = kd_ * derivative;

        last_error_ = error;

        // Calculate total output
        auto output = std::max(ValueType(p_out + i_out + d_out), min_);
        output = std::min(output, max_);
        return output;
    }

private:
    const ValueType min_;
    const ValueType max_;
    const ValueType dt_;
    const FloatType kp_;
    const FloatType ki_;
    const FloatType kd_;
    FloatType last_error_ = 0;
    FloatType integral_ = 0;

    DISALLOW_COPY(Pid);
    DISALLOW_MOVE(Pid);
};

}  // namespace utility
