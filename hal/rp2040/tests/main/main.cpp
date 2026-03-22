#include <cstdio>
#include <pico/stdlib.h>

#include "semaphore_test.hpp"
#include "serial_hal_test.hpp"

int main() {
        stdio_init_all();
        sleep_ms(2000);
        printf("=== RP2040 HAL Tests ===\n");
        run_semaphore_tests();
        run_serial_hal_tests();
        printf("=== Done ===\n");
        return 0;
}
