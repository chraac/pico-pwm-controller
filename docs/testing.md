# Testing

Two layers, both run in docker / CI (`.github/workflows/docker.yml`) and
usable locally. No local toolchain needed — everything runs through the
`chraac/pico-builder` image. For the layout and design of what lives under
`test/`, see [test/README.md](../test/README.md).

## Unit tests (host, GoogleTest)

`test/` is a **standalone CMake project** that compiles the real firmware
logic sources (`frequency_counter.cc`, `fan_speed_manager.cc` + the
header-only helpers) for the host against a fake pico-sdk:

- `test/stubs/include/` — headers shadowing the SDK paths
  (`hardware/pwm.h`, `hardware/gpio.h`, `hardware/i2c.h`, ...)
- `test/stubs/stubs.cc` — implementations with controllable state
- `test/stubs/stub_control.hh` — test API: fake clock
  (`setTimeUs`/`advanceUs`), GPIO IRQ injection (`fireGpioIrq`), PWM level
  recording, register-file fake i2c devices (`i2cAttach`)

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

Notes:

- Keep the source list in `test/CMakeLists.txt` in sync with
  `PWM_CONTROLLER_LIB_SRC` in `exec/CMakeLists.txt`.
- The lib compiles with `-fno-exceptions` to match the firmware build
  (pico-sdk default): the `.cc` definitions omit `noexcept` their headers
  declare.
- Static-state rule: production code keeps per-pin static event slots and a
  `SystemClock` singleton, so each test suite owns a **disjoint GPIO pin
  range** (documented in each test file) instead of resetting statics.
- First configure fetches GoogleTest v1.15.2 via FetchContent (network
  needed once). For offline: `-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=<path>`.

## Integration tests (rp2040js emulator)

`test/integration/` boots real firmware images in the
[wokwi/rp2040js](https://github.com/wokwi/rp2040js) simulator (vendored in
`test/integration/simulator/vendor/` as npm tarballs — no registry access
needed after checkout) with fake i2c peripherals:

- `harness/fake_ina226.js` — INA226 @0x40 on i2c1 with programmable watts
- `harness/fake_ssd1306.js` — LCD @0x3c on i2c0, absorbs the draw traffic
- `harness/rp2040_runner.js` — the only file touching the rp2040js API
  (loads `.uf2`/`.elf`, captures stdout from UART and USB CDC, injects GPIO)

The scenarios assert the whole chain: fake watts → EMA smoother →
per-fan-type power curves → PWM duty cycles.

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

`PWM_TEST_DEBUG=1` traces every i2c byte; `PWM_TEST_SMOKE_MS` /
`PWM_TEST_WATTS` tune the smoke run.

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
