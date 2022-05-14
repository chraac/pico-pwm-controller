// See also: https://github.dev/espressif/esp-idf/blob/c138e47f45299e7fecacb3682211163e43b88df9/examples/bluetooth/nimble/ble_spp/spp_server/main/gatt_svr.c#L91


#pragma once

#include <services/gatt/ble_svc_gatt.h>

#include "logger.hh"

#define BLE_UUID128_INIT_CPP(...) \
    { {BLE_UUID_TYPE_128}, __VA_ARGS__ }

#define BLE_UUID16_INIT_CPP(uuid16) \
    { {BLE_UUID_TYPE_16}, uuid16 }

extern "C" void ble_store_config_init(void);

namespace utility {

namespace ble {

/* 16 Bit Alert Notification Service UUID */
constexpr ble_uuid16_t kBleSvcAnsUuid16 = BLE_UUID16_INIT_CPP(0x1811);

/* 16 Bit Alert Notification Service UUID */
constexpr ble_uuid16_t kBleSvcAnsChrUuid16SupNewAlertCat =
    BLE_UUID16_INIT_CPP(0x2a47);

/* 16 Bit SPP Service UUID */
constexpr ble_uuid16_t kBleSvcSppUuid16 = BLE_UUID16_INIT_CPP(0xABF0);

/* 16 Bit SPP Service Characteristic UUID */
constexpr ble_uuid16_t kBleSvcSppChrUuid16 = BLE_UUID16_INIT_CPP(0xABF1);

/* 59462f12-9543-9999-12c8-58b459a2712d */
constexpr ble_uuid128_t kGattSvrSvcSecTestUuid =
    BLE_UUID128_INIT_CPP(0x2d, 0x71, 0xa2, 0x59, 0xb4, 0x58, 0xc8, 0x12, 0x99,
                         0x99, 0x43, 0x95, 0x12, 0x2f, 0x46, 0x59);

/* 5c3a659e-897e-45e1-b016-007107c96df6 */
constexpr ble_uuid128_t kGattSvrChrSecTestRandUuid =
    BLE_UUID128_INIT_CPP(0xf6, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0, 0xe1,
                         0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

/* 5c3a659e-897e-45e1-b016-007107c96df7 */
constexpr ble_uuid128_t kGattSvrChrSecTestStaticUuid =
    BLE_UUID128_INIT_CPP(0xf7, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0, 0xe1,
                         0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

/* Callback function for custom service */
int BleSvcGattHandler(uint16_t conn_handle, uint16_t attr_handle,
                      struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            log_info("Callback for read");
            break;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            log_info(
                "Data received in write event,conn_handle = %x,attr_handle = "
                "%x",
                conn_handle, attr_handle);
            break;

        default:
            log_info("\nDefault Callback");
            break;
    }

    return 0;
}

int GattSvrChrWrite(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                    void *dst, uint16_t *len) {
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

int GattSvrChrAccessSecTest(uint16_t conn_handle, uint16_t attr_handle,
                            ble_gatt_access_ctxt *ctxt, void *arg) {
    const ble_uuid_t *uuid = ctxt->chr->uuid;
    uint8_t &gatt_svr_sec_test_static_val = *reinterpret_cast<uint8_t *>(arg);

    /* Determine which characteristic is being accessed by examining its
     * 128-bit UUID.
     */

    int rc;
    if (ble_uuid_cmp(uuid, &kGattSvrChrSecTestRandUuid.u) == 0) {
        assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);

        /* Respond with a 32-bit random number. */
        int rand_num = rand();
        rc = os_mbuf_append(ctxt->om, &rand_num, sizeof rand_num);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (ble_uuid_cmp(uuid, &kGattSvrChrSecTestStaticUuid.u) == 0) {
        switch (ctxt->op) {
            case BLE_GATT_ACCESS_OP_READ_CHR:
                rc = os_mbuf_append(ctxt->om, &gatt_svr_sec_test_static_val,
                                    sizeof gatt_svr_sec_test_static_val);
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                rc = GattSvrChrWrite(ctxt->om,
                                     sizeof gatt_svr_sec_test_static_val,
                                     sizeof gatt_svr_sec_test_static_val,
                                     &gatt_svr_sec_test_static_val, nullptr);
                return rc;

            default:
                assert(0);
                return BLE_ATT_ERR_UNLIKELY;
        }
    }

    /* Unknown characteristic; the nimble stack should not have called this
     * function.
     */
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

int GattSvrInit(uint8_t &gatt_svr_sec_test_static_val) {
    const ble_gatt_chr_def kGattChrs[] = {
        {
            /*** Characteristic: Random number generator. */
            .uuid = &kGattSvrChrSecTestRandUuid.u,
            .access_cb = GattSvrChrAccessSecTest,
            .arg = &gatt_svr_sec_test_static_val,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
        },
        {
            /*** Characteristic: Static value. */
            .uuid = &kGattSvrChrSecTestStaticUuid.u,
            .access_cb = GattSvrChrAccessSecTest,
            .arg = &gatt_svr_sec_test_static_val,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                     BLE_GATT_CHR_F_WRITE_ENC,
        },
        {
            /* No more characteristics in this service. */
        }};

    const ble_gatt_svc_def kGattSvrSvcs[] = {
        {
            /*** Service: Security test. */
            .type = BLE_GATT_SVC_TYPE_PRIMARY,
            .uuid = &kGattSvrSvcSecTestUuid.u,
            .characteristics = kGattChrs,
        },
        {
            /* No more services. */
        },
    };

    ble_svc_gap_init();
    ble_svc_gatt_init();

    int rc = ble_gatts_count_cfg(kGattSvrSvcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(kGattSvrSvcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int GattSvrRegister(uint16_t &ble_svc_gatt_read_val_handle,
                    uint16_t &ble_spp_svc_gatt_read_val_handle) {
    const ble_gatt_chr_def kBleSvcGattChrs[] = {
        {
            /* Support new alert category */
            .uuid = reinterpret_cast<const ble_uuid_t *>(
                &kBleSvcAnsChrUuid16SupNewAlertCat),
            .access_cb = BleSvcGattHandler,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                     BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_INDICATE,
            .val_handle = &ble_svc_gatt_read_val_handle,
        },
        {
            /* No more characteristics */
        }};

    const ble_gatt_chr_def kBleSvcSppChrs[] = {
        {
            /* Support SPP service */
            .uuid = reinterpret_cast<const ble_uuid_t *>(&kBleSvcSppChrUuid16),
            .access_cb = BleSvcGattHandler,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                     BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_INDICATE,
            .val_handle = &ble_spp_svc_gatt_read_val_handle,
        },
        {
            /* No more characteristics */
        }};

    /* Define new custom service */
    const ble_gatt_svc_def kBleSvcGattDefs[] = {
        {
            /*** Service: GATT */
            .type = BLE_GATT_SVC_TYPE_PRIMARY,
            .uuid = reinterpret_cast<const ble_uuid_t *>(&kBleSvcAnsUuid16),
            .characteristics = kBleSvcGattChrs,
        },
        {
            /*** Service: SPP */
            .type = BLE_GATT_SVC_TYPE_PRIMARY,
            .uuid = reinterpret_cast<const ble_uuid_t *>(&kBleSvcSppUuid16),
            .characteristics = kBleSvcSppChrs,
        },
        {
            /* No more services. */
        },
    };

    int rc = ble_gatts_count_cfg(kBleSvcGattDefs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(kBleSvcGattDefs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

}  // namespace ble

}  // namespace utility
