#pragma once

#include "base_types.hh"

#ifdef PLATFORM_PICO

#include <pico/stdio.h>

#include <cstdio>


#define log_info(format, args...) printf(format, ##args)
#ifdef DEBUG
#define log_debug(format, args...) printf(format, ##args)
#else
#define log_debug(format, args...) (void)0
#endif

#elif defined(PLATFORM_ESP32)

#include <esp_log.h>

#define LOG_TAG "pwm"
#define log_info(format, args...) ESP_LOGI(LOG_TAG, format, ##args)
#ifdef DEBUG
#define DEFAULT_LOG_LEVEL ESP_LOG_DEBUG
#define log_debug(format, args...) ESP_LOGD(LOG_TAG, format, ##args)
#else
#define DEFAULT_LOG_LEVEL ESP_LOG_INFO
#define log_debug(format, args...) (void)0
#endif

#endif
