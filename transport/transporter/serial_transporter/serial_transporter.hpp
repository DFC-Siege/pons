#pragma once

#include "base_transporter.hpp"
#include "i_serial_hal.hpp"
#include "result.hpp"
#include "transporter/base_transporter.hpp"

namespace transport {
class SerialTransporter : public BaseTransporter {
      public:
        explicit SerialTransporter(serial::ISerialHal &serial_hal, MTU mtu);
        result::Try send(Data &&data) override;
        MTU get_mtu() const override;

      private:
        static constexpr auto TAG = "SerialTransporter";
        serial::ISerialHal &serial_hal;
        MTU mtu;
};

static_assert(Transporter<SerialTransporter>);
} // namespace transport
