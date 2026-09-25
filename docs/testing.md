# Testing

Two layers, both run in docker / CI (`.github/workflows/docker.yml`) and
usable locally. No local toolchain needed — everything runs through the
`chraac/pico-builder` image.

- **`test/unit/`** — host-side unit tests (GoogleTest). The real firmware
  logic, compiled for your machine against a **fake pico-sdk**
  (`test/stubs/`).
- **`test/integration/`** — emulator tests (rp2040js). Real firmware images
  booted in a simulator with fake i2c peripherals, asserting the control
  loop end-to-end.

## Layout

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

## Unit tests (host, GoogleTest)

The firmware calls the SDK directly, so instead of refactoring it, the test
build compiles the same `.cc` files against `test/stubs/include/`, which
shadows the SDK header paths. The stubs record everything (PWM levels, GPIO
config, i2c traffic) and expose a controllable fake clock + GPIO IRQ
injection via `stub_control.hh`. No production code is touched.

Covered: PID math, `EmaSmoother`, thermistor + `GetCurveValue` curve tables,
`GpioFreqencyCounter`/`FanSpeedHelper` rpm, INA226 register protocol and
conversions, all `SingleFanSpeedManager` control modes / dead-band /
no-tach handling, per-fan-type curve selection, and the
`FanSpeedManagerWithSelector` mux logic.

```sh
# in the pico-builder container (or docker/docker-compose-test.sh unit):
cmake -S test -B build-test -DCMAKE_BUILD_TYPE=Release
cmake --build build-test -j
ctest --test-dir build-test --output-on-failure
```

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
- GoogleTest v1.15.2 is fetched via FetchContent, pinned by tag **and**
  SHA256 (network needed once). For fully offline:
  `-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=<path>`.

## Integration tests (rp2040js emulator)

`harness/rp2040_runner.js` loads a `.uf2` or flash-linked `.elf` into the
vendored [wokwi/rp2040js](https://github.com/wokwi/rp2040js) simulator,
attaches the byte-level fake i2c devices to the RP2040's i2c peripherals,
captures firmware stdout from **both** UART and USB CDC, and boots into the
flash image (see the comment in the runner for why the entry point is the
flash base). Scenarios then program the fake INA226 and assert on the
firmware's own `TEST:` log lines:

```text
fake watts -> INA226 regs -> firmware EMA -> per-fan-type curves -> pwm
```

The structured `TEST:` lines and the 100 ms loop only exist in the
`INTEGRATION_TEST` firmware build; the plain `--smoke` mode also runs stock
images straight from `build/debug|release/`.

```sh
# smoke: run any stock firmware image and print its log live
bash test/integration/run.sh                          # defaults to build/debug/pwm_controller_pcie.uf2
bash test/integration/run.sh build/release/pwm_controller_pcie.uf2

# ci mode: scenario assertions, exit code 0/1
bash test/integration/run.sh <firmware> --ci
```

The `INTEGRATION_TEST` firmware build (UART stdio, 100 ms loop,
machine-parsable `TEST:` log lines) is built by the `build-emulator-fw` CI
job:

```sh
cmake -B build-emu -DPICO_BOARD=seeed_xiao_rp2040 -DINTEGRATION_TEST=ON \
      -DUSB_STDIO=false -DCMAKE_BUILD_TYPE=Release
make -C build-emu -j pwm_controller_pcie_emu
bash test/integration/run.sh build-emu/exec/pwm_controller_pcie_emu.elf --ci
```

Everything rp2040js-specific is deliberately isolated in
`harness/rp2040_runner.js` — if the simulator API ever changes, only that
file needs fixing. `PWM_TEST_DEBUG=1` traces every i2c byte;
`PWM_TEST_SMOKE_MS` / `PWM_TEST_WATTS` tune the smoke run.

### Known emulator limitations

- The first ~0.5 s of firmware stdout is not reliably captured (boot log
  lines); scenarios assert on loop-phase output instead.
- A fully absent i2c device (address NACK) wedges the SDK i2c driver inside
  the emulator — unlike real hardware. The probe-failure scenario uses a
  "wrong device" (ACKs, wrong ids) instead.
- RP2040 only (rp2040js limitation); the emulator build always pins
  `seeed_xiao_rp2040`.

## Docker wrapper

```sh
docker/docker-compose-test.sh unit          # unit tests in the builder image
docker/docker-compose-test.sh integration   # emu firmware build + scenarios
docker/docker-compose-test.sh all
```

The same wrapper is what CI runs. `exec/test/pid_simulator.py` remains the
interactive PID-tuning sandbox; `test/unit/pid_test.cc` is the regression
coverage.
