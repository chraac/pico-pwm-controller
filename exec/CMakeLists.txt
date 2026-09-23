
set(PWM_CONTROLLER_LIB_SRC
    frequency_counter.cc
    fan_speed_manager.cc
)

set(PWM_CONTROLLER_TARGETS pwm_controller_lite pwm_controller pwm_controller_pcie)

add_executable(pwm_controller_lite ${PWM_CONTROLLER_LIB_SRC} pwm_controller_lite.cc)
add_executable(pwm_controller ${PWM_CONTROLLER_LIB_SRC} pwm_controller.cc)
add_executable(pwm_controller_pcie ${PWM_CONTROLLER_LIB_SRC} pwm_controller_pcie.cc)

foreach(target IN LISTS PWM_CONTROLLER_TARGETS)
    pico_generate_pio_header(${target} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

    if(USB_STDIO)
        pico_enable_stdio_usb(${target} 1)
        target_compile_definitions(${target} PUBLIC USB_STDIO)
    else()
        pico_enable_stdio_uart(${target} 1)
        target_compile_definitions(${target} PUBLIC UART_STDIO)
    endif()

    target_link_libraries(${target} pico_stdlib hardware_pwm hardware_adc hardware_i2c hardware_pio pico-ssd1306)

    # create map/bin/hex file etc.
    pico_add_extra_outputs(${target})
endforeach()
