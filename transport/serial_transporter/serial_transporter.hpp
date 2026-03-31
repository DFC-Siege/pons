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
        result::Result<bool> send(std::span<const uint8_t> data);
        result::Result<std::vector<uint8_t>> receive();

      private:
        static constexpr auto TIMEOUT = 1000;
        serial::ISerialHal &serial_hal;
};
} // namespace transport
