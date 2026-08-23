# Pico Pwm Controller

A PWM fan controller runs on raspberry-pico

[![Builder](https://github.com/chraac/pico-pwm-controller/actions/workflows/docker.yml/badge.svg)](https://github.com/chraac/pico-pwm-controller/actions/workflows/docker.yml)

## Features

- Controls up to 4 independent 4-wire PWM fans (25 kHz PWM, following the
  [Noctua PWM specification](https://noctua.at/pub/media/wysiwyg/Noctua_PWM_specifications_white_paper.pdf))
- Two control modes:
  - **RPM mode** – closed-loop [PID](exec/pid.hh) control that holds each fan at a
    target RPM using the fan's tachometer signal
  - **Temperature mode** – drives the PWM duty cycle from an NTC thermistor
    reading through a piecewise-linear temperature curve
- Fan speed (RPM) measurement from the tach signal: `rpm = freq[Hz] * 60 / 2`
- SSD1306 128x64 I2C LCD showing temperature, fan RPM and duty cycle
- RGB status LED
- Logging over USB CDC (`log_info` / `log_debug`, debug logs in Debug builds only)

## How it works

The main loop runs every 400 ms (`kPoolIntervalMs` in
[fan_speed_manager.hh](exec/fan_speed_manager.hh)):

1. Read the thermistor (ADC) and convert the resistance to a temperature with
   the Steinhart–Hart beta equation.
2. For each fan: read the tach frequency counter, compute the current RPM and
   update the PWM duty cycle.
3. Refresh the LCD and RGB LED.

### RPM mode (PID)

Each fan gets a PID controller (`Kp=0.5, Ki=0.3, Kd=0.02`) whose output is the
PWM duty cycle, clamped to 500–10000 (duty cycle resolution is 10000 steps).
The default target is 1900 RPM with a ±60 RPM tolerance band — when the fan is
inside the band, the PID is skipped to avoid hunting.

### Temperature mode

Fans in temperature mode use a linear-interpolated curve
(`kLinearFanPwmCurve` in [temp_helper.hh](exec/temp_helper.hh)):

| Temperature | Duty cycle |
| ----------- | ---------- |
| 10 °C       | 15 %       |
| 20 °C       | 20 %       |
| 30 °C       | 26 %       |
| 35 °C       | 31 %       |
| 40 °C       | 36 %       |
| 45 °C       | 44 %       |
| 50 °C       | 55 %       |
| 55 °C       | 68 %       |
| 60 °C       | 81 %       |
| 65 °C       | 100 %      |

Below 10 °C the curve clamps to 15 %, above 65 °C to 100 %.

### Temperature sensing

An NTC thermistor (default: 100kΩ, β=3950) is wired as a voltage divider with a
10kΩ resistor on an ADC pin. Supported thermistor profiles are defined in
[temp_helper.hh](exec/temp_helper.hh) (`kNtc10k3435`, `kNtc10k3950`,
`kNtc100k3950`).

## Hardware

### Supported boards

Built and tested with the Seeed XIAO RP2040 (default) and XIAO RP2350 form
factor; any RP2040/RP2350 board works as long as the pin map fits — pins are
configured in [pwm_controller_lite.cc](exec/pwm_controller_lite.cc) /
[pwm_controller.cc](exec/pwm_controller.cc).

![pins](docs/pico_pwm_pins.png)

### Firmware variants

Two firmware variants are selected at build time with `BUILD_LITE` in the
top-level [CMakeLists.txt](CMakeLists.txt):

- **Lite (default)** – [pwm_controller_lite.cc](exec/pwm_controller_lite.cc):
  4 independent fans, each with its own PWM + tach pin, plus LCD, RGB LED and
  temperature control. The first 3 fans run in temperature mode and the 4th in
  RPM mode.
- **Multi-fan (mux)** – [pwm_controller.cc](exec/pwm_controller.cc): 4 PWM
  groups driving 12 fans. Tach lines are routed through an external analog
  multiplexer (select pins GP8–GP11) to a single input pin (GP13); fans are
  polled one at a time and each PWM group is PID-controlled on the maximum fan
  speed of its group.

### Pin mapping

Lite variant, board version 9x (`PICO_LITE_BOARD_VERSION != 90`):

| Function       | Pin                    |
| -------------- | ---------------------- |
| Fan PWM        | GP3, GP2, GP27, GP29   |
| Fan tach       | GP4, GP1, GP28, GP0    |
| Thermistor ADC | GP26                   |
| LCD I2C1       | SCL=GP7, SDA=GP6       |
| RGB LED        | R=GP17, G=GP16, B=GP25 |

Board version 90 swaps two pins (see [pwm_controller_lite.cc](exec/pwm_controller_lite.cc)):
PWM on GP3, GP2, GP26, GP28 and tach on GP4, GP1, GP27, GP29.

Mux variant ([fan_speed_manager.cc](exec/fan_speed_manager.cc)): PWM on
GP0, GP7, GP27, GP17; tach input on GP13; mux select bits on GP8–GP11.

## Build Project

### With docker

1. Install [docker compose](https://docs.docker.com/compose/install/)

2. Build (default board: Seeed XIAO RP2040)

    ```bash
    docker/docker-compose-compile.sh
    ```

    Or build for the XIAO RP2350:

    ```bash
    docker/docker-compose-compile.sh -rp2350
    ```

3. Copy the build/release/pwm_controller.uf2 into RPI-RP2 drive

Both Debug and Release builds are produced under `build/debug/` and
`build/release/` (`.uf2`, `.elf`, `.bin` and `.map` files).

### Without docker

Any environment with the [Pico SDK](https://github.com/raspberrypi/pico-sdk)
(≥ 1.3.0), CMake and an ARM cross toolchain works:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DPICO_BOARD=seeed_xiao_rp2040
make -j
```

### Build options

Options are set in the top-level [CMakeLists.txt](CMakeLists.txt):

| Option          | Default | Description                                        |
| --------------- | ------- | -------------------------------------------------- |
| `BUILD_LITE`    | `true`  | Build the lite firmware instead of the mux variant |
| `BOARD_VERSION` | `"93"`  | Board revision, selects the lite pin map           |
| `USB_STDIO`     | `true`  | Log over USB CDC instead of UART                   |

## Logs

The firmware logs over USB CDC (the same USB port used for flashing). Connect
with any serial terminal, e.g. `screen /dev/ttyACM0` or
`pio device monitor`. Debug-level messages are only compiled into Debug builds.

## PID tuning

A simple simulation for tuning the PID constants is available in
[exec/test/pid_simulator.py](exec/test/pid_simulator.py).

## Project layout

```text
exec/            firmware sources
  pwm_controller_lite.cc   lite firmware entry point
  pwm_controller.cc        mux firmware entry point
  fan_speed_manager.*      fan control loop and PID wiring
  temp_helper.hh           thermistor + temperature curve
  frequency_counter.*      tach pulse counting
  lcd_helper.hh            SSD1306 LCD drawing
thirdparty/      dependencies (pico-ssd1306)
docker/          docker-based build environment
```

## Running picture

![pic](pic/pic.jpg)
