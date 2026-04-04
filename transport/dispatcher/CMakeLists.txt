if(ESP_PLATFORM)
        idf_component_register(
                INCLUDE_DIRS "."
                REQUIRES transport logger
        )
else()
        add_library(dispatcher INTERFACE)
        target_include_directories(dispatcher INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(dispatcher INTERFACE transport logger)
        target_compile_features(dispatcher INTERFACE cxx_std_20)
        install(TARGETS dispatcher EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )
        if(PLATFORM STREQUAL "posix")
                add_executable(dispatcher_test dispatcher_test.cpp)
                target_link_libraries(dispatcher_test PRIVATE Catch2::Catch2WithMain dispatcher platform_async)
                target_compile_features(dispatcher_test PRIVATE cxx_std_20)
                catch_discover_tests(dispatcher_test)
        endif()
endif()
