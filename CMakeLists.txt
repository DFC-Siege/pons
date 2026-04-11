cmake_minimum_required(VERSION 3.16)
set(PLATFORM "posix" CACHE STRING "Target platform (esp32, stm32, posix, rp2040)")
if(ESP_PLATFORM)
        set(EXTRA_COMPONENT_DIRS
                ${CMAKE_CURRENT_SOURCE_DIR}/core/result
                ${CMAKE_CURRENT_SOURCE_DIR}/core/logger
                ${CMAKE_CURRENT_SOURCE_DIR}/core/logger/console_logger
                ${CMAKE_CURRENT_SOURCE_DIR}/core/serializer
                ${CMAKE_CURRENT_SOURCE_DIR}/core/mutex
                ${CMAKE_CURRENT_SOURCE_DIR}/platform
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/ble_hal
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/serial_hal
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/http_client
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/store
                ${CMAKE_CURRENT_SOURCE_DIR}/transport
                ${CMAKE_CURRENT_SOURCE_DIR}/transport/dispatcher
                ${CMAKE_CURRENT_SOURCE_DIR}/transport/requester
                ${CMAKE_CURRENT_SOURCE_DIR}/transport/transporter
                ${CMAKE_CURRENT_SOURCE_DIR}/transport/transporter/chunked_transporter
                ${CMAKE_CURRENT_SOURCE_DIR}/transport/transporter/serial_transporter
                ${CMAKE_CURRENT_SOURCE_DIR}/transport/transporter/direct_transporter
        )
        include($ENV{IDF_PATH}/tools/cmake/project.cmake)
        project(pons)
elseif(PLATFORM STREQUAL "rp2040")
        include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
        project(pons C CXX ASM)
        set(CMAKE_CXX_STANDARD 20)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
        set(CMAKE_CXX_EXTENSIONS OFF)
        pico_sdk_init()
        add_compile_options(-Wno-free-nonheap-object)
        add_compile_definitions(PICO_BUILD)
        add_subdirectory(core/result)
        add_subdirectory(core/logger)
        add_subdirectory(core/serializer)
        add_subdirectory(core/mutex)
        add_subdirectory(platform)
        add_subdirectory(hal/ble_hal)
        add_subdirectory(hal/serial_hal)
        add_subdirectory(hal/store)
        add_subdirectory(hal/rp2040/rp2040_serial_hal)
        add_subdirectory(transport)
        add_subdirectory(transport/dispatcher)
        add_subdirectory(transport/requester)
        add_subdirectory(transport/transporter)
        add_subdirectory(transport/transporter/chunked_transporter)
        add_subdirectory(transport/transporter/serial_transporter)
        add_subdirectory(transport/transporter/direct_transporter)
else()
        project(pons CXX)
        set(CMAKE_CXX_STANDARD 23)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
        include(FetchContent)
        FetchContent_Declare(
                Catch2
                GIT_REPOSITORY https://github.com/catchorg/Catch2.git
                GIT_TAG v3.5.0
        )
        FetchContent_MakeAvailable(Catch2)
        include(CTest)
        include(Catch)
        enable_testing()
        set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Install prefix")
        add_subdirectory(core/result)
        add_subdirectory(core/logger)
        add_subdirectory(core/logger/console_logger)
        add_subdirectory(core/serializer)
        add_subdirectory(core/mutex)
        add_subdirectory(platform)
        add_subdirectory(hal/ble_hal)
        add_subdirectory(hal/serial_hal)
        add_subdirectory(hal/http_client)
        add_subdirectory(hal/store)
        add_subdirectory(hal/posix/posix_serial_hal)
        add_subdirectory(transport)
        add_subdirectory(transport/dispatcher)
        add_subdirectory(transport/requester)
        add_subdirectory(transport/transporter)
        add_subdirectory(transport/transporter/chunked_transporter)
        add_subdirectory(transport/transporter/serial_transporter)
        add_subdirectory(transport/transporter/direct_transporter)
        install(EXPORT pons-targets
                FILE pons-targets.cmake
                NAMESPACE pons::
                DESTINATION lib/cmake/pons
        )
        include(CMakePackageConfigHelpers)
        configure_package_config_file(
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/pons-config.cmake.in
                ${CMAKE_CURRENT_BINARY_DIR}/pons-config.cmake
                INSTALL_DESTINATION lib/cmake/pons
        )
        write_basic_package_version_file(
                ${CMAKE_CURRENT_BINARY_DIR}/pons-config-version.cmake
                VERSION 0.1.0
                COMPATIBILITY SameMajorVersion
        )
        install(FILES
                ${CMAKE_CURRENT_BINARY_DIR}/pons-config.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/pons-config-version.cmake
                DESTINATION lib/cmake/pons
        )
endif()
