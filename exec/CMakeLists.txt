add_executable(
    pwm_controller
    frequency_counter.cc
    fan_speed_manager.cc
    pwm_controller.cc
)

if(USB_STDIO)
    pico_enable_stdio_usb(pwm_controller 1)
    target_compile_definitions(pwm_controller PUBLIC USB_STDIO)
else()
    pico_enable_stdio_uart(pwm_controller 1)
    target_compile_definitions(pwm_controller PUBLIC UART_STDIO)
endif()

target_link_libraries(pwm_controller pico_stdlib hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(pwm_controller)
