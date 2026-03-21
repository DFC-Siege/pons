#include "serial_hal_test.hpp"
#include "serial_hal.hpp"
#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hal/uart_types.h>
#include <unity.h>

static serial::SerialHal *hal = nullptr;

static void test_constructor_does_not_crash() {
        hal = new serial::SerialHal();
        TEST_ASSERT_TRUE(true);
}

static void test_send_returns_ok() {
        const std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        const auto result = hal->send(data);
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
