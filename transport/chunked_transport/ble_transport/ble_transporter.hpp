#pragma once

#include <cstdint>
#include <memory>

#include "chunked_transporter.hpp"
#include "i_ble_hal.hpp"
#include "i_transport.hpp"
#include "result.hpp"

namespace transport {
class BleTransporter : public ChunkedTransporter {
      public:
        BleTransporter(uint16_t mtu, ble::IBleHal &ble_hal);

        result::Result<bool>
        concrete_send(std::span<const uint8_t> data) override;

      private:
        static constexpr auto TAG = "BleTransporter";
        ble::IBleHal &ble_hal;
};
} // namespace transport
