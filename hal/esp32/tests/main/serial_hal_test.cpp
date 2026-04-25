#include "serial_hal_test.hpp"
#include "esp32_serial_hal.hpp"
#include <driver/uart.h>
#include <hal/uart_types.h>
#include <unity.h>

static serial::Esp32SerialHal *hal = nullptr;

static void test_constructor_does_not_crash() {
        static constexpr auto BAUDRATE = 115200;
        static constexpr auto BUFFER_SIZE = 1024;
        static constexpr auto UART = UART_NUM_1;
        static constexpr auto TX_PIN = 7;
        static constexpr auto RX_PIN = 6;
        hal = new serial::Esp32SerialHal(UART, TX_PIN, RX_PIN, BAUDRATE,
                                         BUFFER_SIZE);
        TEST_ASSERT_TRUE(true);
}

static void test_send_returns_ok() {
        const auto result = hal->send({0x01, 0x02, 0x03});
        TEST_ASSERT_FALSE(result.failed());
}

static void test_send_empty_data_returns_ok() {
        const auto result = hal->send({});
        TEST_ASSERT_FALSE(result.failed());
}

static void test_loop_returns_ok_when_no_data() {
        const auto result = hal->loop();
        TEST_ASSERT_FALSE(result.failed());
}

void run_serial_hal_tests() {
        RUN_TEST(test_constructor_does_not_crash);
        RUN_TEST(test_send_returns_ok);
        RUN_TEST(test_send_empty_data_returns_ok);
        RUN_TEST(test_loop_returns_ok_when_no_data);
}
