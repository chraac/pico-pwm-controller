
add_library(pico-ssd1306 STATIC
    pico-ssd1306/ssd1306.c
)

target_include_directories(pico-ssd1306 PUBLIC
    pico-ssd1306
)

target_link_libraries(pico-ssd1306 PUBLIC
    pico_stdlib
    hardware_i2c
)
