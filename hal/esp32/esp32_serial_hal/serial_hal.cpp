#include <algorithm>
#include <driver/uart.h>
#include <hal/uart_types.h>
#include <vector>

#include "result.hpp"
#include "serial_hal.hpp"

namespace serial {
SerialHal::SerialHal() {
        uart_config_t uart_config = {.baud_rate = BAUDRATE,
                                     .data_bits = UART_DATA_8_BITS,
                                     .parity = UART_PARITY_DISABLE,
                                     .stop_bits = UART_STOP_BITS_1,
                                     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                                     .rx_flow_ctrl_thresh =
                                         UART_HW_FLOWCTRL_DISABLE,
                                     .source_clk = UART_SCLK_DEFAULT,
                                     .flags = {}};

        uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
        uart_param_config(UART_NUM_1, &uart_config);
        uart_set_pin(UART_NUM_1, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE,
                     UART_PIN_NO_CHANGE);
}

result::Result<bool> SerialHal::send(std::span<const uint8_t> data) {
        if (data.empty()) {
                return result::ok();
        }

        const auto response =
            uart_write_bytes(UART_NUM_1, data.data(), data.size());
        if (response < 0) {
                return result::err(
                    "something went wrong while sending over serial");
        }

        return result::ok();
}

result::Result<std::vector<uint8_t>> SerialHal::read(uint32_t timeout) {
        std::vector<uint8_t> data(BUF_SIZE);
        int length =
            uart_read_bytes(UART_NUM_1, data.data(), BUF_SIZE, timeout);
        if (length < 0) {
                return result::err(
                    "something went wrong while reading over serial");
        }

        if (length == 0) {
                return result::err("empty response");
        }

        data.resize(length);
        return result::ok(data);
}
} // namespace serial
