idf_component_register(
    SRCS
        "main.cpp"
        "semaphore_test.cpp"
    INCLUDE_DIRS "."
    REQUIRES unity esp32_async core_async result
)
