
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ble_spp_helper.hh"
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
    BleSppHelper::GetInstance().Init(UART_NUM_0);
    esp_log_level_set(LOG_TAG, DEFAULT_LOG_LEVEL);
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

    for (bool light_on = true;; light_on = !light_on) {
        gpio_set_level(kGpioRed, light_on);
        gpio_set_level(kGpioGreen, light_on);
        gpio_set_level(kGpioBlue, light_on);
        log_info("app_main.light_on: %d\n", int(light_on));
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // delay 1000ms
    }
}