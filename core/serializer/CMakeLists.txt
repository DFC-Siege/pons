if(ESP_PLATFORM)
    idf_component_register(
        INCLUDE_DIRS "."
        REQUIRES result
    )
else()
    add_library(serializer INTERFACE)
    target_include_directories(serializer INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/pons>
    )
    target_link_libraries(serializer INTERFACE result)
    target_compile_features(serializer INTERFACE cxx_std_20)
    install(TARGETS serializer EXPORT pons-targets)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/pons
        FILES_MATCHING PATTERN "*.hpp"
    )
    if(PLATFORM STREQUAL "posix")
        add_executable(serializer_test serializer_test.cpp)
        target_link_libraries(serializer_test PRIVATE Catch2::Catch2WithMain serializer)
        target_compile_features(serializer_test PRIVATE cxx_std_23)
        catch_discover_tests(serializer_test)
    endif()
endif()
