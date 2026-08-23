# INA226 I2C Command Reference (Pico C SDK)

Reference for talking to a TI INA226 current/shunt/power monitor over I2C,
bare-metal style with the Pico C SDK (`hardware/i2c.h`), no Arduino library.

- RobTillaart Arduino library (register addresses / formulas): <https://github.com/RobTillaart/INA226>
- TI datasheet: <https://www.ti.com/lit/ds/symlink/ina226.pdf>

## 1. Wiring

The INA226 is a normal I2C slave and can share the bus with the SSD1306 LCD
(address `0x3C` does not collide with the INA226 range `0x40..0x4F`).

On this project's XIAO RP2040 the OLED already owns `i2c1` on
GPIO6 (SDA) / GPIO7 (SCL) — the INA226 can hang on the same two wires.

| INA226 pin | Connect to                    |
| ---------- | ----------------------------- |
| VS         | 3V3 (also fine: 5V, max 36V)  |
| GND        | GND (same ground as Pico)     |
| SDA        | GPIO6 (i2c1 SDA) + pull-up    |
| SCL        | GPIO7 (i2c1 SCL) + pull-up    |
| A1 / A0    | GND (address 0x40), see below |
| VBUS       | load supply rail to measure   |
| IN+ / IN−  | across the shunt resistor     |

Shunt wiring: the measured current must flow **through** the shunt —
put the shunt in the load's high-side (or low-side) path, `IN+` on the
supply side, `IN−` on the load side, and keep the layout kelvin-style
(sense lines close to the shunt pads).

## 2. I2C addressing

- 7-bit address range **0x40–0x4F** (16 addresses), set by A1/A0 pins
  (each low/high/floating → 2 bits).
- Most breakout modules default to **0x40** (A1 = A0 = GND).
- Bus speed: standard 100 kHz / fast 400 kHz (the project already runs
  `i2c1` at 400 kHz — fine).

## 3. Transaction format

The chip has an 8-bit **pointer register** that selects which 16-bit
register the next read/write hits. All data transfers are 16-bit,
**MSB first**.

Write a register (e.g. write `0x4127` to Configuration):

```
S | 0x80 (0x40<<1 | W) | ACK | ptr | ACK | MSB | ACK | LSB | ACK | P
```

Read a register (e.g. read Bus Voltage, ptr = 0x02):

```
S | 0x80 (0x40<<1 | W) | ACK | 0x02 | ACK |
Sr| 0x81 (0x40<<1 | R) | ACK | MSB | ACK | LSB | NACK | P
```

Pico SDK equivalents:

```c
// write 16-bit reg: pointer + 2 bytes
uint8_t buf[3] = {reg, (value >> 8) & 0xFF, value & 0xFF};
i2c_write_blocking(i2c1, 0x40, buf, 3, false);

// read 16-bit reg: set pointer, then read 2 bytes
uint8_t ptr = reg, val[2];
i2c_write_blocking(i2c1, 0x40, &ptr, 1, true);          // no stop (repeated start)
i2c_read_blocking(i2c1, 0x40, val, 2, false);           // MSB first
uint16_t raw = (val[0] << 8) | val[1];
```

## 4. Register map

| Ptr   | Register            | R/W | Reset    | Format                                  |
| ----- | ------------------- | --- | -------- | --------------------------------------- |
| 0x00  | Configuration       | R/W | 0x4127   | bit field, see §5                       |
| 0x01  | Shunt Voltage       | R   | 0x0000   | two's complement, LSB = 2.5 µV          |
| 0x02  | Bus Voltage         | R   | 0x0000   | unsigned, LSB = 1.25 mV (0–36 V range)  |
| 0x03  | Power               | R   | 0x0000   | unsigned, LSB = 25 × Current_LSB        |
| 0x04  | Current             | R   | 0x0000   | two's complement, LSB = Current_LSB     |
| 0x05  | Calibration         | R/W | 0x0000   | 15-bit value, see §7                    |
| 0x06  | Mask/Enable (Alert) | R/W | 0x0000   | bit field, see §6                       |
| 0x07  | Alert Limit         | R/W | 0x0000   | same LSB as the selected compared reg   |
| 0xFE  | Manufacturer ID     | R   | 0x5449   | ASCII "TI" — good probe for detection   |
| 0xFF  | Die ID              | R   | 0x2260   | fixed — good probe for detection        |

Quick sanity probe after wiring:

```c
// read 0xFE → expect 0x5449, read 0xFF → expect 0x2260
```

