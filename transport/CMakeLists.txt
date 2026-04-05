if(ESP_PLATFORM)
        idf_component_register(
                INCLUDE_DIRS "."
                REQUIRES result transporter
        )
else()
        add_library(transport INTERFACE)
        target_include_directories(transport INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(transport INTERFACE result transporter)
        target_compile_features(transport INTERFACE cxx_std_20)
        install(TARGETS transport EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )
endif()
