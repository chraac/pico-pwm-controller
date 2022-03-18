import collections

import matplotlib.pyplot as plt
import numpy as np

target_rpm = 2000
Kp = 0.5  # proportion
Ki = 0.3  # integration
Kd = 0.02  # diff
time_sample = 200
time_length = time_sample
t = np.linspace(0, time_length, time_sample)


class fan_model_base:
    def __init__(self, dic):
        assert len(dic) > 0
        dic = collections.OrderedDict(sorted(dic.items()))
        self._keys = list(dic.keys())
        self._values = list(dic.values())

    def get_rpm(self, pwm_cyc):
        return int(np.interp(pwm_cyc, self._keys, self._values))


# Noctua fan speed module
# https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf
class noctua_fan(fan_model_base):
    def __init__(self):
        super(noctua_fan, self).__init__({20: 450, 60: 1350, 100: 2000})


# Nidec D1225C fan
# http://www.nidec-servo.com/en/digital/pdf/D1225C_hi.pdf
class nidec_fan(fan_model_base):
    def __init__(self):
        super(nidec_fan, self).__init__(
            {10: 1000, 20: 2200, 30: 3000, 40: 3600, 50: 4200, 60: 4500, 70: 4700, 80: 4900, 100: 5400})


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
pid_controller = pid(min_val=0, max_val=10000, dt=1, kp=Kp, ki=Ki, kd=Kd)
#fan_model = noctua_fan()
fan_model = nidec_fan()

for i in range(0, time_sample):
    cycle = pid_controller.calculate(target_rpm, output[-1] if len(output) > 0 else 0)
    rpm = fan_model.get_rpm(cycle / 100)
    print("iteration:{} ,cycle:{},rpm:{}".format(i, cycle, rpm))
    output.append(rpm)

plt.figure('pid')
plt.xlim(0, time_length)
plt.ylim(0, 2 * target_rpm)
plt.plot(t, output)
plt.plot(t, out_plt)
plt.pause(1000)
