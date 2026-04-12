#pragma once

#include <cstdint>
#include <functional>
#include <hal/uart_types.h>
#include <string>
#include <vector>

#include "result.hpp"
#include "serial_hal.hpp"

struct QueueDefinition;
using QueueHandle_t = QueueDefinition *;

namespace serial {
using Baudrate = int;
using BufferSize = uint32_t;
using Pin = uint8_t;

class Esp32SerialHal {
      public:
        Esp32SerialHal(uart_port_t uart, Pin tx_pin, Pin rx_pin,
                       Baudrate baudrate, BufferSize buffer_size,
                       uint16_t max_packet_size = 512,
                       uint32_t max_buffer_size = 2048);
        result::Try send(Data &&data);
        void on_receive(ReceiveCallback cb);
        result::Try loop();

      private:
        Baudrate baudrate;
        BufferSize buffer_size;
        uart_port_t uart;
        Pin tx_pin;
        Pin rx_pin;
        ReceiveCallback receive_callback;
        uint16_t max_packet_size;
        uint32_t max_buffer_size;
        Data buffer;
        Data tmp;
        QueueHandle_t event_queue = nullptr;
};
} // namespace serial
