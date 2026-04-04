if(ESP_PLATFORM)
        idf_component_register(
                SRCS "serial_transporter.cpp"
                INCLUDE_DIRS "."
                REQUIRES result logger serial_hal chunked_transport transport
        )
else()
        add_library(serial_transport serial_transporter.cpp)
        target_include_directories(serial_transport PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(serial_transport PUBLIC result serial_hal chunked_transport transport logger)
        target_compile_features(serial_transport PUBLIC cxx_std_20)

        install(TARGETS serial_transport EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )

        if(PLATFORM STREQUAL "posix")
                add_executable(serial_transport_test serial_transport_test.cpp)
                target_link_libraries(serial_transport_test PRIVATE Catch2::Catch2WithMain serial_transport platform_async)
                target_compile_features(serial_transport_test PRIVATE cxx_std_20)
                catch_discover_tests(serial_transport_test)
        endif()
endif()
