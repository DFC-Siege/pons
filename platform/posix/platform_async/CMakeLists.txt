add_library(platform_async INTERFACE)
target_include_directories(platform_async INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/pons>
)
target_link_libraries(platform_async INTERFACE core_async posix_async)
target_compile_features(platform_async INTERFACE cxx_std_23)

install(TARGETS platform_async EXPORT pons-targets)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/pons
    FILES_MATCHING PATTERN "*.hpp"
)
