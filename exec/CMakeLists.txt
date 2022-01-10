add_executable(
    pwm_controller
    pwm_controller.cc
)

target_link_libraries(pwm_controller pico_stdlib hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(pwm_controller)
