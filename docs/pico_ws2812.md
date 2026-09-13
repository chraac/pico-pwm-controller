# WS2812 / SK6812 Addressable LED (PIO) Reference (Pico C SDK)

How this project drives a single-wire addressable RGB(W) pixel — the protocol
and its timing, the byte order quirk of the onboard part, the PIO program in
[`ws2812.pio`](../exec/ws2812.pio), the [`Ws2812Helper`](../exec/rgb_led_helper.hh)
API, and how the PCIe firmware maps power draw to LED colour.

- WS2812B datasheet (timing, GRB order):
  <https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf>
- SK6812-RGBW datasheet (white channel, RGBW order variants):
  <https://cdn-shop.adafruit.com/product-files/2727/2727-13702786-SK6812RGBW.pdf>
- RP2040 datasheet, PIO chapter (state machines, instruction memory):
  <https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf>
- Upstream of our `.pio` file — pico-examples `pio/ws2812`:
  <https://github.com/raspberrypi/pico-examples/tree/master/pio/ws2812>
- Companion doc: [pico_pwm_settings.md](pico_pwm_settings.md) (PWM side),
  [pico_fan_speed.md](pico_fan_speed.md) (tach side)

## 1. The wire protocol

One data line, no clock. The host shifts bits MSB-first at **800 kHz**
(1.25 µs per bit slot); the *duration of the high phase* encodes the bit:

```text
            ├─── T1H ≈0.88 µs ───┤         bit slot = 10 PIO cycles = 1.25 µs
 bit '1':   ██████████████████████▁▁▁▁▁▁▁
            ├─ T0H ≈0.25 µs ─┤
 bit '0':   ████▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁
```

Each pixel consumes 24 bits (RGB parts) or 32 bits (RGBW parts), then
re-drives the remaining bits on its DO pin — that is how strips chain.
Sending the line **low for ≥ 50–80 µs** (WS2812B: 50 µs, SK6812: 80 µs)
is the **RESET/latch**: pixels freeze whatever they last shifted in and
show it. Data is not displayed until a latch gap occurs.

Consequences that shape the driver:

- Timing tolerance is ±150 ns — unreachable from C code with IRQ jitter;
  hence PIO (§3) does the bit-banging.
- A continuous back-to-back stream never latches; the stream must pause
  (see §5 gotchas). At one pixel refreshed every 500 ms this is free.

## 2. Byte order — this board's pixel is RGBW-order

The classic WS2812B latches **GRB** on the wire (green byte first). The
onboard pixel of the RP2040-Zero class boards is an SK6812 RGBW variant
whose channels arrive in **RGB(W)** order — verified on hardware; the
initial GRB packing (and the old `RP2040_Zero_Test.c` scratch program)
showed red/green swapped on this part.

`Ws2812Helper::SetRgb` packs the 32-bit FIFO word as:

```text
  bit 31        23        15         7        0
      ├─ R ─────┼─ G ─────┼─ B ──────┼─ W ─────┤   sent MSB-first
```

```cpp
const uint32_t rgbw = (uint32_t(red) << 24) | (uint32_t(green) << 16) |
                      (uint32_t(blue) << 8);
pio_sm_put_blocking(pio_, sm_, rgbw);
```

