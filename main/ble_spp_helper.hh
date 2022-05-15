#pragma once

#include "base_types.hh"

#ifdef PLATFORM_ESP32
#include <driver/uart.h>
#include <freertos/stream_buffer.h>
#include <host/ble_hs.h>
#endif

namespace utility {

class BleSppHelper : public Singleton<BleSppHelper> {
public:
    BleSppHelper() = default;
    ~BleSppHelper() = default;

    bool Init();

#ifdef PLATFORM_ESP32
    void LogWithBuffer(const char *buffer, size_t size);
    void LogTask();
    void Advertise(const uint8_t addr_type);
    void Advertise() { Advertise(addr_type_); }
    int GapEvent(ble_gap_event *event);
#endif

private:
#ifdef PLATFORM_ESP32
    StreamBufferHandle_t log_buffer_ = nullptr;
    bool is_connected_ = false;
    uint8_t addr_type_ = 0;
    uint8_t gatt_svr_sec_test_static_val_ = 0;
    uint16_t ble_svc_gatt_read_val_handle_ = 0;
    uint16_t ble_spp_svc_gatt_read_val_handle_ = 0;
    uint16_t connection_handle_ = 0;
#endif

    DISALLOW_COPY(BleSppHelper);
    DISALLOW_MOVE(BleSppHelper);
};

}  // namespace utility
