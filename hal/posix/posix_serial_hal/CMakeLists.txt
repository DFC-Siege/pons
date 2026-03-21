cmake_minimum_required(VERSION 3.16)

add_library(posix_serial_hal serial_hal.cpp)
target_include_directories(posix_serial_hal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/pons>
)
target_link_libraries(posix_serial_hal PUBLIC serial_hal result)
target_compile_features(posix_serial_hal PUBLIC cxx_std_23)

install(TARGETS posix_serial_hal EXPORT pons-targets)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/pons
    FILES_MATCHING PATTERN "*.hpp"
)
