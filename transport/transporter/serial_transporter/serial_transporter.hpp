#pragma once

#include "i_serial_hal.hpp"
#include "result.hpp"
#include "transporter/base_transporter.hpp"

namespace transport {
class SerialTransporter : public BaseTransporter {
      public:
        explicit SerialTransporter(serial::ISerialHal &serial_hal);
        result::Result<bool> send(DataView data) override;
        MTU get_mtu() const override;

      private:
        serial::ISerialHal &serial_hal;
};
} // namespace transport
