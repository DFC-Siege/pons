if(ESP_PLATFORM)
        idf_component_register(
                COMPONENT_NAME core_async
                INCLUDE_DIRS "."
        )
else()
        add_library(core_async INTERFACE)
        target_include_directories(core_async INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_compile_features(core_async INTERFACE cxx_std_20)

        install(TARGETS core_async EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )

        if(PLATFORM STREQUAL "posix")
                add_executable(core_async_test core_async_test.cpp)
                target_link_libraries(core_async_test PRIVATE Catch2::Catch2WithMain core_async)
                target_compile_features(core_async_test PRIVATE cxx_std_20)
                catch_discover_tests(core_async_test)
        endif()
endif()

