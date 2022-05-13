// clang-format off
#include "ble_spp_helper.hh"
// clang-format on

#include <esp_nimble_hci.h>
#include <host/util/util.h>
#include <nimble/ble.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <nvs_flash.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

#include "logger.hh"

#define BLE_UUID128_INIT_CPP(...) \
    { {BLE_UUID_TYPE_128}, __VA_ARGS__ }

#define BLE_UUID16_INIT_CPP(uuid16) \
    { {BLE_UUID_TYPE_16}, uuid16 }

#define BLE_UUID16_DECLARE_CPP(uuid16) \
    ((ble_uuid16_t)BLE_UUID16_INIT_CPP(uuid16))

using namespace utility;

namespace {

constexpr ble_uuid16_t kGattSvrSvcAlterUuid16 = BLE_UUID16_INIT_CPP(0x1811);

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

// TODO: make this values as class member

void PrintBleAddr(const uint8_t *addr) {
    log_info("%02x:%02x:%02x:%02x:%02x:%02x", addr[5], addr[4], addr[3],
             addr[2], addr[1], addr[0]);
}

extern "C" void ble_store_config_init(void);

/**
 * Logs information about a connection to the console.
 */
void BleSppServerPrintConnDesc(ble_gap_conn_desc *desc) {
    log_info("handle=%d our_ota_addr_type=%d our_ota_addr=", desc->conn_handle,
             desc->our_ota_addr.type);
    PrintBleAddr(desc->our_ota_addr.val);
    log_info(" our_id_addr_type=%d our_id_addr=", desc->our_id_addr.type);
    PrintBleAddr(desc->our_id_addr.val);
    log_info(" peer_ota_addr_type=%d peer_ota_addr=", desc->peer_ota_addr.type);
    PrintBleAddr(desc->peer_ota_addr.val);
    log_info(" peer_id_addr_type=%d peer_id_addr=", desc->peer_id_addr.type);
    PrintBleAddr(desc->peer_id_addr.val);
    log_info(
        " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
        "encrypted=%d authenticated=%d bonded=%d\n",
        desc->conn_itvl, desc->conn_latency, desc->supervision_timeout,
        desc->sec_state.encrypted, desc->sec_state.authenticated,
        desc->sec_state.bonded);
}

/* Callback function for custom service */
static int BleSvcGattHandler(uint16_t conn_handle, uint16_t attr_handle,
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

void BleSppUartTask(void *param) {
    reinterpret_cast<BleSppHelper *>(param)->UartTask();
}

void BleSppUartInit(const uint32_t uart_num, void *param,
                    QueueHandle_t &spp_common_uart_queue) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_RTS,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_APB,
    };

