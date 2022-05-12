# It seems that the express idf build system will call the CMakeLists.txt in sub directory from a separated process without calling makin directory CMakeLists.txt.
# See also: https://github.dev/espressif/esp-idf/blob/273633ee310fbc18b17edfaeae3f3121508e3b8d/tools/cmake/scripts/component_get_requirements.cmake#L118
cmake_minimum_required(VERSION 3.12)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/platform_detect.cmake)

message("PLATFORM_ESP32: ${PLATFORM_ESP32}")
message("PLATFORM_PICO: ${PLATFORM_PICO}")

if(PLATFORM_PICO)
    add_executable(
        pwm_controller
        pwm_helper_pico.cc
        frequency_counter_pico.cc
        pwm_controller_pico.cc
    )
    if (USB_STDIO)
        pico_enable_stdio_usb(pwm_controller 1)
        target_compile_definitions(pwm_controller PUBLIC USB_STDIO)
    else()
        pico_enable_stdio_uart(pwm_controller 1)
        target_compile_definitions(pwm_controller PUBLIC UART_STDIO)
    endif()

    target_link_libraries(pwm_controller pico_stdlib hardware_pwm)

    # create map/bin/hex file etc.
    pico_add_extra_outputs(pwm_controller)
elseif(PLATFORM_ESP32)
    set(exec_srcs
        "pwm_helper_esp32.cc"
        "pwm_controller_esp32.cc"
    )
    idf_component_register(SRCS "${exec_srcs}"
        INCLUDE_DIRS .
        PRIV_REQUIRES driver)
endif()
