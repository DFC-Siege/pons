if(ESP_PLATFORM)
        idf_component_register(
                SRCS
                "chunked_receiver.cpp"
                "chunked_sender.cpp"
                "chunked_transporter.cpp"
                INCLUDE_DIRS "."
                REQUIRES result logger transport platform_async core_async
        )
else()
        add_library(chunked_transport chunked_receiver.cpp chunked_sender.cpp chunked_transporter.cpp)
        target_include_directories(chunked_transport PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(chunked_transport PUBLIC transport result platform_async core_async logger)
        target_compile_features(chunked_transport PUBLIC cxx_std_23)

        install(TARGETS chunked_transport EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )

        if(PLATFORM STREQUAL "posix")
                add_executable(chunked_transport_test
                        packet_test.cpp
                        chunked_receiver_test.cpp
                        chunked_sender_test.cpp
                        chunked_transporter_test.cpp
                )
                target_link_libraries(chunked_transport_test PRIVATE Catch2::Catch2WithMain chunked_transport platform_async)
                target_compile_features(chunked_transport_test PRIVATE cxx_std_23)
                catch_discover_tests(chunked_transport_test)
        endif()
endif()
