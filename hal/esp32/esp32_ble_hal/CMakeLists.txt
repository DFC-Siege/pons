idf_component_register(
    SRCS "ble_hal.cpp"
    INCLUDE_DIRS "."
    REQUIRES result logger bt ble_hal
)
