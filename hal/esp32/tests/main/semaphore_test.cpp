#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <unity.h>

#include "esp32_semaphore.hpp"
#include "semaphore_test.hpp"

static void test_take_returns_true_when_given() {
        async::Esp32Semaphore sem;
        sem.give();
        TEST_ASSERT_TRUE(sem.take(100));
}

static void test_take_returns_false_on_timeout() {
        async::Esp32Semaphore sem;
        TEST_ASSERT_FALSE(sem.take(10));
}

static void test_take_blocks_until_give() {
        async::Esp32Semaphore sem;
        bool done = false;
        xTaskCreate(
            [](void *arg) {
                    auto *ctx = static_cast<
                        std::pair<async::Esp32Semaphore *, bool *> *>(arg);
                    vTaskDelay(pdMS_TO_TICKS(10));
                    *ctx->second = true;
                    ctx->first->give();
                    vTaskDelete(nullptr);
            },
            "give_task", 2048,
            new std::pair<async::Esp32Semaphore *, bool *>(&sem, &done), 5,
            nullptr);
        TEST_ASSERT_TRUE(sem.take(1000));
        TEST_ASSERT_TRUE(done);
}

static void test_give_from_isr() {
        async::Esp32Semaphore sem;
        sem.give_from_isr();
        TEST_ASSERT_TRUE(sem.take(100));
}

void run_semaphore_tests() {
        RUN_TEST(test_take_returns_true_when_given);
        RUN_TEST(test_take_returns_false_on_timeout);
        RUN_TEST(test_take_blocks_until_give);
        RUN_TEST(test_give_from_isr);
}
