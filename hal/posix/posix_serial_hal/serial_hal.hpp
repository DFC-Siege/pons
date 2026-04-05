#pragma once
#include "i_serial_hal.hpp"
#include "result.hpp"
#include <cstdint>
#include <functional>
#include <termios.h>
#include <vector>

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
        std::vector<uint8_t> buffer;
        static constexpr auto BUF_SIZE = 1024;
        static constexpr auto LENGTH_PREFIX_SIZE = sizeof(uint16_t);
};

} // namespace serial
