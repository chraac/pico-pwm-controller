cmake_minimum_required(VERSION 3.12)

set(USB_STDIO FALSE)
include(cmake/platform_detect.cmake)

if(PLATFORM_ESP32)
    set(BUILD_PLATFORM "${IDF_TARGET}" CACHE STRING "The target platform of the current build, possible value: pico, esp32c3")
else()
    set(BUILD_PLATFORM "pico" CACHE STRING "The target platform of the current build, possible value: pico, esp32c3")
endif()

message("BUILD_PLATFORM: ${BUILD_PLATFORM}")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -Os")

if(PLATFORM_PICO) 
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
    add_compile_options(-Wall
        -Wformat
        -Wno-unused-function # we have some for the docs that aren't called
        )
        add_subdirectory(main)
elseif(PLATFORM_ESP32)
    add_compile_definitions(PLATFORM_ESP32)
    #set(COMMON_COMPILE_FLAGS "-DPLATFORM_ESP32")
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_COMPILE_FLAGS}")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS}")

    include(cmake/esp32_sdk_import.cmake)
    #set(COMPONENTS main)
    project(pwm_controller)
else()
    message(FATAL_ERROR "Invalid BUILD_PLATFORM, possible value: pico, esp32c3")
endif()
