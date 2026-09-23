# PWM Settings Reference (Pico C SDK)

Reference for configuring the RP2040/RP2350 PWM peripheral for 4-wire fan
control, bare-metal style with the Pico C SDK (`hardware/pwm.h`), no Arduino
library. Covers how [`PwmHelper`](../exec/pwm_helper.hh) maps onto the
hardware, what the current settings actually produce, and the frequency vs
resolution trade-offs.

- RP2040 datasheet §4.5 (PWM), divider/register details around p. 553:
  <https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf>
- RP2350 datasheet (PWM): <https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf>
- SDK `hardware_pwm` API docs: <https://raspberrypi.github.io/pico-sdk-doxygen/html/group__hardware__pwm.html>
- Noctua PWM specifications white paper (25 kHz, 21–28 kHz window):
  <https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf>

## 1. Hardware: slices and channels

The PWM block is organised as **slices**, each with a 16-bit counter and two
output **channels** (A and B):

| Chip  | Slices | PWM-capable GPIOs       | Notes                                    |
| ----- | ------ | ----------------------- | ---------------------------------------- |
| RP2040 | 8     | GPIO0–GPIO29 (all 30)   | GPIO16–29 **alias** slices 0–6 again     |
| RP2350 | 12    | GPIO0–GPIO23 (24 pins)  | no aliasing; GPIO24+ have no PWM function |

Mapping (`pwm_gpio_to_slice_num()` / `pwm_gpio_to_channel()` in the SDK):

- channel = `gpio & 1`
- slice = `(gpio >> 1) % 8` on RP2040, `gpio >> 1` on RP2350 (GPIO ≤ 23)

RP2040 mapping for the pins this project cares about (datasheet GPIO
function table):

| GPIO | slice.channel | GPIO | slice.channel |
| ---- | ------------- | ---- | ------------- |
| GP0  | 0A            | GP17 | 0B (aliases GP1)  |
| GP1  | 0B            | GP25 | 4B (aliases GP9)  |
| GP2  | 1A            | GP26 | 5A (aliases GP10) |
| GP3  | 1B            | GP27 | 5B (aliases GP11) |
| GP4  | 2A            | GP28 | 6A (aliases GP12) |
| GP7  | 3B            | GP29 | 6B (aliases GP13) |

**Shared per slice:** the counter, `WRAP` and `CLKDIV` — so both channels of a
slice must run at the *same frequency and resolution*.
**Independent per channel:** `CC` (compare level) — the duty cycle.

## 2. The three knobs

With `PWM_DIV_FREE_RUNNING` the counter counts 0 → wrap → 0 and the output is
**high while counter < CC** (edge-aligned PWM):

```
f_pwm = f_sys / (clkdiv × (wrap + 1))
steps = wrap + 1                 // duty-cycle resolution, 16-bit counter → max 65536
duty  = CC / (wrap + 1)          // CC = 0 → always low, CC ≥ wrap+1 → always high
```

- `clkdiv` (`DIV` register): 8-bit integer + 4-bit fraction (1/16 steps).
  A value of **0 or 1 both mean divide by 1** (datasheet, `DIV.INT`).
- `wrap` (`TOP` register): 16-bit, 0–65535.
- `CC` (`CSR`/`CC` compare): 16-bit per channel, written through
  `pwm_set_gpio_level()`.

## 3. Project defaults (PwmHelper)

```cpp
// exec/pwm_helper.hh
constexpr uint kDefaultPwmTop = 9999;      // wrap  → 10000 duty steps
constexpr uint kDefaultCycleDenom = kDefaultPwmTop + 1;

pwm_config_set_clkdiv_int(&pwm_config_,
    SystemClock::GetInstance().GetClockKhz() / freq_khz / top);   // integer!
pwm_config_set_clkdiv_mode(&pwm_config_, PWM_DIV_FREE_RUNNING);
pwm_config_set_wrap(&pwm_config_, top);
pwm_init(slice_num, &pwm_config_, true);
gpio_set_function(gpio_pin_, GPIO_FUNC_PWM);
```

| Setting              | Value        | Defined in                                      |
| -------------------- | ------------ | ----------------------------------------------- |
| PWM frequency target | 25 kHz       | `kPwmFreqKhz`, [fan_speed_manager.hh](../exec/fan_speed_manager.hh) |
| wrap (`top`)         | 9999         | `kDefaultPwmTop`, pwm_helper.hh                 |
| duty denominator     | 10000        | `kDefaultCycleDenom`, pwm_helper.hh             |
| start duty           | 500 (5 %)    | `kStartCycle`, fan_speed_manager.cc             |
| PID duty clamp       | 500–10000    | Pid output limits, fan_speed_manager.cc         |

Duty cycle is set as a fraction `num/denom` and scaled onto the counter:

```cpp
pwm_set_gpio_level(gpio_pin_, num * top / denom);
// num = 500,  denom = 10000 → CC = 499  → high 500/10000 counts = exactly 5 %
// num = 10000            → CC = 9999 = wrap → 99.99 % (see §8)
```

## 4. What the numbers actually produce

The divider for "25 kHz at wrap 9999" would have to be
`f_sys / (25 kHz × 10000)` — i.e. 0.5 at 125 MHz, which is **below the
minimum of 1**. The integer division in `PwmHelper` truncates to 0:

|                              | RP2040 @ 125 MHz | RP2350 @ 150 MHz |
| ---------------------------- | ---------------- | ---------------- |
| `clock_khz / 25 / 9999`      | 125000/25 = 5000 → 5000/9999 = **0** | 6000/9999 = **0** |
| `DIV.INT` written            | 0                | 0                |
| hardware divider in effect   | 1 (0 ≡ 1, §2)    | 1                |
| **actual PWM frequency**     | 125 MHz / 10000 = **12.5 kHz** | 150 MHz / 10000 = **15 kHz** |

