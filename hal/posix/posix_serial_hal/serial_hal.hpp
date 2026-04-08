#pragma once

#include <cstdint>
#include <functional>
#include <termios.h>
#include <vector>

#include "i_serial_hal.hpp"
#include "result.hpp"

namespace serial {

class SerialHal : public ISerialHal {
      public:
        explicit SerialHal(const char *device, int baud_rate = B115200,
                           uint16_t max_packet_size = 512);
        ~SerialHal();
        SerialHal(const SerialHal &) = delete;
        SerialHal &operator=(const SerialHal &) = delete;
        result::Try send(Data &&data) override;
        void on_receive(ReceiveCallback cb) override;
        result::Try loop() override;

      private:
        int fd;
        ReceiveCallback receive_callback;
        std::vector<uint8_t> buffer;
        uint16_t max_packet_size;
        static constexpr auto BUF_SIZE = 1024;
        static constexpr auto LENGTH_PREFIX_SIZE = sizeof(uint16_t);
};

} // namespace serial
