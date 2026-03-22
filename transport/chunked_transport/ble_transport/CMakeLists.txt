if(ESP_PLATFORM)
        idf_component_register(
                SRCS "ble_transporter.cpp"
                INCLUDE_DIRS "."
                REQUIRES result logger ble_hal chunked_transport transport
        )
else()
        add_library(ble_transport ble_transporter.cpp)
        target_include_directories(ble_transport PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(ble_transport PUBLIC result ble_hal chunked_transport transport logger)
        target_compile_features(ble_transport PUBLIC cxx_std_20)

        install(TARGETS ble_transport EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )

        if(PLATFORM STREQUAL "posix")
                add_executable(ble_transport_test ble_transport_test.cpp)
                target_link_libraries(ble_transport_test PRIVATE Catch2::Catch2WithMain ble_transport platform_async)
                target_compile_features(ble_transport_test PRIVATE cxx_std_20)
                catch_discover_tests(ble_transport_test)
        endif()
endif()
