#include <algorithm>
#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <hal/uart_types.h>
#include <string>

#include "logger.hpp"
#include "result.hpp"
#include "serial_hal.hpp"

namespace serial {

static constexpr auto TAG = "SerialHal";

static std::string to_hex_string(const uint8_t *data, size_t len) {
        std::string hex;
        for (size_t i = 0; i < len; ++i) {
                hex += std::to_string(data[i]) + " ";
        }
        return hex;
}

SerialHal::SerialHal(uart_port_t uart, Pin tx_pin, Pin rx_pin,
                     Baudrate baudrate, BufferSize buffer_size,
                     uint16_t max_packet_size)
    : baudrate(baudrate), buffer_size(buffer_size), uart(uart), tx_pin(tx_pin),
      rx_pin(rx_pin), max_packet_size(max_packet_size), tmp(buffer_size) {
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

        uart_flush_input(uart);

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

        const uint16_t length = static_cast<uint16_t>(data.size());
        logging::logger().println(
            logging::LogLevel::Debug, TAG,
            "sending " + std::to_string(length) +
                " bytes: " + to_hex_string(data.data(), data.size()));

        const uint8_t prefix[2] = {static_cast<uint8_t>(length & 0xFF),
                                static_cast<uint8_t>((length >> 8) & 0xFF)};

        const auto prefix_response =
            uart_write_bytes(uart, prefix, sizeof(prefix));
        if (prefix_response < 0) {
                logging::logger().println(
                    logging::LogLevel::Error, TAG,
                    "uart_write_bytes prefix failed with " +
                        std::to_string(prefix_response));
                return result::err(
                    "something went wrong while sending prefix over serial");
        }
        const auto response = uart_write_bytes(uart, data.data(), data.size());

        if (response < 0) {
                logging::logger().println(logging::LogLevel::Error, TAG,
                                          "uart_write_bytes failed with " +
                                              std::to_string(response));
                return result::err(
                    "something went wrong while sending over serial");
        }

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

        if (event.type != UART_DATA) {
                return result::ok();
        }

        const int length = uart_read_bytes(uart, tmp.data(), event.size, 0);
        if (length < 0) {
                logging::logger().println(logging::LogLevel::Error, TAG,
                                          "uart_read_bytes failed");
                return result::err("read error");
        }

        buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + length);

        size_t consumed = 0;
        while (buffer.size() - consumed >= 2) {
                const uint16_t packet_length =
                    static_cast<uint16_t>(buffer[consumed]) |
                    (static_cast<uint16_t>(buffer[consumed + 1]) << 8);

                if (packet_length > max_packet_size || packet_length == 0) {
                        logging::logger().println(
                            logging::LogLevel::Error, TAG,
                            "desync! garbage length=" +
                                std::to_string(packet_length));
                        consumed++;
                        continue;
                }

                if (buffer.size() - consumed < 2 + packet_length) {
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
                buffer.erase(buffer.begin(), buffer.begin() + consumed);
        }

        return result::ok();
}

} // namespace serial
