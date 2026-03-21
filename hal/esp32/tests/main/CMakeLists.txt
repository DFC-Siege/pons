idf_component_register(
    SRCS
        "main.cpp"
        "semaphore_test.cpp"
        "ble_hal_test.cpp"
        "http_client_test.cpp"
    INCLUDE_DIRS "."
    REQUIRES unity esp32_async esp32_ble_hal esp32_http_client core_async result nvs_flash esp_wifi esp_netif esp_event
)
