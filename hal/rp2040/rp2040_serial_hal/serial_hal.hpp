#pragma once

#include <cstdint>
#include <vector>

#include "i_serial_hal.hpp"
#include "result.hpp"

namespace serial {
class SerialHal : public ISerialHal {
      public:
        SerialHal();
        result::Result<bool> send(Data &&data) override;
        void on_receive(ReceiveCallback cb) override;
        result::Result<bool> loop() override;

      private:
        static constexpr auto BAUDRATE = 115200;
        static constexpr auto TX_PIN = 8;
        static constexpr auto RX_PIN = 9;
        static constexpr auto BUF_SIZE = 1024;
        ReceiveCallback receive_callback;
        Data buffer;
};
} // namespace serial
