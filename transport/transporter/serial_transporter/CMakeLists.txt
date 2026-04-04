if(ESP_PLATFORM)
        idf_component_register(
                SRCS "serial_transporter.cpp"
                INCLUDE_DIRS "."
                REQUIRES result logger serial_hal transporter transport
        )
else()
        add_library(serial_transporter serial_transporter.cpp)
        target_include_directories(serial_transporter PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_link_libraries(serial_transporter PUBLIC result serial_hal transporter logger transport)
        target_compile_features(serial_transporter PUBLIC cxx_std_20)
        install(TARGETS serial_transporter EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )
endif()
