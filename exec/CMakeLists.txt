
add_executable(pwm_controller)

if (PLATFORM_PICO)
    target_sources(
        pwm_controller PRIVATE
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
elseif (PLATFORM_ESP32_C3)
    target_sources(
        pwm_controller PRIVATE
        pwm_controller_esp32.cc
    )
endif()
