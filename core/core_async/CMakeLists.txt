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
    target_compile_features(core_async INTERFACE cxx_std_23)

    install(TARGETS core_async EXPORT pons-targets)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/pons
        FILES_MATCHING PATTERN "*.hpp"
    )
endif()
