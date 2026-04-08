#pragma once

#include <cstdint>
#include <vector>

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "i_serial_hal.hpp"
#include "result.hpp"

namespace serial {
using Baudrate = uint;
using BufferSize = uint32_t;
using Pin = uint8_t;

class SerialHal : public ISerialHal {
      public:
        SerialHal(uart_inst_t *uart, Pin tx_pin, Pin rx_pin, Baudrate baudrate,
                  uint16_t max_packet_size = 512,
                  uint32_t max_buffer_size = 2048);

        result::Try send(Data &&data) override;
        void on_receive(ReceiveCallback cb) override;
        result::Try loop() override;

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
