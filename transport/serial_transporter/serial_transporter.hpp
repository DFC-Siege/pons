#pragma once

#include <cstdint>
#include <memory>
#include <queue>
#include <vector>

#include "i_serial_hal.hpp"
#include "result.hpp"
#include "transporter.hpp"

namespace transport {
class SerialTransporter {
      public:
        SerialTransporter(serial::ISerialHal &serial_hal);
        result::Result<bool> send(DataView data);
        result::Result<bool> add_receiver(ReceiveCallback callback);

      private:
        serial::ISerialHal &serial_hal;
};
} // namespace transport
