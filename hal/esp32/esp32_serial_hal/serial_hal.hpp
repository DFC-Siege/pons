#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "i_serial_hal.hpp"
#include "result.hpp"

namespace serial {
class SerialHal : public ISerialHal {
      public:
        SerialHal();
        result::Result<bool> send(std::span<const uint8_t> data) override;
        result::Result<std::vector<uint8_t>> read(uint32_t timeout) override;

      private:
        // TODO: Move into constructor
        static constexpr auto BAUDRATE = 115200;
        static constexpr auto TX_PIN = 7;
        static constexpr auto RX_PIN = 6;
        static constexpr auto BUF_SIZE = 1024;
};
} // namespace serial
