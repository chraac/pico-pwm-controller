#pragma once

#include "base_types.hh"

#ifdef PLATFORM_ESP32
#include <driver/uart.h>
#include <host/ble_hs.h>
#endif

namespace utility {

class BleSppHelper : public Singleton<BleSppHelper> {
public:
    BleSppHelper() = default;
    ~BleSppHelper() = default;

    bool Init(const uint32_t uart_num);

#ifdef PLATFORM_ESP32
    void UartTask();
    void Advertise(const uint8_t addr_type);
    void Advertise() { Advertise(addr_type_); }
    int GapEvent(ble_gap_event *event);
#endif

private:
    uint32_t uart_num_;
#ifdef PLATFORM_ESP32
    QueueHandle_t spp_common_uart_queue_ = nullptr;
    bool is_connected_ = false;
    uint8_t addr_type_ = 0;
    uint16_t connection_handle_ = 0;
#endif

    DISALLOW_COPY(BleSppHelper);
    DISALLOW_MOVE(BleSppHelper);
};

}  // namespace utility
