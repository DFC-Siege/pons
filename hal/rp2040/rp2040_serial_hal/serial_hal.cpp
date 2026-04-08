#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <string>
#include <vector>

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

SerialHal::SerialHal(uart_inst_t *uart, Pin tx_pin, Pin rx_pin,
                     Baudrate baudrate, uint16_t max_packet_size)
    : uart(uart), baudrate(baudrate), tx_pin(tx_pin), rx_pin(rx_pin),
      max_packet_size(max_packet_size) {
        uart_init(this->uart, this->baudrate);
        gpio_set_function(this->tx_pin, GPIO_FUNC_UART);
        gpio_set_function(this->rx_pin, GPIO_FUNC_UART);

        gpio_pull_up(this->rx_pin);

        uart_set_hw_flow(this->uart, false, false);
        uart_set_format(this->uart, 8, 1, UART_PARITY_NONE);
        uart_set_fifo_enabled(this->uart, true);

        logging::logger().println(
            logging::LogLevel::Debug, TAG,
            "initialized uart=" + std::to_string(uart_get_index(uart)) +
                " tx=" + std::to_string(tx_pin) + " rx=" +
                std::to_string(rx_pin) + " baud=" + std::to_string(baudrate));
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

        uart_write_blocking(uart, prefix, sizeof(prefix));
        uart_write_blocking(uart, data.data(), data.size());

        logging::logger().println(logging::LogLevel::Debug, TAG,
                                  "send complete");
        return result::ok();
}

void SerialHal::on_receive(ReceiveCallback cb) {
        logging::logger().println(logging::LogLevel::Debug, TAG,
                                  "receive callback registered");
        receive_callback = std::move(cb);
}

result::Try SerialHal::loop() {
        bool read_any = false;
        while (uart_is_readable(uart)) {
                buffer.push_back(static_cast<uint8_t>(uart_getc(uart)));
                read_any = true;
        }

        if (read_any) {
                logging::logger().println(
                    logging::LogLevel::Debug, TAG,
                    "read bytes from hardware, buffer size=" +
                        std::to_string(buffer.size()));
        }

        while (buffer.size() >= 2) {
                const uint16_t packet_length =
                    static_cast<uint16_t>(buffer[0]) |
                    (static_cast<uint16_t>(buffer[1]) << 8);

                if (packet_length > max_packet_size || packet_length == 0) {
                        logging::logger().println(
                            logging::LogLevel::Error, TAG,
                            "desync detected! garbage length=" +
                                std::to_string(packet_length));
                        buffer.erase(buffer.begin());
                        continue;
                }

                if (buffer.size() < 2 + packet_length) {
                        break;
                }

                logging::logger().println(logging::LogLevel::Debug, TAG,
                                          "dispatching packet length=" +
                                              std::to_string(packet_length));

                Data packet(buffer.begin() + 2,
                            buffer.begin() + 2 + packet_length);

                buffer.erase(buffer.begin(),
                             buffer.begin() + 2 + packet_length);

                if (receive_callback) {
                        receive_callback(std::move(packet));
                }
        }
        return result::ok();
}

} // namespace serial
