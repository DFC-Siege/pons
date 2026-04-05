cmake_minimum_required(VERSION 3.16)

set(PONS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(rp2040_hal_tests C CXX ASM)
set(CMAKE_CXX_STANDARD 20)
pico_sdk_init()

add_subdirectory(${PONS_DIR}/core/result ${CMAKE_BINARY_DIR}/result)
add_subdirectory(${PONS_DIR}/hal/serial_hal ${CMAKE_BINARY_DIR}/serial_hal)
add_subdirectory(${PONS_DIR}/hal/rp2040/rp2040_serial_hal ${CMAKE_BINARY_DIR}/rp2040_serial_hal)

add_subdirectory(main)
