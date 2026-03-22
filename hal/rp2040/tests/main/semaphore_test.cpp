#include <cstdio>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "rp2040_semaphore.hpp"
#include "semaphore_test.hpp"

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

static async::RP2040Semaphore *shared_sem = nullptr;

static void core1_give_task() {
        sleep_ms(10);
        shared_sem->give();
}

static void test_take_returns_true_when_given() {
        async::RP2040Semaphore sem;
        sem.give();
        TEST_ASSERT(sem.take(100));
}

static void test_take_returns_false_on_timeout() {
        async::RP2040Semaphore sem;
        TEST_ASSERT(!sem.take(10));
}

static void test_take_blocks_until_give() {
        async::RP2040Semaphore sem;
        shared_sem = &sem;
        multicore_launch_core1(core1_give_task);
        TEST_ASSERT(sem.take(1000));
        multicore_reset_core1();
}

static void test_give_from_isr() {
        async::RP2040Semaphore sem;
        sem.give_from_isr();
        TEST_ASSERT(sem.take(100));
}

void run_semaphore_tests() {
        printf("--- Semaphore Tests ---\n");
        test_take_returns_true_when_given();
        test_take_returns_false_on_timeout();
        test_take_blocks_until_give();
        test_give_from_isr();
        printf("--- %d passed, %d failed ---\n", passed, failed);
}
