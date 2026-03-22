if(ESP_PLATFORM)
        idf_component_register(
                SRCS "console_logger.cpp"
                INCLUDE_DIRS "."
                REQUIRES logger
        )
else()
        add_library(console_logger console_logger.cpp)
        target_include_directories(console_logger PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(console_logger PUBLIC logger)
        target_compile_features(console_logger PUBLIC cxx_std_20)

        install(TARGETS console_logger EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )

        if(PLATFORM STREQUAL "posix")
                add_executable(console_logger_test console_logger_test.cpp)
                target_link_libraries(console_logger_test PRIVATE Catch2::Catch2WithMain console_logger)
                target_compile_features(console_logger_test PRIVATE cxx_std_20)
                catch_discover_tests(console_logger_test)
        endif()
endif()

