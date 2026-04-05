#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <algorithm>
#include <driver/uart.h>
#include <hal/uart_types.h>

#include "result.hpp"
#include "serial_hal.hpp"

namespace serial {
SerialHal::SerialHal(uart_port_t uart, Pin tx_pin, Pin rx_pin,
                     Baudrate baudrate, BufferSize buffer_size)
    : baudrate(baudrate), buffer_size(buffer_size), uart(uart), tx_pin(tx_pin),
      rx_pin(rx_pin), tmp(buffer_size) {
        uart_config_t uart_config = {.baud_rate = baudrate,
                                     .data_bits = UART_DATA_8_BITS,
                                     .parity = UART_PARITY_DISABLE,
                                     .stop_bits = UART_STOP_BITS_1,
                                     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                                     .rx_flow_ctrl_thresh =
                                         UART_HW_FLOWCTRL_DISABLE,
                                     .source_clk = UART_SCLK_DEFAULT,
                                     .flags = {}};
        uart_driver_install(uart, buffer_size * 2, 0, 20, &event_queue, 0);
        uart_param_config(uart, &uart_config);
        uart_set_pin(uart, tx_pin, rx_pin, UART_PIN_NO_CHANGE,
                     UART_PIN_NO_CHANGE);
}

result::Try SerialHal::send(Data &&data) {
        if (data.empty()) {
                return result::ok();
        }

        const uint16_t length = static_cast<uint16_t>(data.size());
        const uint8_t prefix[2] = {static_cast<uint8_t>(length & 0xFF),
                                   static_cast<uint8_t>((length >> 8) & 0xFF)};
        uart_write_bytes(uart, prefix, sizeof(prefix));

        const auto response = uart_write_bytes(uart, data.data(), data.size());
        if (response < 0) {
                return result::err(
                    "something went wrong while sending over serial");
        }

        return result::ok();
}
void SerialHal::on_receive(ReceiveCallback cb) {
        receive_callback = std::move(cb);
}

result::Try SerialHal::loop() {
        uart_event_t event;
        if (xQueueReceive(event_queue, &event, pdMS_TO_TICKS(10)) != pdTRUE) {
                return result::ok();
        }
        if (event.type != UART_DATA) {
                return result::ok();
        }

        const int length = uart_read_bytes(uart, tmp.data(), event.size, 0);
        if (length < 0) {
                return result::err(
                    "something went wrong while reading over serial");
        }

        buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + length);

        size_t consumed = 0;
        while (buffer.size() - consumed >= 2) {
                const uint16_t packet_length =
                    static_cast<uint16_t>(buffer[consumed]) |
                    (static_cast<uint16_t>(buffer[consumed + 1]) << 8);
                if (buffer.size() - consumed < 2 + packet_length) {
                        break;
                }
                Data packet(buffer.begin() + consumed + 2,
                            buffer.begin() + consumed + 2 + packet_length);
                consumed += 2 + packet_length;
                if (receive_callback) {
                        receive_callback(std::move(packet));
                }
        }
        if (consumed > 0) {
                buffer.erase(buffer.begin(), buffer.begin() + consumed);
        }
        return result::ok();
}
} // namespace serial
