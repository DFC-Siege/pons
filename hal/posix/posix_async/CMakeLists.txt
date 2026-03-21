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

add_executable(posix_async_test posix_async_test.cpp)
target_link_libraries(posix_async_test PRIVATE Catch2::Catch2WithMain posix_async)
target_compile_features(posix_async_test PRIVATE cxx_std_23)
catch_discover_tests(posix_async_test)
