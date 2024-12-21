FROM python:bookworm

RUN apt update

# Install pico sdk
# Appendix C: Manual toolchain setup: https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf 
RUN apt install build-essential wget cmake git -y
RUN apt install gcc-arm-none-eabi libnewlib-arm-none-eabi g++ libstdc++-arm-none-eabi-newlib -y
ENV PICO_BASE_PATH=/pico
ENV PICO_SDK_PATH=/pico/pico-sdk
RUN mkdir ${PICO_BASE_PATH}
WORKDIR ${PICO_BASE_PATH}
RUN git clone https://github.com/raspberrypi/pico-sdk.git

ARG TAG_NAME=2.1.0
WORKDIR ${PICO_SDK_PATH}
RUN git fetch --tags && git checkout -b v${TAG_NAME} tags/${TAG_NAME}
RUN git submodule update --init

# Install picotool
# Appendix B: Picotool: https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf 
RUN apt install libusb-1.0-0-dev -y
WORKDIR ${PICO_BASE_PATH}
RUN  git clone https://github.com/raspberrypi/picotool.git
WORKDIR ${PICO_BASE_PATH}/picotool
RUN mkdir build
WORKDIR ${PICO_BASE_PATH}/picotool/build
RUN cmake ..
RUN make -j$(nproc)
RUN make install

WORKDIR ${PICO_BASE_PATH}
