cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(cmake/pico_sdk_import.cmake)

project(pwm_controller C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -Os")

set(USB_STDIO true)
set(BUILD_LITE true)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
    -Wformat
    -Wno-unused-function # we have some for the docs that aren't called
    #-Wno-maybe-uninitialized
    )

add_subdirectory(exec)