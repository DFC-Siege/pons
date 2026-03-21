add_library(posix_async INTERFACE)
target_include_directories(posix_async INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/pons>
)
target_compile_features(posix_async INTERFACE cxx_std_23)

install(TARGETS posix_async EXPORT pons-targets)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/pons
    FILES_MATCHING PATTERN "*.hpp"
)
