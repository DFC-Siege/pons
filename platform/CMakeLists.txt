if(ESP_PLATFORM)
        idf_component_register(
                INCLUDE_DIRS "."
                REQUIRES mutex
        )
else()
        add_library(platform INTERFACE)
        target_include_directories(platform INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(platform INTERFACE mutex)
        target_compile_features(platform INTERFACE cxx_std_20)
        if(PLATFORM STREQUAL "rp2040")
                target_link_libraries(platform INTERFACE pico_sync)
        endif()
        install(TARGETS platform EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )
endif()
