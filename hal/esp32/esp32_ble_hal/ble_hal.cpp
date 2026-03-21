#include <cstring>
#include <vector>

#include "esp_log.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "result.hpp"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "ble_hal.hpp"

namespace ble {

static constexpr char TAG[] = "BleHal";

uint16_t BleHal::tx_attr_handle = 0;

const ble_gatt_svc_def BleHal::gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &service_uuid.u,
        .characteristics =
            (ble_gatt_chr_def[]){
                {
                    .uuid = &rx_uuid.u,
                    .access_cb = BleHal::on_gatt_access,
                    .arg = nullptr,
                    .descriptors = nullptr,
                    .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                },
                {
                    .uuid = &tx_uuid.u,
                    .access_cb = BleHal::on_gatt_access,
                    .arg = nullptr,
                    .descriptors = nullptr,
                    .flags = BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &tx_attr_handle,
                },
                {0},
            },
    },
    {0},
};

void BleHal::begin(std::string_view device_name) {
        nimble_port_init();

        ble_hs_cfg.sync_cb = []() { instance().start_advertising(); };

        ble_svc_gap_init();
        ble_svc_gatt_init();
        ble_gatts_count_cfg(gatt_services);
        ble_gatts_add_svcs(gatt_services);
        ble_svc_gap_device_name_set(device_name.data());

        nimble_port_freertos_init([](void *) {
                nimble_port_run();
                nimble_port_freertos_deinit();
        });
}

result::Result<bool> BleHal::send(std::span<const uint8_t> data) {
        if (!is_connected()) {
                return result::err("ble not connected");
        }

        os_mbuf *om = ble_hs_mbuf_from_flat(data.data(), data.size());
        if (!om) {
                return result::err("error creating buffer");
        }

        const auto result =
            ble_gatts_notify_custom(conn_handle, tx_attr_handle, om);
        if (result != 0) {
                return result::err("error sending data");
        }

        return result::ok();
}

void BleHal::start_advertising() {
        ble_hs_adv_fields fields = {};
        fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
        fields.tx_pwr_lvl_is_present = 1;
        fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

        const char *name = ble_svc_gap_device_name();
        fields.name = reinterpret_cast<const uint8_t *>(name);
        fields.name_len = strlen(name);
        fields.name_is_complete = 1;

        ble_gap_adv_set_fields(&fields);

        ble_gap_adv_params adv_params = {};
        adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
        adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

        ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, nullptr, BLE_HS_FOREVER,
                          &adv_params, on_gap_event, nullptr);
}

int BleHal::on_gap_event(ble_gap_event *event, void *) {
        auto &self = instance();

        switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
                if (event->connect.status == 0) {
                        self.conn_handle = event->connect.conn_handle;
                        if (self.connection_callback)
                                self.connection_callback(true);
                } else {
                        self.start_advertising();
                }
                break;

        case BLE_GAP_EVENT_DISCONNECT:
                self.conn_handle = BLE_HS_CONN_HANDLE_NONE;
                if (self.connection_callback)
                        self.connection_callback(false);
                self.start_advertising();
                break;

        case BLE_GAP_EVENT_MTU:
                ESP_LOGI(TAG, "MTU updated: %d", event->mtu.value);
                break;
        }

        return 0;
}

int BleHal::on_gatt_access(uint16_t, uint16_t, ble_gatt_access_ctxt *ctxt,
                           void *) {
        auto &self = instance();

        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
                uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
                std::vector<uint8_t> buf(len);
                ble_hs_mbuf_to_flat(ctxt->om, buf.data(), len, nullptr);
                if (self.receive_callback) {
                        self.receive_callback(std::move(buf));
                }
        }

        return 0;
}

} // namespace ble
