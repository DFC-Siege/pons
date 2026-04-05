#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <vector>

#include "result.hpp"
#include "serial_hal.hpp"

namespace serial {

SerialHal::SerialHal(uart_inst_t *uart, Pin tx_pin, Pin rx_pin,
                     Baudrate baudrate)
    : uart(uart), baudrate(baudrate), tx_pin(tx_pin), rx_pin(rx_pin), tmp(256) {
        uart_init(this->uart, this->baudrate);
        gpio_set_function(this->tx_pin, GPIO_FUNC_UART);
        gpio_set_function(this->rx_pin, GPIO_FUNC_UART);
        uart_set_hw_flow(this->uart, false, false);
        uart_set_format(this->uart, 8, 1, UART_PARITY_NONE);
        uart_set_fifo_enabled(this->uart, true);
}

result::Try SerialHal::send(Data &&data) {
        if (data.empty())
                return result::ok();
        const uint16_t length = static_cast<uint16_t>(data.size());
        const uint8_t prefix[2] = {static_cast<uint8_t>(length & 0xFF),
                                   static_cast<uint8_t>((length >> 8) & 0xFF)};
        uart_write_blocking(uart, prefix, sizeof(prefix));
        uart_write_blocking(uart, data.data(), data.size());
        return result::ok();
}

void SerialHal::on_receive(ReceiveCallback cb) {
        receive_callback = std::move(cb);
}

result::Try SerialHal::loop() {
        size_t len = 0;
        while (uart_is_readable(uart) && len < tmp.size()) {
                tmp[len++] = uart_getc(uart);
        }
        if (len > 0) {
                buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + len);
        }

        size_t consumed = 0;
        while (buffer.size() - consumed >= 2) {
                const uint16_t packet_length =
                    static_cast<uint16_t>(buffer[consumed]) |
                    (static_cast<uint16_t>(buffer[consumed + 1]) << 8);
                if (buffer.size() - consumed < 2 + packet_length)
                        break;
                Data packet(buffer.begin() + consumed + 2,
                            buffer.begin() + consumed + 2 + packet_length);
                consumed += 2 + packet_length;
                if (receive_callback)
                        receive_callback(std::move(packet));
        }
        if (consumed > 0)
                buffer.erase(buffer.begin(), buffer.begin() + consumed);

        return result::ok();
}

} // namespace serial
