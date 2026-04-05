if(ESP_PLATFORM)
        idf_component_register(
                SRCS "serial_transporter.cpp"
                INCLUDE_DIRS "."
                REQUIRES result logger serial_hal transporter transport
        )
else()
        add_library(serial_transporter serial_transporter.cpp)
        target_include_directories(serial_transporter PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(serial_transporter PUBLIC result serial_hal transporter logger transport)
        target_compile_features(serial_transporter PUBLIC cxx_std_20)
        install(TARGETS serial_transporter EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )
        if(PLATFORM STREQUAL "posix")
                add_executable(serial_transporter_test serial_transporter_test.cpp)
                target_link_libraries(serial_transporter_test PRIVATE Catch2::Catch2WithMain serial_transporter)
                target_compile_features(serial_transporter_test PRIVATE cxx_std_20)
                catch_discover_tests(serial_transporter_test)
        endif()
endif()
