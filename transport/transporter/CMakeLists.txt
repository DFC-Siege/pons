if(ESP_PLATFORM)
        idf_component_register(
                SRCS "base_transporter.cpp"
                INCLUDE_DIRS "."
                REQUIRES transport logger
        )
else()
        add_library(transporter base_transporter.cpp)
        target_include_directories(transporter PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(transporter PUBLIC transport logger)
        target_compile_features(transporter PUBLIC cxx_std_20)
        install(TARGETS transporter EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )
        if(PLATFORM STREQUAL "posix")
                add_executable(multiplexer_test multiplexer_test.cpp)
                target_link_libraries(multiplexer_test PRIVATE Catch2::Catch2WithMain transporter platform_async)
                target_compile_features(multiplexer_test PRIVATE cxx_std_20)
                catch_discover_tests(multiplexer_test)
        endif()
endif()
