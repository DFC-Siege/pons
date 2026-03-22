if(ESP_PLATFORM)
        idf_component_register(
                SRCS "logger.cpp"
                INCLUDE_DIRS "."
        )
else()
        add_library(logger logger.cpp)
        target_include_directories(logger PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/pons>
        )
        target_compile_features(logger PUBLIC cxx_std_20)

        install(TARGETS logger EXPORT pons-targets)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DESTINATION include/pons
                FILES_MATCHING PATTERN "*.hpp"
        )
endif()
