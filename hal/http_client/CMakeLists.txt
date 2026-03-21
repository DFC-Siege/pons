if(ESP_PLATFORM)
    idf_component_register(
        INCLUDE_DIRS "."
    )
else()
    add_library(http_client INTERFACE)
    target_include_directories(http_client INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/pons>
    )
    target_compile_features(http_client INTERFACE cxx_std_23)

    install(TARGETS http_client EXPORT pons-targets)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/pons
        FILES_MATCHING PATTERN "*.hpp"
    )
endif()
