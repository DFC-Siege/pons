#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstring>
#include <functional>

#include "console_logger.hpp"

static std::string capture_stdout(std::function<void()> fn) {
        char buffer[1024] = {};
        FILE *old = stdout;
        stdout = fmemopen(buffer, sizeof(buffer), "w");
        fn();
        fflush(stdout);
        fclose(stdout);
        stdout = old;
        return buffer;
}

TEST_CASE("println outputs tag and message") {
        logging::ConsoleLogger logger;
        const auto output = capture_stdout(
            [&] { logger.println(logging::LogLevel::Info, "tag", "message"); });
        REQUIRE(output.find("tag") != std::string::npos);
        REQUIRE(output.find("message") != std::string::npos);
}

TEST_CASE("println appends newline") {
        logging::ConsoleLogger logger;
        const auto output = capture_stdout(
            [&] { logger.println(logging::LogLevel::Info, "tag", "message"); });
        REQUIRE(output.back() == '\n');
}

TEST_CASE("print does not append newline") {
        logging::ConsoleLogger logger;
        const auto output = capture_stdout(
            [&] { logger.print(logging::LogLevel::Info, "tag", "message"); });
        REQUIRE(output.back() != '\n');
}

TEST_CASE("output is suppressed below log level") {
        logging::ConsoleLogger logger;
        logger.set_level(logging::LogLevel::Error);
        const auto output = capture_stdout(
            [&] { logger.println(logging::LogLevel::Info, "tag", "message"); });
        REQUIRE(output.empty());
}

TEST_CASE("output is shown at exact log level") {
        logging::ConsoleLogger logger;
        logger.set_level(logging::LogLevel::Info);
        const auto output = capture_stdout(
            [&] { logger.println(logging::LogLevel::Info, "tag", "message"); });
        REQUIRE(!output.empty());
}