So the fans are driven at 12.5 kHz (RP2040) / 15 kHz (RP2350), not the
nominal 25 kHz. Practical consequences:

- 4-wire fans only sample the duty cycle, and are tolerant of the lower
  frequency — the controller works. But 12.5/15 kHz is **outside the
  21–28 kHz window** the Noctua white paper recommends.
- To keep a true 25 kHz on RP2040, drop the resolution to what the clock
  allows: `wrap = f_sys / 25 kHz − 1 = 4999` (5000 steps, divider 1) —
  exactly 25 kHz. On RP2350: `wrap = 5999` (6000 steps).
- Keeping the current 10000 steps at 25 kHz would need `f_sys ≥ 250 MHz`.

> Note: the code divides by `top` (9999) instead of `top + 1` — an
> off-by-one, but negligible next to the truncation described above.
> The exact expression is `clkdiv = f_sys / (f_pwm × (wrap + 1))`.

## 5. Frequency vs resolution trade-off

Maximum duty steps at a target frequency (divider pinned to 1):
`steps_max = f_sys / f_target`.

| wrap   | steps  | f_pwm @ 125 MHz | f_pwm @ 150 MHz |
| ------ | ------ | --------------- | --------------- |
| 4999   | 5000   | 25 kHz          | 30 kHz          |
| 5999   | 6000   | 20.83 kHz       | 25 kHz          |
| **9999** | **10000** | **12.5 kHz (current)** | 15 kHz (current) |
| 65535  | 65536  | ~1.91 kHz       | ~2.29 kHz       |

For fan duty control, 4000–5000 steps (0.02 %) are far more than enough —
the PID works on ±1 % duty steps anyway.

## 6. Pin / slice map for this project

**Lite variant, board v93** ([pwm_controller_lite.cc](../exec/pwm_controller_lite.cc)):

| Function | Pin | slice.channel |
| -------- | --- | ------------- |
| Fan PWM  | GP3 | 1B            |
| Fan PWM  | GP2 | 1A            |
| Fan PWM  | GP27| 5B            |
| Fan PWM  | GP29| 6B            |
| Fan tach | GP4, GP1, GP28, GP0 | — (GPIO IRQ counting, not PWM) |

**Lite variant, board v90:** PWM on GP3 (1B), GP2 (1A), GP26 (5A), GP28 (6A).

**Mux variant** ([pwm_controller.cc](../exec/pwm_controller.cc)):

| Function | Pin | slice.channel |
| -------- | --- | ------------- |
| Fan PWM  | GP0 | 0A            |
| Fan PWM  | GP7 | 3B            |
| Fan PWM  | GP27| 5B            |
| Fan PWM  | GP17| 0B            |
| Tach in  | GP13 | 6B pin, used as IRQ input only |

Shared-slice notes:

- Lite: **GP2 + GP3 sit on slice 1** — they must share frequency/wrap
  (they do: all channels use the same config). Duty stays independent
  (CC_A vs CC_B). Mux: **GP0 + GP17 sit on slice 0** the same way.
- Two `PwmHelper` instances on one slice each call `pwm_init()` — the second
  call re-initialises the slice. Harmless while both use identical configs;
  if they disagreed, the last one to construct would win.
- Tach pulses are counted with GPIO edge interrupts
  ([frequency_counter.hh](../exec/frequency_counter.hh)), *not* with the PWM
  block's counter modes — so tach pins never steal PWM slices.

## 7. RP2350 notes

- Default `clk_sys` is **150 MHz**, so the same settings yield 15 kHz
  (§4) and `clock_get_hz(clk_sys)` must be the source of truth — which
  `PwmHelper` already does via `SystemClock`.
- PWM exists only on **GPIO0–GPIO23**. The current lite pin maps use
  GP26–GP29 for PWM (v90) or GP27/GP29 (v93), and the mux variant uses
  GP27 — **none of these have a PWM function on RP2350**. A `-rp2350`
  build needs its own pin map.
- 12 slices with no aliasing: `slice = gpio >> 1` for GPIO ≤ 23.

## 8. Gotchas

- `DIV.INT` of **0 or 1 both divide by 1** — that is why the release
  firmware still outputs PWM even though the computed divider truncates
  to 0 (§4).
- Debug builds keep the SDK parameter assertions enabled (no `-DNDEBUG` in
  `CMAKE_CXX_FLAGS_DEBUG`). There, `pwm_config_set_clkdiv_int(…, 0)` trips
  the SDK assert `div >= 1 && div < 256` and panics — the truncated
  divider is silent in Release only.
- Both channels of a slice share wrap/divider. Any second PWM user on an
  aliased GPIO (e.g. GP16 aliases GP0's slice 0) inherits — or overwrites —
  the slice config.
- `CC = wrap` gives 99.99 %, not 100 % (the counter is low for one count).
  True full-on needs `CC = wrap + 1`; `SetDutyCycle(10000)` lands on
  99.99 % — irrelevant for fans, visible on LEDs.
- `pwm_config_set_clkdiv_int()` is integer-only. Fractional division
  (`pwm_config_set_clkdiv_int_frac()` / float `pwm_config_set_clkdiv()`)
  averages the period over 16 cycles → jitter; prefer integer dividers.
- Changing `wrap` on a live slice can glitch the output for a period; keep
  wrap fixed after init (`PwmHelper` does) and only change `CC`.
- Noctua duty semantics: ~20 % and below is the min-RPM region (many fans
  won't start there — hence the 5 % start duty and the 500-cycle PID
  clamp), 0 % stops the fan on 0-RPM-capable models, 100 % is full speed.
