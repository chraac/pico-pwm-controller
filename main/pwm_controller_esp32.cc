
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "logger.hh"


namespace {
constexpr gpio_num_t kGpioRed = GPIO_NUM_3;
constexpr gpio_num_t kGpioGreen = GPIO_NUM_4;
constexpr gpio_num_t kGpioBlue = GPIO_NUM_5;
}  // namespace

extern "C" void app_main() {
    log_debug("app_main.init\n");

    gpio_reset_pin(kGpioRed);
    gpio_set_direction(kGpioRed, GPIO_MODE_OUTPUT);
    gpio_reset_pin(kGpioGreen);
    gpio_set_direction(kGpioGreen, GPIO_MODE_OUTPUT);
    gpio_reset_pin(kGpioBlue);
    gpio_set_direction(kGpioBlue, GPIO_MODE_OUTPUT);
    for (uint32_t i = 0;; i = ((i + 1) % 3)) {
        gpio_set_level(kGpioRed, i % 3 == 0);
        gpio_set_level(kGpioGreen, i % 3 == 1);
        gpio_set_level(kGpioBlue, i % 3 == 2);
        vTaskDelay(500 / portTICK_PERIOD_MS);  // delay 500ms
    }
}