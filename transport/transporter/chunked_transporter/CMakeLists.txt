if(ESP_PLATFORM)
        idf_component_register(
                INCLUDE_DIRS "."
                REQUIRES result logger transporter transport mutex platform
        )
else()
        add_library(chunked_transporter INTERFACE)
        target_include_directories(chunked_transporter INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(chunked_transporter INTERFACE result transporter logger transport mutex platform)
        target_compile_features(chunked_transporter INTERFACE cxx_std_20)
        install(TARGETS chunked_transporter EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )
        if(PLATFORM STREQUAL "posix")
                add_executable(chunked_transporter_test chunked_transporter_test.cpp packet_test.cpp)
                target_link_libraries(chunked_transporter_test PRIVATE Catch2::Catch2WithMain chunked_transporter mutex platform)
                target_compile_features(chunked_transporter_test PRIVATE cxx_std_20)
                catch_discover_tests(chunked_transporter_test)
        endif()
endif()
