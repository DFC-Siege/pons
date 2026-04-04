#pragma once

#include <cstdint>
#include <functional>
#include <termios.h>

#include "i_serial_hal.hpp"
#include "result.hpp"

namespace serial {
class SerialHal : public ISerialHal {
      public:
        explicit SerialHal(const char *device, int baud_rate = B115200);
        ~SerialHal();

        SerialHal(const SerialHal &) = delete;
        SerialHal &operator=(const SerialHal &) = delete;

        result::Result<bool> send(Data &&data) override;
        void on_receive(ReceiveCallback cb) override;
        result::Result<bool> loop() override;

      private:
        int fd;
        ReceiveCallback receive_callback;
        static constexpr auto BUF_SIZE = 1024;
};
} // namespace serial
