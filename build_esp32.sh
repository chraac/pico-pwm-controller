#!/bin/bash

set -e
PROJECT_PATH=$(pwd)
echo "PROJECT_PATH: $PROJECT_PATH"

docker run --rm -e IDF_TARGET=esp32c3 -v $PROJECT_PATH:/project -w /project espressif/idf:v4.4.1 idf.py -Bbuild_esp32 -DCMAKE_BUILD_TYPE=Debug build