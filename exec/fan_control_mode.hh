#pragma once

namespace utility {

// fan curve input source + output target; shared by the fan speed managers
// and the lcd drawer
enum class FanControlMode {
    kTempToPwm = 0,
    kTempToRpm,
    kPwrToPwm,
};

}  // namespace utility
