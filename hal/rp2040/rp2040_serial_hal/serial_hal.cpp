#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <vector>

#include "result.hpp"
#include "serial_hal.hpp"

namespace serial {

SerialHal::SerialHal(uart_inst_t *uart, Pin tx_pin, Pin rx_pin,
                     Baudrate baudrate)
    : uart(uart), baudrate(baudrate), tx_pin(tx_pin), rx_pin(rx_pin) {
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
        uart_write_blocking(uart1, prefix, sizeof(prefix));
        uart_write_blocking(uart1, data.data(), data.size());
        return result::ok();
}

void SerialHal::on_receive(ReceiveCallback cb) {
        receive_callback = std::move(cb);
}

result::Try SerialHal::loop() {
        while (uart_is_readable(uart1)) {
                const auto c = uart_getc(uart1);
                buffer.push_back(static_cast<uint8_t>(c));
        }
        while (buffer.size() >= 2) {
                const uint16_t packet_length =
                    static_cast<uint16_t>(buffer[0]) |
                    (static_cast<uint16_t>(buffer[1]) << 8);
                if (buffer.size() < 2 + packet_length)
                        break;
                Data packet(buffer.begin() + 2,
                            buffer.begin() + 2 + packet_length);
                buffer.erase(buffer.begin(),
                             buffer.begin() + 2 + packet_length);
                if (receive_callback)
                        receive_callback(std::move(packet));
        }
        return result::ok();
}

} // namespace serial
