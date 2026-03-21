idf_component_register(
    SRCS "serial_hal.cpp"
    INCLUDE_DIRS "."
    REQUIRES result logger driver serial_hal
)
