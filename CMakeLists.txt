cmake_minimum_required(VERSION 3.12)

set(BUILD_PLATFORM "pico" CACHE STRING "The target platform of the current build, possible value: pico, esp32-c3")

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
elseif (${BUILD_PLATFORM} STREQUAL "esp32-c3")
    set(PLATFORM_ESP32_C3 true)
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    include($ENV{IDF_PATH}/tools/cmake/utilities.cmake)
    # Use generator expression so that users can append/override flags even after call to
    # idf_build_process
    idf_build_get_property(include_directories INCLUDE_DIRECTORIES GENERATOR_EXPRESSION)
    idf_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(c_compile_options C_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(cxx_compile_options CXX_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(asm_compile_options ASM_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
    idf_build_get_property(common_reqs ___COMPONENT_REQUIRES_COMMON)

    include_directories("${include_directories}")
    add_compile_options("${compile_options}")
    add_c_compile_options("${c_compile_options}")
    add_cxx_compile_options("${cxx_compile_options}")
    add_asm_compile_options("${asm_compile_options}")
    add_compile_options("${compile_definitions}")
    link_libraries(${common_reqs})
  
    add_compile_definitions(PLATFORM_ESP32 PLATFORM_ESP32_C3)
else()
    message(FATAL_ERROR "Invalid BUILD_PLATFORM, possible value: pico, esp32-c3")
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

add_subdirectory(exec)