## 5. Configuration register (0x00), reset = 0x4127

```
 15   14 13 12   11 10 9   8 7 6   5 4 3   2 1 0
+----+-----------+---------+-------+---------+-------+
| RST| reserved  |  AVG    |VBUSCT | VSHCT  | MODE  |
+----+-----------+---------+-------+---------+-------+
```

- **Bit 15 RST** — write 1 to reset all registers to defaults (self-clearing).
- **Bits 14–12** — reserved (default reads `001`), leave as reset value.
- **Bits 11–9 AVG** — number of samples averaged:
  `000`=1, `001`=4, `010`=16, `011`=64, `100`=128, `101`=256, `110`=512, `111`=1024
- **Bits 8–6 VBUSCT** — bus voltage conversion time:
  `000`=140 µs, `001`=204 µs, `010`=332 µs, `011`=588 µs,
  `100`=1.1 ms (default), `101`=2.116 ms, `110`=4.156 ms, `111`=8.244 ms
- **Bits 5–3 VSHCT** — shunt voltage conversion time (same 8 values as VBUSCT).
- **Bits 2–0 MODE**:
  `000`/`100` = power-down · `001` = shunt, one-shot · `010` = bus, one-shot ·
  `011` = shunt+bus, one-shot · `101` = shunt continuous ·
  `110` = bus continuous · `111` = **shunt+bus continuous (default)**

Total conversion time = (VBUSCT + VSHCT) × AVG. With reset defaults:
(1.1 ms + 1.1 ms) × 1 ≈ **2.2 ms** per update of the shunt/bus registers.
Poll for conversion-ready instead of hard-coding delays (see §6 CVRF).

Example — 16 averages, 1.1 ms both channels, continuous:

```
cfg = (4 << 9) | (4 << 6) | (4 << 3) | 7  =  0x247F... check bits:
     AVG=100(16), VBUSCT=100(1.1ms), VSHCT=100(1.1ms), MODE=111
```

## 6. Mask/Enable register (0x06)

Alert-function select + status flags:

| Bit  | Name  | Meaning                                              |
| ---- | ----- | ---------------------------------------------------- |
| 15   | SOL   | Shunt over-voltage (limit = shunt LSB 2.5 µV steps)  |
| 14   | SUL   | Shunt under-voltage                                  |
| 13   | BOL   | Bus over-voltage (1.25 mV steps)                     |
| 12   | BUL   | Bus under-voltage                                    |
| 11   | POL   | Power over-limit (25 × Current_LSB steps)            |
| 10   | CNVR  | Alert on conversion ready                            |
| 9–5  | —     | reserved, 0                                          |
| 4    | AFF   | (read) alert function flag                           |
| 3    | CVRF  | (read) conversion ready flag — cleared by read       |
| 2    | OVF   | (read) math overflow (current/power calc saturated)  |
| 1    | APOL  | alert pin polarity: 0 = active-low, 1 = active-high  |
| 0    | LEN   | 0 = transparent, 1 = latch alert until read         |

Only **one** of SOL/SUL/BOL/BUL/POL/CNVR may be selected at a time.
Polling without the alert pin: read 0x06 and check bit 3 (CVRF).

## 7. Reading current

Two ways — the calibration register is only needed for the on-chip
Current/Power registers.

### 7a. Simple: read shunt voltage, divide by R (no calibration)

```c
int16_t raw = read16(0x01);                  // two's complement
float v_shunt_mV = raw * 2.5e-3f;            // 2.5 µV per LSB
float amps = v_shunt_mV / shunt_ohms / 1000.0f;
```

Full-scale shunt input is ±81.92 mV. Max measurable current =
`81.92 mV / R_shunt`.

> Module note: common INA226 breakouts ship an **R100 (0.1 Ω)** shunt →
> max ≈ **0.82 A**. Fine for a fan rail (~0.1–0.5 A); not fine for 10 A.
> Check the marking on your shunt: `R100` = 0.1 Ω, `R002`/`2mΩ` = 0.002 Ω.

### 7b. Full: on-chip Current (0x04) / Power (0x03) registers

Pick `Current_LSB` (the resolution of the current register), then program
the calibration register:

```
Cal = 0.00512 / (Current_LSB × R_shunt)     // round to nearest int, ≤ 32767
```

The chip then fills Current = Shunt_V / (R_shunt × Current_LSB) and
Power = Current × Bus_V / 25 internally (Power LSB = 25 × Current_LSB).

