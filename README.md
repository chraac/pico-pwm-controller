# Pico Pwm Controller

A PWM fan controller runs on raspberry-pico

## Build Project

1. Install docker

1. Pull builder image

    ```bash
    docker pull chraac/pico-builder:latest
    ```

1. Build

    ```bash
    docker run --rm -it \
        -v /path/to/your/pico:/pico \
        -v /path/to/your/project:/project \
        chraac/pico-builder:latest \
        bash -c 'mkdir -p $PROJECT_PATH/build && cd $PROJECT_PATH/build && cmake .. && make'
    ```

1. Copy the build/exec/pwm_controller.uf2 into RPI-RP2 drive
