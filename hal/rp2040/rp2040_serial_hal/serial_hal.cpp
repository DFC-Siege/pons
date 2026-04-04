#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <vector>

#include "result.hpp"
#include "serial_hal.hpp"

namespace serial {

SerialHal::SerialHal() {
        uart_init(uart1, BAUDRATE);
        gpio_set_function(TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(RX_PIN, GPIO_FUNC_UART);
        uart_set_hw_flow(uart1, false, false);
        uart_set_format(uart1, 8, 1, UART_PARITY_NONE);
        uart_set_fifo_enabled(uart1, true);
}

result::Result<bool> SerialHal::send(std::span<const uint8_t> data) {
        if (data.empty())
                return result::ok();
        uart_write_blocking(uart1, data.data(), data.size());
        return result::ok();
}

void SerialHal::on_receive(ReceiveCallback cb) {
        receive_callback = std::move(cb);
}

result::Result<bool> SerialHal::loop() {
        static std::vector<uint8_t> buffer;
        while (uart_is_readable(uart1)) {
                const auto c = uart_getc(uart1);
                buffer.push_back(static_cast<uint8_t>(c));
        }
        if (!buffer.empty() && receive_callback) {
                receive_callback(std::move(buffer));
                buffer.clear();
        }
        return result::ok();
}

} // namespace serial
