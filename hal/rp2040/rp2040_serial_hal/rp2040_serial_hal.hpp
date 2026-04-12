#pragma once

#include <cstdint>
#include <vector>

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "result.hpp"
#include "serial_hal.hpp"

namespace serial {
using Baudrate = uint;
using BufferSize = uint32_t;
using Pin = uint8_t;

class RP2040SerialHal {
      public:
        RP2040SerialHal(uart_inst_t *uart, Pin tx_pin, Pin rx_pin,
                        Baudrate baudrate, uint16_t max_packet_size = 512,
                        uint32_t max_buffer_size = 2048);

        result::Try send(Data &&data);
        void on_receive(ReceiveCallback cb);
        result::Try loop();

      private:
        uart_inst_t *uart;
        Baudrate baudrate;
        Pin tx_pin;
        Pin rx_pin;
        ReceiveCallback receive_callback;
        uint16_t max_packet_size;
        uint32_t max_buffer_size;
        Data buffer;
};
} // namespace serial
