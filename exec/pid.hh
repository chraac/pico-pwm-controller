#pragma once

#include <algorithm>
#include "base_types.hh"

namespace utility {

    class Pid {
    public:
        typedef int32_t ValueType;
        typedef float FloatType;
        Pid(const ValueType min, const ValueType max, const ValueType dt, FloatType kp, FloatType ki, FloatType kd) noexcept
            : _min(min)
            , _max(max)
            , _dt(dt)
            , _kp(kp)
            , _ki(ki)
            , _kd(kd) { 
        }

        ValueType calculate(ValueType target, ValueType current) {
            if (target == current) {
                _last_error = 0;
                return target;
            }

            // Proportional term
            const auto error = FloatType(target - current);
            const auto p_out = _kp * error;

            // Integral term
            _integral += error * FloatType(_dt);
            const auto i_out = _ki * _integral;


            // Derivative term
            const auto derivative = (error - _last_error) / FloatType(_dt);
            const auto d_out = _kd * derivative;

            _last_error = error;

            // Calculate total output
            auto output = std::max(ValueType(p_out + i_out + d_out), _min);
            output = std::min(output, _max);
            return output;
        }

    private:
        const ValueType _min;
        const ValueType _max;
        const ValueType _dt;
        const FloatType _kp;
        const FloatType _ki;
        const FloatType _kd;
        FloatType _last_error = 0;
        FloatType _integral = 0;

        DISALLOW_COPY(Pid);
        DISALLOW_MOVE(Pid);
    };

}
