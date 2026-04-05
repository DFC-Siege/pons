add_executable(rp2040_hal_tests
        main.cpp
        serial_hal_test.cpp
)

target_link_libraries(rp2040_hal_tests PRIVATE
        rp2040_serial_hal
        result
        serial_hal
        pico_stdlib
        pico_sync
        pico_multicore
)

target_compile_features(rp2040_hal_tests PRIVATE cxx_std_20)
pico_enable_stdio_usb(rp2040_hal_tests 1)
pico_enable_stdio_uart(rp2040_hal_tests 0)
pico_add_extra_outputs(rp2040_hal_tests)
