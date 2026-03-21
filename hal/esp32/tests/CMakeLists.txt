cmake_minimum_required(VERSION 3.16)

set(PONS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

set(EXTRA_COMPONENT_DIRS
    ${PONS_DIR}/core/logger
    ${PONS_DIR}/core/logger/console_logger
    ${PONS_DIR}/core/core_async
    ${PONS_DIR}/core/result
    ${PONS_DIR}/hal/esp32/esp32_async
    ${PONS_DIR}/hal/ble_hal
    ${PONS_DIR}/hal/esp32/esp32_ble_hal
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_hal_tests)