Good default: `Current_LSB = max_expected_current / 32768`
(what the RobTillaart library does via `setMaxCurrentShunt()`).

Worked example — R = 0.1 Ω, max current 0.8 A:

```
Current_LSB = 0.8 / 32768        = 24.4 µA
Cal         = 0.00512 / (24.4e-6 × 0.1) = 2097 = 0x0831
Power LSB   = 25 × 24.4 µA       = 0.61 mA... (25 × Current_LSB in W/A sense)
```

Constraint: `R_shunt × max_current ≤ 81.92 mV` (else ERR_SHUNTVOLTAGE_HIGH
in library terms / you lose range). If Cal overflows 0x7FFF, double the LSB
and halve Cal until it fits.

## 8. Register-scale cheat sheet

| Quantity    | Raw → physical                    |
| ----------- | --------------------------------- |
| Bus voltage | `raw × 1.25 mV`                   |
| Shunt volt. | `raw(int16) × 2.5 µV`             |
| Current     | `raw(int16) × Current_LSB`        |
| Power       | `raw × 25 × Current_LSB`          |
| Cal reg     | `0.00512 / (Current_LSB × R)`     |

## 9. Minimal driver skeleton (project style)

```cpp
// ina226_helper.hh
#pragma once

#include <hardware/i2c.h>

#include "base_types.hh"

namespace utility {

class Ina226Device {
    constexpr static const uint8_t kI2cAddr = 0x40;
    constexpr static const float kShuntOhms = 0.1f;  // R100 module

    // register pointers
    enum Reg : uint8_t {
        kConfig = 0x00, kShuntV = 0x01, kBusV = 0x02, kPower = 0x03,
        kCurrent = 0x04, kCalib = 0x05, kMaskEnable = 0x06,
        kAlertLimit = 0x07, kMfgId = 0xFE, kDieId = 0xFF,
    };

public:
    explicit Ina226Device(i2c_inst_t *i2c) noexcept : i2c_inst_(i2c) {}

    bool Probe() noexcept {  // true if a likely INA226 answers
        return Read16(kMfgId) == 0x5449 && Read16(kDieId) == 0x2260;
    }

    // 16x avg, 1.1ms conversions, shunt+bus continuous -> ~35ms/update
    void Configure(uint16_t cfg = 0x247F) noexcept { Write16(kConfig, cfg); }

    float GetBusVolts() noexcept { return Read16(kBusV) * 1.25e-3f; }
    float GetShuntMilliVolts() noexcept {
        return static_cast<int16_t>(Read16(kShuntV)) * 2.5e-3f;
    }
    // simplest current path: no calibration register needed
    float GetAmps() noexcept {
        return GetShuntMilliVolts() / (kShuntOhms * 1000.0f);
    }

private:
    uint16_t Read16(uint8_t reg) noexcept {
        uint8_t val[2] = {};
        i2c_write_blocking(i2c_inst_, kI2cAddr, &reg, 1, true);
        i2c_read_blocking(i2c_inst_, kI2cAddr, val, 2, false);
        return static_cast<uint16_t>(val[0] << 8) | val[1];
    }

    void Write16(uint8_t reg, uint16_t value) noexcept {
        uint8_t buf[3] = {reg, static_cast<uint8_t>(value >> 8),
                          static_cast<uint8_t>(value & 0xFF)};
        i2c_write_blocking(i2c_inst_, kI2cAddr, buf, 3, false);
    }

    i2c_inst_t *i2c_inst_;

    DISALLOW_COPY(Ina226Device);
    DISALLOW_MOVE(Ina226Device);
};

}  // namespace utility
```

Usage alongside the existing LCD (same `i2c1`, initialized by
`Ssd1306Device` first):

```cpp
utility::Ina226Device ina(i2c1);
if (ina.Probe()) {
    ina.Configure();
    // poll every ~50 ms
    float amps = ina.GetAmps();
    float volts = ina.GetBusVolts();
}
```

## 10. Gotchas

- No calibration written → Current (0x04) and Power (0x03) registers read 0.
  Use §7a if you only need current.
- Shunt Voltage and Current registers are **signed** (current can be
  negative depending on IN+/IN− orientation). Cast to `int16_t`.
- CVRF/conversion-ready: in continuous mode registers refresh every
  `(VBUSCT + VSHCT) × AVG` — polling faster just returns stale data.
- Bus Voltage register measures VBUS pin (0–36 V), NOT the VS supply pin.
- The Alert pin (open-drain, active-low by default) needs a pull-up if used.
