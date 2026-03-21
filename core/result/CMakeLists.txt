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
