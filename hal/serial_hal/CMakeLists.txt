if(ESP_PLATFORM)
    idf_component_register(
        INCLUDE_DIRS "."
        REQUIRES result
    )
else()
    add_library(serial_hal INTERFACE)
    target_include_directories(serial_hal INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/pons>
    )
    target_link_libraries(serial_hal INTERFACE result)
    target_compile_features(serial_hal INTERFACE cxx_std_23)

    install(TARGETS serial_hal EXPORT pons-targets)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/pons
        FILES_MATCHING PATTERN "*.hpp"
    )
endif()
