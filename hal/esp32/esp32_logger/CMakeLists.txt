idf_component_register(
    SRCS "esp32_logger.cpp"
    INCLUDE_DIRS "."
    REQUIRES logger log esp_system
)
