#pragma once

#include <cstdint>
#include <string_view>

#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "services/gatt/ble_svc_gatt.h"

#include "i_ble_hal.hpp"

namespace ble {

class BleHal : public IBleHal {
      public:
        static BleHal &instance() {
                static BleHal inst;
                return inst;
        }

        void begin(std::string_view device_name);

        result::Result<bool> send(std::span<const uint8_t> data) override;
        void on_receive(ReceiveCallback cb) override {
                receive_callback = std::move(cb);
        }
        void on_connection_changed(ConnectionCallback cb) override {
                connection_callback = std::move(cb);
        }
        bool is_connected() const override {
                return conn_handle != BLE_HS_CONN_HANDLE_NONE;
        }

        BleHal(const BleHal &) = delete;
        BleHal &operator=(const BleHal &) = delete;

        static int on_gap_event(ble_gap_event *event, void *arg);
        static int on_gatt_access(uint16_t conn_handle, uint16_t attr_handle,
                                  ble_gatt_access_ctxt *ctxt, void *arg);

      private:
        uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
        static uint16_t tx_attr_handle;
        static constexpr ble_uuid128_t service_uuid =
            BLE_UUID128_INIT(0x6e, 0x40, 0x00, 0x01, 0xb5, 0xa3, 0xf3, 0x93,
                             0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e);
        static constexpr ble_uuid128_t rx_uuid =
            BLE_UUID128_INIT(0x6e, 0x40, 0x00, 0x02, 0xb5, 0xa3, 0xf3, 0x93,
                             0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e);
        static constexpr ble_uuid128_t tx_uuid =
            BLE_UUID128_INIT(0x6e, 0x40, 0x00, 0x03, 0xb5, 0xa3, 0xf3, 0x93,
                             0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e);
        static const ble_gatt_svc_def gatt_services[];
        ReceiveCallback receive_callback;
        ConnectionCallback connection_callback;

        BleHal() = default;

        void start_advertising();
};

} // namespace ble
