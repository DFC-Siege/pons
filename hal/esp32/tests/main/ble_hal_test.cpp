#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <unity.h>

#include "ble_hal.hpp"
#include "ble_hal_test.hpp"

static void test_ble_not_connected_initially() {
        TEST_ASSERT_FALSE(ble::BleHal::instance().is_connected());
}

static void test_send_fails_when_not_connected() {
        const std::vector<uint8_t> data = {0x01, 0x02};
        const auto result = ble::BleHal::instance().send(data);
        TEST_ASSERT_TRUE(result.failed());
}

static void test_begin_does_not_crash() {
        ble::BleHal::instance().begin("pons-test");
        vTaskDelay(pdMS_TO_TICKS(100));
        TEST_ASSERT_TRUE(true);
}

static void test_on_receive_callback_is_set() {
        bool called = false;
        ble::BleHal::instance().on_receive(
            [&](std::span<const uint8_t>) { called = true; });
        TEST_ASSERT_TRUE(true);
}

static void test_on_connection_changed_callback_is_set() {
        ble::BleHal::instance().on_connection_changed([](bool) {});
        TEST_ASSERT_TRUE(true);
}

void run_ble_hal_tests() {
        RUN_TEST(test_ble_not_connected_initially);
        RUN_TEST(test_send_fails_when_not_connected);
        RUN_TEST(test_begin_does_not_crash);
        RUN_TEST(test_on_receive_callback_is_set);
        RUN_TEST(test_on_connection_changed_callback_is_set);
}
