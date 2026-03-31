cmake_minimum_required(VERSION 3.16)

set(PLATFORM "posix" CACHE STRING "Target platform (esp32, stm32, posix, rp2040)")

if(ESP_PLATFORM)
        set(EXTRA_COMPONENT_DIRS
                ${CMAKE_CURRENT_SOURCE_DIR}/core/result
                ${CMAKE_CURRENT_SOURCE_DIR}/core/core_async
                ${CMAKE_CURRENT_SOURCE_DIR}/core/logger
                ${CMAKE_CURRENT_SOURCE_DIR}/core/logger/console_logger
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/esp32/esp32_async
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/ble_hal
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/serial_hal
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/http_client
                ${CMAKE_CURRENT_SOURCE_DIR}/platform/${PLATFORM}/platform_async
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/store
                ${CMAKE_CURRENT_SOURCE_DIR}/transport
                ${CMAKE_CURRENT_SOURCE_DIR}/transport/chunked_transport
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

        add_subdirectory(core/result)
        add_subdirectory(core/core_async)
        add_subdirectory(core/logger)
        add_subdirectory(hal/ble_hal)
        add_subdirectory(hal/serial_hal)
        add_subdirectory(hal/store)
        add_subdirectory(hal/rp2040/rp2040_async)
        add_subdirectory(hal/rp2040/rp2040_serial_hal)
        add_subdirectory(platform/rp2040/platform_async)
        add_subdirectory(transport)
        add_subdirectory(transport/chunked_transport)
        add_subdirectory(transport/chunked_transport/serial_transport)
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
        add_subdirectory(core/core_async)
        add_subdirectory(core/logger)
        add_subdirectory(core/logger/console_logger)
        add_subdirectory(hal/ble_hal)
        add_subdirectory(hal/serial_hal)
        add_subdirectory(hal/http_client)
        add_subdirectory(hal/store)
        add_subdirectory(hal/${PLATFORM}/${PLATFORM}_async)
        add_subdirectory(hal/${PLATFORM}/${PLATFORM}_serial_hal)
        add_subdirectory(platform/${PLATFORM}/platform_async)
        add_subdirectory(transport)
        add_subdirectory(transport/chunked_transport)
        add_subdirectory(transport/chunked_transport/serial_transport)
        add_subdirectory(transport/chunked_transport/ble_transport)

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
