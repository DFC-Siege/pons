#include <cstdio>
#include <hardware/uart.h>
#include <pico/stdlib.h>
#include <vector>

#include "serial_hal.hpp"
#include "serial_hal_test.hpp"

#define TEST_ASSERT(cond)                                                      \
        do {                                                                   \
                if (!(cond)) {                                                 \
                        printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__,        \
                               #cond);                                         \
                        failed++;                                              \
                } else {                                                       \
                        printf("PASS: %s\n", #cond);                           \
                        passed++;                                              \
                }                                                              \
        } while (0)

static int passed = 0;
static int failed = 0;
static serial::SerialHal *hal = nullptr;

static void test_constructor_does_not_crash() {
        hal = new serial::SerialHal(uart1, 4, 5, 115200);
        TEST_ASSERT(hal != nullptr);
}

static void test_send_returns_ok() {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        const auto result = hal->send(std::move(data));
        TEST_ASSERT(!result.failed());
}

static void test_send_empty_returns_ok() {
        const auto result = hal->send({});
        TEST_ASSERT(!result.failed());
}

static void test_loop_returns_ok_when_no_data() {
        const auto result = hal->loop();
        TEST_ASSERT(!result.failed());
}

static void test_on_receive_callback_fires() {
        bool called = false;
        hal->on_receive([&](std::span<const uint8_t>) { called = true; });

        const std::vector<uint8_t> data = {0x41, 0x42, 0x43};
        uart_write_blocking(uart1, data.data(), data.size());
        sleep_ms(10);

        hal->loop();
        TEST_ASSERT(called);
        hal->on_receive(nullptr);
}

void run_serial_hal_tests() {
        printf("--- Serial HAL Tests ---\n");
        test_constructor_does_not_crash();
        test_send_returns_ok();
        test_send_empty_returns_ok();
        test_loop_returns_ok_when_no_data();
        test_on_receive_callback_fires();
        printf("--- %d passed, %d failed ---\n", passed, failed);
}
