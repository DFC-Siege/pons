if(ESP_PLATFORM)
        idf_component_register(
                INCLUDE_DIRS "."
                REQUIRES transport logger mutex semaphore platform dispatcher serializer
        )
else()
        add_library(requester INTERFACE)
        target_include_directories(requester INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(requester INTERFACE transport logger mutex semaphore platform dispatcher serializer)
        target_compile_features(requester INTERFACE cxx_std_20)
        install(TARGETS requester EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )
        if(PLATFORM STREQUAL "posix")
                add_executable(requester_test requester_test.cpp)
                target_link_libraries(requester_test PRIVATE Catch2::Catch2WithMain requester mutex semaphore platform dispatcher serializer)
                target_compile_features(requester_test PRIVATE cxx_std_20)
                catch_discover_tests(requester_test)

                add_executable(serialized_requester_test serialized_requester_test.cpp)
                target_link_libraries(serialized_requester_test PRIVATE Catch2::Catch2WithMain requester mutex semaphore platform dispatcher serializer)
                target_compile_features(serialized_requester_test PRIVATE cxx_std_20)
                catch_discover_tests(serialized_requester_test)
        endif()
endif()