    // Install UART driver, and get the queue.
    uart_driver_install(uart_num, 4096, 8192, 10, &spp_common_uart_queue, 0);
    // Set UART parameters
    uart_param_config(uart_num, &uart_config);
    // Set UART pins
    uart_set_pin(uart_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    xTaskCreate(BleSppUartTask, "BleSppUartTask", 2048, (void *)param, 8,
                nullptr);
}

void BleSppServerOnReset(int reason) {
    log_info("BleSppServerOnReset, reason: %d\n", reason);
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * ble_spp_server uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unused by
 *                                  ble_spp_server.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
int BleSppGapEvent(ble_gap_event *event, void *arg) {
    return reinterpret_cast<BleSppHelper *>(arg)->GapEvent(event);
}

void BleSppServerOnSync() {
    int rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    uint8_t addr_type;
    rc = ble_hs_id_infer_auto(0, &addr_type);
    if (rc != 0) {
        log_info("error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    ble_hs_id_copy_addr(addr_type, addr_val, nullptr);
    log_info("Device Address: ");
    PrintBleAddr(addr_val);
    log_info("\n");

    /* Begin advertising. */
    BleSppHelper::GetInstance().Advertise(addr_type);
}

void BleSppServerOnRegister(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    char buffer[BLE_UUID_STR_LEN + 1] = {};

    switch (ctxt->op) {
        case BLE_GATT_REGISTER_OP_SVC:
            log_debug("Registered service %s with handle=%d\n",
                      ble_uuid_to_str(ctxt->svc.svc_def->uuid, buffer),
                      ctxt->svc.handle);
            break;

        case BLE_GATT_REGISTER_OP_CHR:
            log_debug(
                "Registering characteristic %s with "
                "def_handle=%d val_handle=%d\n",
                ble_uuid_to_str(ctxt->chr.chr_def->uuid, buffer),
                ctxt->chr.def_handle, ctxt->chr.val_handle);
            break;

        case BLE_GATT_REGISTER_OP_DSC:
            log_debug("Registering descriptor %s with handle=%d\n",
                      ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buffer),
                      ctxt->dsc.handle);
            break;

        default:
            assert(0);
            break;
    }
}

void BleSppServerHostTask(void *param) {
    log_info("BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

}  // namespace

bool BleSppHelper::Init(const uint32_t uart_num) {
    /* Initialize NVS — it is used to store PHY calibration data */
    uart_num_ = uart_num;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());
    nimble_port_init();

    /* Initialize uart driver and start uart task */
    BleSppUartInit(uart_num_, this, spp_common_uart_queue_);
    ble_hs_cfg.reset_cb = BleSppServerOnReset;
    ble_hs_cfg.sync_cb = BleSppServerOnSync;
    ble_hs_cfg.gatts_register_cb = BleSppServerOnRegister;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;

    int rc = GattSvrInit(gatt_svr_sec_test_static_val_);
    if (rc != 0) {
        log_info("error on new_gatt_svr_init, rc: %d\n", rc);
        return false;
    }
    /* Register custom service */
    rc = GattSvrRegister(ble_svc_gatt_read_val_handle_,
                         ble_spp_svc_gatt_read_val_handle_);
    if (rc != 0) {
        log_info("error on gatt_svr_register, rc: %d\n", rc);
        return false;
    }

    /* Set the default device name. */
    rc = ble_svc_gap_device_name_set("esp32c3-spp-svr");
    if (rc != 0) {
        log_info("error on ble_svc_gap_device_name_set, rc: %d\n", rc);
        return false;
    }

    /* XXX Need to have template for store */
    ble_store_config_init();

    nimble_port_freertos_init(BleSppServerHostTask);
    return true;
}

void BleSppHelper::UartTask() {
    log_info("BLE server UART_task started\n");
    uart_event_t event;
    int rc = 0;
    for (;;) {
        // Waiting for UART event.
        if (xQueueReceive(spp_common_uart_queue_, (void *)&event,
                          (TickType_t)portMAX_DELAY)) {
            switch (event.type) {
                // Event of UART receving data
                case UART_DATA:
                    if (event.size && (is_connected_ == true)) {
                        uint8_t ntf[1];
                        ntf[0] = 90;
                        struct os_mbuf *txom;
                        txom = ble_hs_mbuf_from_flat(ntf, sizeof(ntf));
                        rc = ble_gattc_notify_custom(
                            connection_handle_,
                            ble_spp_svc_gatt_read_val_handle_, txom);
                        if (rc == 0) {
                            log_info("Notification sent successfully");
                        } else {
                            log_info("Error in sending notification");
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
    vTaskDelete(nullptr);
}

/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
void BleSppHelper::Advertise(const uint8_t addr_type) {
    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */

    /* Advertise two flags:
     *     o Discoverability in forthcoming advertisement (general)
     *     o BLE-only (BR/EDR unsupported).
     */
    ble_hs_adv_fields fields = {};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    const auto *name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    fields.uuids16 = &kGattSvrSvcAlterUuid16;
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    addr_type_ = addr_type;
    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        log_info("error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising. */
    ble_gap_adv_params adv_params = {};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(addr_type, nullptr, BLE_HS_FOREVER, &adv_params,
                           BleSppGapEvent, this);
    if (rc != 0) {
        log_info("error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

int BleSppHelper::GapEvent(ble_gap_event *event) {
    ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            /* A new connection was established or a connection attempt failed.
             */
            log_info("connection %s; status=%d ",
                     event->connect.status == 0 ? "established" : "failed",
                     event->connect.status);
            if (event->connect.status == 0) {
                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                BleSppServerPrintConnDesc(&desc);
                is_connected_ = true;
                connection_handle_ = event->connect.conn_handle;
            }
            log_info("\n");
            if (event->connect.status != 0) {
                /* Connection failed; resume advertising. */
                BleSppHelper::GetInstance().Advertise();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            log_info("disconnect; reason=%d ", event->disconnect.reason);
            BleSppServerPrintConnDesc(&event->disconnect.conn);
            MODLOG_DFLT(INFO, "\n");

            /* Connection terminated; resume advertising. */
            BleSppHelper::GetInstance().Advertise();
            return 0;

        case BLE_GAP_EVENT_CONN_UPDATE:
            /* The central has updated the connection parameters. */
            log_info("connection updated; status=%d ",
                     event->conn_update.status);
            rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
            assert(rc == 0);
            BleSppServerPrintConnDesc(&desc);
            log_info("\n");
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            log_info("advertise complete; reason=%d",
                     event->adv_complete.reason);
            BleSppHelper::GetInstance().Advertise();
            return 0;

        case BLE_GAP_EVENT_MTU:
            log_info("mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                     event->mtu.conn_handle, event->mtu.channel_id,
                     event->mtu.value);
            return 0;

        default:
            return 0;
    }
}