#include <nvs_flash.h>
#include <unity.h>

#include "ble_hal_test.hpp"
#include "http_client_test.hpp"
#include "logger_test.hpp"
#include "nvs_store_test.hpp"
#include "semaphore_test.hpp"
#include "serial_hal_test.hpp"

extern "C" void app_main() {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
            ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                nvs_flash_erase();
                nvs_flash_init();
        }
        UNITY_BEGIN();
        run_nvs_store_tests();
        run_semaphore_tests();
        run_ble_hal_tests();
        run_http_client_tests();
        run_logger_tests();
        run_serial_hal_tests();
        UNITY_END();
}