- `rgbw = true` (the default, and this board's config) sets the state
  machine's autopull threshold to 32 bits: all four bytes go out, and
  `SetWhite()` — which only fills bits [7:0] — drives the white channel.
- `rgbw = false` drops the threshold to 24 bits: only R, G, B leave the
  FIFO and `SetWhite()` degenerates to "black" (its byte is never pulled).
- Porting to another pixel: check its channel order first (GRB / RGB /
  RBGW … all exist in the wild). Symptom of a wrong order is *shuffled
  colours*, not a dead LED — only the packing in `SetRgb` needs to change.

## 3. The PIO program

[`exec/ws2812.pio`](../exec/ws2812.pio) is the single-wire program from
pico-examples (4 of the 32 instruction slots of one PIO block):

```text
bitloop:  out x, 1       side 0 [T3-1]   ; shift next bit, keep line low
          jmp !x do_zero side 1 [T1-1]   ; rising edge, branch on the bit
do_one:   jmp  bitloop   side 1 [T2-1]   ; '1': stay high (T1+T2 cycles)
do_zero:  nop            side 0 [T2-1]   ; '0': drop low (T2+T3 cycles)
```

- `T1=2, T2=5, T3=3` → **10 cycles per bit**; the init helper derives the
  clock divider as `div = clk_sys / (800 kHz × 10)`, so the program tracks
  any `clk_sys` (125 MHz default, 48 MHz after `set_sys_clock_48`, …).
- `.side_set 1` drives the pin *from the same instruction* — even a stalled
  `out` keeps the line in a defined state, which is what makes the timing
  bit-exact.
- Out-shift is **left/MSB-first with autopull**, TX FIFOs joined (8 words
  deep). One FIFO word = one pixel = one `pio_sm_put_blocking()` call.

The constructor claims resources and starts the machine:

```cpp
sm_(pio_claim_unused_sm(pio_, true)),          // any free SM on pio0
offset_(pio_add_program(pio_, &ws2812_program)) // panics if PIO is full
```

Build wiring ([exec/CMakeLists.txt](../exec/CMakeLists.txt)):
`pico_generate_pio_header()` runs **pioasm** at build time and produces
`ws2812.pio.h` (program listing + `ws2812_program_init`) into the build
tree; the targets also link `hardware_pio`.

## 4. The helper and its use in the PCIe firmware

| Member | Behaviour |
| --- | --- |
| `Ws2812Helper{pin, rgbw=true, pio=pio0, freq=800000}` | claims an SM, loads the program, blanks the pixel |
| `SetRgb(r, g, b)` | one RGB frame; also updates `current_value_` |
| `SetWhite(w)` | white-channel frame (RGBW parts only, §2) |
| `SetRed/SetGreen/SetBlue/Off` | conveniences over `SetRgb` |
| `Next()` | steps the 8 on/off RGB combos, like `RgbLedHelper::Next()`; continues from the last manually set colour |

[`pwm_controller_pcie.cc`](../exec/pwm_controller_pcie.cc) uses it as a
power indicator on `kWs2812LedPin` = **GP16**:

```cpp
Ws2812Helper rgb_led{kWs2812LedPin};
...
SetPwrLedColor(rgb_led, watts, kLedGreenW /*20 W*/, kLedRedW /*90 W*/);
```

`SetPwrLedColor` blends **green → yellow → red** across that window. The
channel values are passed through `std::sqrt(t)` deliberately: LED duty is
linear in *current*, but perceived brightness is roughly √duty — without
the curve the blend looks like it jumps to yellow almost immediately.
The loop runs every `kBoardPoolIntervalMs` = 500 ms, which also satisfies
the latch gap for free (§1).

`RgbLedHelper` (three discrete GPIO pins) remains for the lite board;
the two classes are drop-in similar (`Next()`, `SetRed()`…) on purpose.

## 5. Gotchas

- **Wire order is per-part, not per-family.** GRB (stock WS2812B), RGB
  (this board's SK6812 variant), RBG, GRBW… all exist. First thing to
  check when colours look shuffled on new hardware (§2).
- **No latch without a pause.** Pushing words back-to-back forever just
  streams them down a chain; pixels only display after ≥ 50–80 µs idle.
  Single pixel + 500 ms poll = immune, but keep it in mind before raising
  the refresh rate or driving a strip from a tight loop.
- **`pio_sm_put_blocking` spins when the FIFO is full** (8 words). Harmless
  at this update rate; for long strips it bounds the push rate at
  8 pixels / 10 µs and the loop must tolerate the block.
- **`rgbw=true` on a plain RGB pixel** sends 32 bits where 24 latch —
  harmless for a *single* pixel (the extra byte is discarded at RESET),
  but on a strip it lands in pixel 2's first channel. Match the flag to
  the part.
- **Logic levels for external 5 V strips:** at VDD = 5 V the spec'd VIH is
  0.7·VDD = 3.5 V — above the Pico's 3.3 V drive. It usually works, but
  the robust fix is a level shifter (or feed the first pixel ~4.3 V). The
  onboard pixel runs on the 3V3 domain, so direct drive is in spec here.
- **Current:** budget ~60 mA per channel at full white (~20 mA per colour
  at 0xff). Irrelevant for one onboard pixel; size the supply for strips.
- **One PIO SM is held forever** (claimed in the constructor, never
  released). With the program's 4 instructions loaded on `pio0`, three SMs
  remain — fine unless other subsystems start claiming PIO heavily.
- **Single-core use:** `pio_sm_put_blocking` is not cross-core
  synchronised by this wrapper; construct and call the helper from the
  same core (core 0, as everywhere in this firmware).
