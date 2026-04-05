idf_component_register(
    SRCS
        "main.cpp"
        "ble_hal_test.cpp"
        "http_client_test.cpp"
        "logger_test.cpp"
        "serial_hal_test.cpp"
        "nvs_store_test.cpp"
    INCLUDE_DIRS "."
    REQUIRES unity esp32_ble_hal esp32_http_client result nvs_flash esp_wifi esp_netif esp_event esp32_logger logger log esp_system driver serial_hal esp32_serial_hal nvs_store
)
