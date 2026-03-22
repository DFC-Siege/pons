add_library(rp2040_serial_hal serial_hal.cpp)
target_include_directories(rp2040_serial_hal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/pons>
)
target_link_libraries(rp2040_serial_hal PUBLIC serial_hal result hardware_uart hardware_gpio)
target_compile_features(rp2040_serial_hal PUBLIC cxx_std_20)

install(TARGETS rp2040_serial_hal EXPORT pons-targets)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/pons
    FILES_MATCHING PATTERN "*.hpp"
)
