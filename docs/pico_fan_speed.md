# Fan Speed (Tach → RPM) Reference (Pico C SDK)

How this project reads fan speed from a 4-wire fan's tachometer signal and
turns it into RPM — bare-metal with the Pico C SDK, no Arduino library.
Covers the signal itself, the measurement chain in
[`FanSpeedHelper`](../exec/fan_speed_helper.hh) /
[`GpioFreqencyCounter`](../exec/frequency_counter.hh), the math and its
resolution limits, and how the value is consumed (PID loop, LCD, USB log).

- Noctua PWM specifications white paper (tach: 2 pulses/rev, open drain):
  <https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf>
- SDK `hardware_gpio` API docs (IRQ callbacks):
  <https://raspberrypi.github.io/pico-sdk-doxygen/html/group__hardware__gpio.html>
- Companion doc: [pico_pwm_settings.md](pico_pwm_settings.md) (PWM side, pin/slice maps)

## 1. The tach signal

A 4-wire fan's 3rd wire is a **tachometer output**: the fan outputs
**2 pulses per revolution** (Noctua convention; some vendors use 1 or 4 —
check the fan datasheet if RPM reads wrong by a factor). The output is
**open drain / open collector**, so it only pulls low; a **pull-up resistor
is required** to see the pulses:

```text
3.3V ──[pull-up ~10k]──┬──► Pico GPIO (input + rising-edge IRQ)
                       │
                 fan tach (open drain, 2 pulses/rev)
```

- Pull up to **3.3 V** — RP2040/RP2350 GPIOs are **not 5 V tolerant**.
  (The Noctua paper allows 5–12 V pull-ups; use 3.3 V for a direct Pico
  connection or add a level shifter.)
- The Pico's internal pull-ups (~50–80 kΩ) work electrically but give slow
  rise times; an external ~10 kΩ is preferable at high RPM.

Frequency → speed:

```text
rpm = f_tach[Hz] × 60 / 2 = f_tach[Hz] × 30
```

Example: 1900 RPM → 63.3 Hz tach → ~127 edges/s.

## 2. Measurement chain in this project

```text
tach pin ─► GPIO rising-edge IRQ ─► event_count_[pin]++ (critical section)
                    │
read (every kPoolIntervalMs = 400 ms):
  GetFrequencyMilliHertz()  → count × 1e9 / interval_us   [mHz, read-and-clear]
  GetFanSpeedRpm()          → mHz × 30 / 1000             [rpm]
```

Pulse counting, not the PWM block — tach pins never occupy a PWM slice
(see [pico_pwm_settings.md §6](pico_pwm_settings.md)):

```cpp
// exec/frequency_counter.cc
gpio_set_input_enabled(gpio_pin, true);
gpio_set_irq_enabled_with_callback(gpio_pin, GPIO_IRQ_EDGE_RISE, true,
                                   &GpioFreqencyCounter::GpioEventHandler);
```

- One **shared static callback** for all pins; per-pin counts live in
  `event_count_[gpio]`, guarded by a `CriticalSection` (the ISR runs in IRQ
  context, the reader on the main loop).
- `GetFrequencyMilliHertz()` is **read-and-clear**: it returns the count
  accumulated *since the previous call*, resets it to 0, and re-anchors the
  time base (`last_time_us_`).

```cpp
// exec/fan_speed_helper.hh
uint32_t GetFanSpeedRpm() noexcept {
    // fan speed [rpm] = frequency [Hz] × 60 ÷ 2
    return freq_counter_.GetFrequencyMilliHertz() * 30 / 1000;
}
```

Integer milli-Hertz keeps the math in `uint32_t`/`uint64_t` — no float, no
drift — and preserves sub-Hz resolution for the RPM conversion.

## 3. Resolution: one pulse is one step

The counter counts *whole edges* in the poll window, so the smallest
non-zero reading is one pulse per interval:

```text
step_rpm = 60 / 2 / interval_s = 30 × 1000 / kPoolIntervalMs
```

That constant exists in the code as `kFanSpeedStepRpm`
([fan_speed_manager.cc](../exec/fan_speed_manager.cc)):

| Poll interval                           | RPM step | Pulses per read @ 1900 RPM |
| --------------------------------------- | -------- | -------------------------- |
| 100 ms                                  | 300      | ~6                         |
| 200 ms                                  | 150      | ~13                        |
| **400 ms** (current, `kPoolIntervalMs`) | **75**   | ~25                        |

Worked example at the project default: 1900 RPM = 63.3 Hz → 25.33 edges per
400 ms window → the counter sees 25 or 26 depending on phase →
**1875 or 1950 RPM**. The reading always lands on multiples of 75.

Consequences:

- The ±60 RPM target band (`kRpmTolerance`) plus `kFanSpeedStepRpm` define
  the skip-PID window `kMaxTargetRpm = 1900 + 75` — tuned so one quantisation
  step inside the band still counts as "at target".
- Below ~300 RPM the window catches 0–1 pulses per read; readings toggle
  between 0 and 75 and the PID is effectively blind (it skips on 0 anyway,
  see §4).
