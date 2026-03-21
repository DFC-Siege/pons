idf_component_register(
    SRCS
        "main.cpp"
        "semaphore_test.cpp"
        "ble_hal_test.cpp"
    INCLUDE_DIRS "."
    REQUIRES unity esp32_async esp32_ble_hal core_async result
)
