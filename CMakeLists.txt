cmake_minimum_required(VERSION 3.12)

IF(DEFINED ENV{IDF_TARGET})
    set(BUILD_PLATFORM "$ENV{IDF_TARGET}" CACHE STRING "The target platform of the current build, possible value: pico, esp32c3")
else()
    set(BUILD_PLATFORM "pico" CACHE STRING "The target platform of the current build, possible value: pico, esp32c3")
endif()

set(PLATFORM_ESP32_C3 false)
set(PLATFORM_PICO false)
set(USB_STDIO false)
message("BUILD_PLATFORM: ${BUILD_PLATFORM}")
if (${BUILD_PLATFORM} STREQUAL "pico") 
    set(PLATFORM_PICO true)
    # Pull in SDK (must be before project)
    include(cmake/pico_sdk_import.cmake)
    set(USB_STDIO true)

    if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
    endif()

    # Initialize the SDK
    pico_sdk_init()
    add_compile_definitions(PLATFORM_PICO)
elseif (${BUILD_PLATFORM} STREQUAL "esp32c3")
    set(PLATFORM_ESP32_C3 true)
    set(EXTRA_COMPONENT_DIRS "exec")
    #set(COMPONENTS "exec")
    include(cmake/esp32_sdk_import.cmake)
  
    add_compile_definitions(PLATFORM_ESP32 PLATFORM_ESP32_C3)
else()
    message(FATAL_ERROR "Invalid BUILD_PLATFORM, possible value: pico, esp32c3")
endif()

project(pwm_controller C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -Os")

add_compile_options(-Wall
    -Wformat
    -Wno-unused-function # we have some for the docs that aren't called
    )

if (PLATFORM_PICO)
    add_subdirectory(exec)
endif()
