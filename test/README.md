# test/

Test setup for the pwm_controller firmware. Two layers:

- **`unit/`** — host-side unit tests (GoogleTest). The real firmware logic,
  compiled for your machine against a **fake pico-sdk** (`stubs/`).
- **`integration/`** — emulator tests (rp2040js). Real firmware images booted
  in a simulator with fake i2c peripherals, asserting the control loop
  end-to-end.

How to *run* everything (docker / WSL / CI commands): see
[docs/testing.md](../docs/testing.md). This file is about the structure.

```text
test/
├── CMakeLists.txt            standalone host project -- NO pico-sdk needed
├── unit/                     one suite per module under test
│   ├── pid_test.cc             exec/pid.hh
│   ├── ema_smoother_test.cc    exec/ema_smoother.hh
│   ├── temp_helper_test.cc     exec/temp_helper.hh (curves, thermistor)
│   ├── frequency_counter_test.cc
│   ├── fan_speed_helper_test.cc
│   ├── ina226_test.cc          register protocol + conversions via fake i2c
│   └── fan_speed_manager_test.cc  control modes, dead-band, mux
├── stubs/                    the fake pico-sdk
│   ├── include/                headers shadowing SDK paths (hardware/pwm.h,
│   │                           hardware/gpio.h, hardware/i2c.h, ...)
│   ├── stubs.cc                implementations, recording into one state blob
│   └── stub_control.hh         the API tests drive: setTimeUs/advanceUs,
│                               fireGpioIrq, i2cAttach, pwmGpioLevel, ...
└── integration/
    ├── run.sh                one entry point: ./run.sh [fw] [--smoke|--ci]
    ├── smoke.js                interactive: print firmware log lines live
    ├── run-all.js              ci mode: run scenarios/, exit 0/1
    ├── harness/
    │   ├── rp2040_runner.js      ALL rp2040js API calls live here (see below)
    │   ├── fake_ina226.js        INA226 @0x40 on i2c1, programmable watts
    │   └── fake_ssd1306.js       LCD @0x3c on i2c0, absorbs draw traffic
    ├── scenarios/             assertions; add a *.test.js to add a scenario
    │   ├── pcie_boot_smoke.test.js
    │   └── pcie_power_to_pwm.test.js
    └── simulator/            VENDORED simulator (committed, offline)
        ├── setup.sh            extracts vendor/*.tgz into node_modules/
        └── vendor/             rp2040js + uf2 tarballs, bootrom.js
```

## How the unit tests reach hardware-coupled code

The firmware calls the SDK directly, so instead of refactoring it, the test
build compiles the same `.cc` files against `stubs/include/`, which shadows
the SDK header paths. The stubs record everything (PWM levels, GPIO config,
i2c traffic) and expose a controllable fake clock + GPIO IRQ injection via
`stub_control.hh`. No production code is touched.

Invariants to keep in mind:

- **Source list sync** — `test/CMakeLists.txt` compiles the same files as
  `PWM_CONTROLLER_LIB_SRC` in `exec/CMakeLists.txt`. Update both together.
- **Pin ranges** — production code keeps per-GPIO static event slots and a
  `SystemClock` singleton that a host process cannot reset. Each test suite
  therefore owns a disjoint pin range (documented at the top of each test
  file); don't reuse pins across suites.
- **`-fno-exceptions`** — the hostlib builds with it to match the firmware
  build (pico-sdk default), because the `.cc` definitions omit `noexcept`
  their headers declare.

## How the integration tests work

`rp2040_runner.js` loads a `.uf2` or flash-linked `.elf` into the vendored
rp2040js simulator, attaches the byte-level fake i2c devices to the RP2040's
i2c peripherals, captures firmware stdout from **both** UART and USB CDC,
and boots into the flash image (see the comment in the runner for why the
entry point is the flash base). Scenarios then program the fake INA226 and
assert on the firmware's own `TEST:` log lines:

```
fake watts -> INA226 regs -> firmware EMA -> per-fan-type curves -> pwm
```

The structured `TEST:` lines and the 100 ms loop only exist in the
`INTEGRATION_TEST` firmware build (`-DINTEGRATION_TEST=ON -DUSB_STDIO=false`,
adds the `pwm_controller_pcie_emu` target); the plain `--smoke` mode also
runs stock images straight from `build/debug|release/`.

Everything rp2040js-specific is deliberately isolated in `rp2040_runner.js`
— if the simulator API ever changes, only that file needs fixing. Known
emulator limitations (lost boot log lines, i2c-NACK wedge) are documented in
[docs/testing.md](../docs/testing.md) and worked around in the scenarios.
