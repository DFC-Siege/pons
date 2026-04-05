#pragma once

#include <cstdint>
#include <functional>
#include <hal/uart_types.h>
#include <string>
#include <vector>

#include "i_serial_hal.hpp"
#include "result.hpp"

namespace serial {
using Baudrate = int;
using BufferSize = uint32_t;
using Pin = uint8_t;

class SerialHal : public ISerialHal {
      public:
        SerialHal(uart_port_t uart, Pin tx_pin, Pin rx_pin, Baudrate baudrate,
                  BufferSize buffer_size);
        result::Try send(Data &&data) override;
        void on_receive(ReceiveCallback cb) override;
        result::Try loop() override;

      private:
        Baudrate baudrate;
        BufferSize buffer_size;
        uart_port_t uart;
        Pin tx_pin;
        Pin rx_pin;
        ReceiveCallback receive_callback;
        Data buffer;
};
} // namespace serial
