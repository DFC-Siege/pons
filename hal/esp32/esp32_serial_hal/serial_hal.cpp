#include <algorithm>
#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <hal/uart_types.h>

#include "logger.hpp"
#include "result.hpp"
#include "serial_hal.hpp"

namespace serial {

static constexpr auto TAG = "SerialHal";

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
        logging::logger().println(logging::LogLevel::Debug, TAG,
                                  "initialized uart=" + std::to_string(uart) +
                                      " tx=" + std::to_string(tx_pin) +
                                      " rx=" + std::to_string(rx_pin) +
                                      " baud=" + std::to_string(baudrate));
}

result::Try SerialHal::send(Data &&data) {
        if (data.empty()) {
                logging::logger().println(
                    logging::LogLevel::Debug, TAG,
                    "send called with empty data, skipping");
                return result::ok();
        }
        logging::logger().println(logging::LogLevel::Debug, TAG,
                                  "sending " + std::to_string(data.size()) +
                                      " bytes");
        const uint16_t length = static_cast<uint16_t>(data.size());
        const uint8_t prefix[2] = {static_cast<uint8_t>(length & 0xFF),
                                   static_cast<uint8_t>((length >> 8) & 0xFF)};
        uart_write_bytes(uart, prefix, sizeof(prefix));
        const auto response = uart_write_bytes(uart, data.data(), data.size());
        if (response < 0) {
                logging::logger().println(logging::LogLevel::Error, TAG,
                                          "uart_write_bytes failed with " +
                                              std::to_string(response));
                return result::err(
                    "something went wrong while sending over serial");
        }
        logging::logger().println(logging::LogLevel::Debug, TAG,
                                  "sent " + std::to_string(response) +
                                      " bytes");
        return result::ok();
}

void SerialHal::on_receive(ReceiveCallback cb) {
        logging::logger().println(logging::LogLevel::Debug, TAG,
                                  "receive callback registered");
        receive_callback = std::move(cb);
}

result::Try SerialHal::loop() {
        uart_event_t event;
        if (xQueueReceive(event_queue, &event, pdMS_TO_TICKS(10)) != pdTRUE) {
                return result::ok();
        }
        logging::logger().println(
            logging::LogLevel::Debug, TAG,
            "uart event type=" + std::to_string(event.type) +
                " size=" + std::to_string(event.size));
        if (event.type != UART_DATA) {
                logging::logger().println(logging::LogLevel::Debug, TAG,
                                          "ignoring non-data event");
                return result::ok();
        }
        const int length = uart_read_bytes(uart, tmp.data(), event.size, 0);
        if (length < 0) {
                logging::logger().println(logging::LogLevel::Error, TAG,
                                          "uart_read_bytes failed with " +
                                              std::to_string(length));
                return result::err(
                    "something went wrong while reading over serial");
        }
        logging::logger().println(logging::LogLevel::Debug, TAG,
                                  "read " + std::to_string(length) +
                                      " bytes, buffer size=" +
                                      std::to_string(buffer.size() + length));
        buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + length);
        size_t consumed = 0;
        while (buffer.size() - consumed >= 2) {
                const uint16_t packet_length =
                    static_cast<uint16_t>(buffer[consumed]) |
                    (static_cast<uint16_t>(buffer[consumed + 1]) << 8);
                if (buffer.size() - consumed < 2 + packet_length) {
                        logging::logger().println(
                            logging::LogLevel::Debug, TAG,
                            "incomplete packet, expected=" +
                                std::to_string(packet_length) + " available=" +
                                std::to_string(buffer.size() - consumed - 2));
                        break;
                }
                logging::logger().println(logging::LogLevel::Debug, TAG,
                                          "dispatching packet length=" +
                                              std::to_string(packet_length));
                Data packet(buffer.begin() + consumed + 2,
                            buffer.begin() + consumed + 2 + packet_length);
                consumed += 2 + packet_length;
                if (receive_callback) {
                        receive_callback(std::move(packet));
                }
        }
        if (consumed > 0) {
                logging::logger().println(logging::LogLevel::Debug, TAG,
                                          "consumed " +
                                              std::to_string(consumed) +
                                              " bytes from buffer");
                buffer.erase(buffer.begin(), buffer.begin() + consumed);
        }
        return result::ok();
}

} // namespace serial
