#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <functional>

#include "console_logger.hpp"

static std::string capture(std::function<void()> fn) {
        const char *tmp = "/tmp/pons_test_output";
        freopen(tmp, "w", stdout);
        fn();
        fflush(stdout);
        freopen("/dev/tty", "w", stdout);

        FILE *f = fopen(tmp, "r");
        char buf[1024] = {};
        fread(buf, 1, sizeof(buf), f);
        fclose(f);
        return buf;
}

TEST_CASE("println outputs tag and message") {
        logging::ConsoleLogger logger;
        const auto output = capture(
            [&] { logger.println(logging::LogLevel::Info, "tag", "message"); });
        REQUIRE(output.find("tag") != std::string::npos);
        REQUIRE(output.find("message") != std::string::npos);
}

TEST_CASE("println appends newline") {
        logging::ConsoleLogger logger;
        const auto output = capture(
            [&] { logger.println(logging::LogLevel::Info, "tag", "message"); });
        REQUIRE(output.back() == '\n');
}

TEST_CASE("print does not append newline") {
        logging::ConsoleLogger logger;
        const auto output = capture(
            [&] { logger.print(logging::LogLevel::Info, "tag", "message"); });
        REQUIRE(output.back() != '\n');
}

TEST_CASE("output is suppressed below log level") {
        logging::ConsoleLogger logger;
        logger.set_level(logging::LogLevel::Error);
        const auto output = capture(
            [&] { logger.println(logging::LogLevel::Info, "tag", "message"); });
        REQUIRE(output.empty());
}

TEST_CASE("output is shown at exact log level") {
        logging::ConsoleLogger logger;
        logger.set_level(logging::LogLevel::Info);
        const auto output = capture(
            [&] { logger.println(logging::LogLevel::Info, "tag", "message"); });
        REQUIRE(!output.empty());
}

TEST_CASE("log level string is included in output") {
        logging::ConsoleLogger logger;
        const auto output = capture([&] {
                logger.println(logging::LogLevel::Error, "tag", "message");
        });
        REQUIRE(output.find("E") != std::string::npos);
}
