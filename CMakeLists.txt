cmake_minimum_required(VERSION 3.12)

if(DEFINED ENV{IDF_TARGET})
    set(BUILD_PLATFORM "$ENV{IDF_TARGET}" CACHE STRING "The target platform of the current build, possible value: pico, esp32c3")
else()
    set(BUILD_PLATFORM "pico" CACHE STRING "The target platform of the current build, possible value: pico, esp32c3")
endif()

set(PLATFORM_ESP32 FALSE)
set(PLATFORM_ESP32_C3 FALSE)
set(PLATFORM_PICO FALSE)
set(USB_STDIO FALSE)
message("BUILD_PLATFORM: ${BUILD_PLATFORM}")
if(${BUILD_PLATFORM} STREQUAL "pico") 
    set(PLATFORM_PICO TRUE)
    # Pull in SDK (must be before project)
    include(cmake/pico_sdk_import.cmake)
    set(USB_STDIO TRUE)

    if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
    endif()

    project(pwm_controller C CXX ASM)
    
    # Initialize the SDK
    pico_sdk_init()
    add_compile_definitions(PLATFORM_PICO)
    add_subdirectory(main)
elseif(${BUILD_PLATFORM} STREQUAL "esp32c3")
    set(PLATFORM_ESP32 TRUE)
    set(PLATFORM_ESP32_C3 TRUE)
    add_compile_definitions(PLATFORM_ESP32 PLATFORM_ESP32_C3)

    include(cmake/esp32_sdk_import.cmake)
    project(pwm_controller)
else()
    message(FATAL_ERROR "Invalid BUILD_PLATFORM, possible value: pico, esp32c3")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -Os")

add_compile_options(-Wall
    -Wformat
    -Wno-unused-function # we have some for the docs that aren't called
    )
