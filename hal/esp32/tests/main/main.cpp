#include <unity.h>

#include "semaphore_test.hpp"

extern "C" void app_main() {
        UNITY_BEGIN();
        run_semaphore_tests();
        UNITY_END();
}
