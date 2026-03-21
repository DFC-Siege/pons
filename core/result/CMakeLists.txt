if(ESP_PLATFORM)
    idf_component_register(
        INCLUDE_DIRS "."
    )
else()
    add_library(result INTERFACE)
    target_include_directories(result INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/pons>
    )
    target_compile_features(result INTERFACE cxx_std_23)

    install(TARGETS result EXPORT pons-targets)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/pons
        FILES_MATCHING PATTERN "*.hpp"
    )
endif()

if(NOT ESP_PLATFORM)
    add_executable(result_test result_test.cpp)
    target_link_libraries(result_test PRIVATE Catch2::Catch2WithMain result)
    target_compile_features(result_test PRIVATE cxx_std_23)
    catch_discover_tests(result_test)
endif()
