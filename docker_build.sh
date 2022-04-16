#!/bin/bash

if [ -z $PICO_BASE_PATH ]; then 
    echo 'Variable PICO_BASE_PATH not set'
    exit 1
else
    echo "PICO_BASE_PATH: $PICO_BASE_PATH"
fi

set -e
PROJECT_PATH=$(pwd)
echo "PROJECT_PATH: $PROJECT_PATH"

docker run --rm -it \
    -v $PICO_BASE_PATH:/pico \
    -v $PROJECT_PATH:/project \
    -w=/project \
    -e PICO_SDK_PATH=/pico/pico-sdk \
    chraac/pico-builder:latest \
    bash -c 'mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make'
