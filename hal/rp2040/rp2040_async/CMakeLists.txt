add_library(rp2040_async INTERFACE)
target_include_directories(rp2040_async INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/pons>
)
target_link_libraries(rp2040_async INTERFACE pico_sync)
target_compile_features(rp2040_async INTERFACE cxx_std_20)

install(TARGETS rp2040_async EXPORT pons-targets)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/pons
    FILES_MATCHING PATTERN "*.hpp"
)
