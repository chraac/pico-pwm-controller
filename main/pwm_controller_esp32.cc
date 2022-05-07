
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "logger.hh"

extern "C" void app_main() { log_debug("app_main.init\n"); }