if(ESP_PLATFORM)
        idf_component_register(
                INCLUDE_DIRS "."
                REQUIRES result logger transporter transport
        )
else()
        add_library(direct_transporter INTERFACE)
        target_include_directories(direct_transporter INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(direct_transporter INTERFACE result transporter logger transport)
        target_compile_features(direct_transporter INTERFACE cxx_std_20)
        install(TARGETS direct_transporter EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )
        if(PLATFORM STREQUAL "posix")
                add_executable(direct_transporter_test direct_transporter_test.cpp)
                target_link_libraries(direct_transporter_test PRIVATE Catch2::Catch2WithMain direct_transporter transporter mutex platform)
                target_compile_features(direct_transporter_test PRIVATE cxx_std_20)
                catch_discover_tests(direct_transporter_test)
        endif()
endif()
