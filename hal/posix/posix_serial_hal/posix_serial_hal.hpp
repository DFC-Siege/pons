#pragma once

#include <cstdint>
#include <functional>
#include <termios.h>
#include <vector>

#include "result.hpp"
#include "serial_hal.hpp"

namespace serial {

class PosixSerialHal {
      public:
        explicit PosixSerialHal(const char *device, int baud_rate = B115200,
                                uint16_t max_packet_size = 512,
                                uint32_t max_buffer_size = 2048);
        ~PosixSerialHal();
        PosixSerialHal(const PosixSerialHal &) = delete;
        PosixSerialHal &operator=(const PosixSerialHal &) = delete;
        result::Try send(Data &&data);
        void on_receive(ReceiveCallback cb);
        result::Try loop();

      private:
        int fd;
        ReceiveCallback receive_callback;
        std::vector<uint8_t> buffer;
        uint16_t max_packet_size;
        uint32_t max_buffer_size;
        static constexpr auto BUF_SIZE = 1024;
        static constexpr auto LENGTH_PREFIX_SIZE = sizeof(uint16_t);
};

} // namespace serial
