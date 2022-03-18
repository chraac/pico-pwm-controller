import matplotlib.pyplot as plt
import numpy as np

target_rpm = 1800
Kp = 2  # proportion
Ki = 0.7  # integration
Kd = 0.75  # diff
time_sample = 100
time_length = time_sample
t = np.linspace(0, time_length, time_sample)


# Noctua fan speed module
# https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf
def model_2000rpm(pwm_cycle, denom=100):
    pwm_cycle = pwm_cycle / denom
    if pwm_cycle <= 20:
        return 450
    elif pwm_cycle <= 60:
        return int((pwm_cycle - 20) * 22.5 + 450)
    elif pwm_cycle <= 100:
        return int((pwm_cycle - 60) * 16.25 + 1350)
    else:
        return 2000


class pid:
    def __init__(self, min_val, max_val, dt, kp, ki, kd):
        self._last_error = 0
        self._integral = 0
        self._min = min_val
        self._max = max_val
        self._dt = dt
        self._kp = kp
        self._ki = ki
        self._kd = kd

    def calculate(self, target_val, current_val):
        err = target_val - current_val
        self._integral += err * self._dt
        diff = (err - self._last_error) / self._dt
        self._last_error = err
        ret = int(Kp * err + Ki * self._integral + Kd * diff)
        return min(max(ret, self._min), self._max)


output = []
out_plt = np.linspace(target_rpm, target_rpm, time_sample)
pid_controller = pid(min_val=2000, max_val=10000, dt=1, kp=Kp, ki=Ki, kd=Kd)

for i in range(0, time_sample):
    cycle = pid_controller.calculate(target_rpm, output[-1] if len(output) > 0 else 0)
    rpm = model_2000rpm(cycle)
    print("iteration:{} ,cycle:{},rpm:{}".format(i, cycle, rpm))
    output.append(rpm)

plt.figure('pid')
plt.xlim(0, time_length)
plt.ylim(0, 2 * target_rpm)
plt.plot(t, output)
plt.plot(t, out_plt)
plt.pause(1000)