- Faster polling shrinks the step but starves the count (1 pulse/100 ms =
  300 RPM step — worse, not better). 400 ms is a reasonable middle ground
  for 1000–3000 RPM fans.

## 4. Reading the speed

In firmware:

```cpp
SingleFanSpeedManager fan{kPwm0Pin, kFanSpd0Pin, /*use_temp=*/true};
fan.GetFanSpeedRpm();          // current RPM (read-and-clear, see §7)
fan.Next(temp);                // control step; returns the RPM it acted on
```

`Next()` behaviour depends on the mode:

- **RPM mode** (PID): 0 RPM (stalled fan / no signal) → skip the PID and
  reset duty to the 5 % start cycle; inside the tolerance band → skip;
  otherwise `pid_.calculate(target_rpm, current_speed)` sets the duty.
- **Temperature mode**: duty comes from the temp curve; the tach value is
  still measured and reported, just not fed back.

For the human:

- **USB CDC log** (Debug/Release `log_info`, one line per fan every 400 ms):
  `fan.pwm_gpio.<pin>.rpm.<value>` — e.g. `fan.pwm_gpio.3.rpm.1900`.
- **SSD1306 LCD**: each fan tile shows current RPM (and duty % or target).

## 5. Pin map (tach inputs)

**Lite variant, board v93** ([pwm_controller_lite.cc](../exec/pwm_controller_lite.cc)):

| Fan | Tach pin | Fan | Tach pin |
| --- | -------- | --- | -------- |
| 0   | GP4      | 2   | GP28     |
| 1   | GP1      | 3   | GP0      |

**Lite variant, board v90**: tach on GP4, GP1, GP27, GP29.

**Mux variant** ([pwm_controller.cc](../exec/pwm_controller.cc)): one shared
tach input on **GP13**; see §6.

All tach pins are plain IRQ inputs — no function-select conflict with the
PWM pins (GP3, GP2, GP27, GP29).

## 6. Mux variant: 12 fans, one tach pin

`FanSpeedManagerWithSelector` routes 12 fan tach lines through an external
analog multiplexer into the single GP13 input. Four Pico GPIOs (GP8–GP11,
bit0..bit3) drive the mux select bits — via internal **pull-up/pull-down**,
not push-pull output:

```cpp
// exec/fan_speed_helper.hh — FanSpeedSelector
gpio_pull_up(gpio_pin_bitX_)   // bit = 1
gpio_pull_down(gpio_pin_bitX_) // bit = 0
sleep_ms(1);                   // wait for the analog switch to settle
```

The mux settle time is why `sleep_ms(1)` (UTC analog switch datasheet:
<http://www.utc-ic.com/uploadfile/2011/0923/20110923124731897.pdf>).

Each `Next()` polls one fan and is a full sequence:

1. Read `GetFanSpeedRpm()` — the count belongs to the *previously selected*
   fan (it had a full 400 ms to accumulate).
2. Advance `current_fan_` round-robin over `kFanIndexArray`
   ({0,1,2,3,4,5,8,9,10,11,12,13} — the board's wiring skips indices 6, 7),
   select it, and `speed_helper_.Reset()`.
3. `Reset()` zeroes the counter **and acknowledges any stale IRQ**
   (`gpio_acknowledge_irq`) so a pending edge from the previous fan doesn't
   leak into the next fan's window — then restarts the time base.
4. Per PWM group (3 fans share one PWM channel): the PID acts on the
   **maximum** RPM seen in the group, once per full group sweep.

Resolution note: each fan is sampled every `kFanCount × kPoolIntervalMs` =
4.8 s, but its *window* is still one 400 ms slice — so the step stays 75 RPM,
just sparsely sampled (12 of 12 fans per 4.8 s).

## 7. Gotchas

- **`GetFanSpeedRpm()` is read-and-clear and re-anchors the time base.**
  Call it exactly once per poll tick; a second call returns ~0 and corrupts
  the interval for the next one. `Next()`/`FanSpeedManagerWithSelector` are
  the only sanctioned callers.
- **No pull-up is enabled in code.** `GpioFreqencyCounter` only does
  `gpio_set_input_enabled()` — the board must supply the tach pull-up
  (3.3 V). An un-pulled open-drain tach floats and reads noise/0.
- **Rising edges are counted verbatim** — no debounce/filter. Hall-effect
  tach outputs are clean, but long unshielded leads or PWM coupling can
  double-count and inflate RPM.
- **Quantisation is 75 RPM** at the current 400 ms poll (§3); don't expect
  finer granularity, and don't tighten `kRpmTolerance` below one step.
- `event_count_` is sized `kGpioPinCount = 30`. RP2350 in the larger
  packages exposes GPIO30–47 — a tach pin ≥ 30 there would index out of
  bounds. (Current pin maps all stay below 30.)
- `gpio_set_irq_enabled_with_callback` installs **one process-wide
  callback** (the last call wins) and enables the IRQ on the **calling
  core** — construct all counters on the same core that runs the poll loop
  (core 0 here), and don't install unrelated GPIO callbacks elsewhere.
- **0 RPM is overloaded**: it means no pulses — stalled fan, fan switched
  off by 0 % duty, *or* missing pull-up/wrong pin. The PID's response
  (reset to start duty) is only correct for the first two.
