FROM python:bookworm

RUN apt update

ARG TAG_NAME=2.0.0

# Install pico sdk
# Appendix C: Manual toolchain setup: https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf 
RUN apt install build-essential wget cmake git -y
RUN apt install gcc-arm-none-eabi libnewlib-arm-none-eabi g++ libstdc++-arm-none-eabi-newlib -y
ENV PICO_BASE_PATH=/pico
ENV PICO_SDK_PATH=/pico/pico-sdk
RUN mkdir ${PICO_BASE_PATH}
WORKDIR ${PICO_BASE_PATH}
RUN git clone https://github.com/raspberrypi/pico-sdk.git
RUN git fetch --tags && git checkout -b v${TAG_NAME} tags/${TAG_NAME}
WORKDIR ${PICO_SDK_PATH}
RUN git submodule update --init
