#include <unity.h>

#include "ble_hal_test.hpp"
#include "http_client_test.hpp"
#include "semaphore_test.hpp"

extern "C" void app_main() {
        UNITY_BEGIN();
        run_semaphore_tests();
        run_ble_hal_tests();
        run_http_client_tests();
        UNITY_END();
}
