
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "logger.hh"
#include "pwm_helper.hh"

using namespace utility;

namespace {

constexpr gpio_num_t kGpioPwm4 = GPIO_NUM_1;
constexpr gpio_num_t kGpioPwm3 = GPIO_NUM_2;
constexpr gpio_num_t kGpioPwm2 = GPIO_NUM_6;
constexpr gpio_num_t kGpioPwm1 = GPIO_NUM_7;
constexpr uint32_t kPwmFreqKhz = 25;

constexpr gpio_num_t kGpioRed = GPIO_NUM_3;
constexpr gpio_num_t kGpioGreen = GPIO_NUM_4;
constexpr gpio_num_t kGpioBlue = GPIO_NUM_5;

}  // namespace

extern "C" void app_main() {
    log_debug("app_main.init\n");

    auto pwm1 = PwmHelper(kGpioPwm1, kPwmFreqKhz);
    auto pwm2 = PwmHelper(kGpioPwm2, kPwmFreqKhz);
    auto pwm3 = PwmHelper(kGpioPwm3, kPwmFreqKhz);
    auto pwm4 = PwmHelper(kGpioPwm4, kPwmFreqKhz);
    pwm1.SetDutyCycle(2000);
    pwm2.SetDutyCycle(4000);
    pwm3.SetDutyCycle(6000);
    pwm4.SetDutyCycle(8000);

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