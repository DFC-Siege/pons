#include <unity.h>

#include "ble_hal_test.hpp"
#include "semaphore_test.hpp"

extern "C" void app_main() {
        UNITY_BEGIN();
        run_semaphore_tests();
        run_ble_hal_tests();
        UNITY_END();
}
