if(ESP_PLATFORM)
        idf_component_register(
                SRCS "base_transporter.cpp"
                INCLUDE_DIRS "."
                REQUIRES result platform_async core_async
        )
else()
        add_library(transport base_transporter.cpp)
        target_include_directories(transport PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(transport PUBLIC result platform_async core_async)
        target_compile_features(transport PUBLIC cxx_std_20)
        install(TARGETS transport EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )
endif()
