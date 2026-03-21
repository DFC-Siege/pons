#include <unity.h>

#include "esp32_logger.hpp"
#include "logger_test.hpp"

static void test_print_info_does_not_crash() {
        logging::Logger logger;
        logger.print(logging::LogLevel::Info, "test", "hello");
}

static void test_print_verbose_does_not_crash() {
        logging::Logger logger;
        logger.print(logging::LogLevel::Verbose, "test", "verbose message");
}

static void test_print_debug_does_not_crash() {
        logging::Logger logger;
        logger.print(logging::LogLevel::Debug, "test", "debug message");
}

static void test_print_warning_does_not_crash() {
        logging::Logger logger;
        logger.print(logging::LogLevel::Warning, "test", "warning message");
}

static void test_print_error_does_not_crash() {
        logging::Logger logger;
        logger.print(logging::LogLevel::Error, "test", "error message");
}

static void test_print_none_does_not_crash() {
        logging::Logger logger;
        logger.print(logging::LogLevel::None, "test", "none message");
}

static void test_println_does_not_crash() {
        logging::Logger logger;
        logger.println(logging::LogLevel::Info, "test", "println message");
}

static void test_print_fmt_does_not_crash() {
        logging::Logger logger;
        logger.println_fmt(logging::LogLevel::Info, "test", "value: {}", 42);
}

static void test_get_level_returns_default() {
        logging::Logger logger;
        TEST_ASSERT_EQUAL(static_cast<int>(logging::LogLevel::Info),
                          static_cast<int>(logger.get_level()));
}

static void test_set_level_changes_level() {
        logging::Logger logger;
        logger.set_level(logging::LogLevel::Warning);
        TEST_ASSERT_EQUAL(static_cast<int>(logging::LogLevel::Warning),
                          static_cast<int>(logger.get_level()));
}

void run_logger_tests() {
        RUN_TEST(test_print_info_does_not_crash);
        RUN_TEST(test_print_verbose_does_not_crash);
        RUN_TEST(test_print_debug_does_not_crash);
        RUN_TEST(test_print_warning_does_not_crash);
        RUN_TEST(test_print_error_does_not_crash);
        RUN_TEST(test_print_none_does_not_crash);
        RUN_TEST(test_println_does_not_crash);
        RUN_TEST(test_print_fmt_does_not_crash);
        RUN_TEST(test_get_level_returns_default);
        RUN_TEST(test_set_level_changes_level);
}
