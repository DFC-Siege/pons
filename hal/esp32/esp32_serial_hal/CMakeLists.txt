idf_component_register(
    SRCS "esp32_serial_hal.cpp"
    INCLUDE_DIRS "."
    REQUIRES result logger driver serial_hal esp_driver_uart
)
