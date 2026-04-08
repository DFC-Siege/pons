#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "transport_data.hpp"

using namespace transport;

TEST_CASE("push_le and pull_le round trip uint8_t") {
        Data buf;
        detail::push_le<uint8_t>(buf, 0xAB);
        REQUIRE(buf.size() == 1);
        REQUIRE(detail::pull_le<uint8_t>(buf, 0) == 0xAB);
}

TEST_CASE("push_le and pull_le round trip uint16_t") {
        Data buf;
        detail::push_le<uint16_t>(buf, 0xBEEF);
        REQUIRE(buf.size() == 2);
        REQUIRE(buf[0] == 0xEF);
        REQUIRE(buf[1] == 0xBE);
        REQUIRE(detail::pull_le<uint16_t>(buf, 0) == 0xBEEF);
}

TEST_CASE("push_le and pull_le round trip uint32_t") {
        Data buf;
        detail::push_le<uint32_t>(buf, 0xDEADBEEF);
        REQUIRE(buf.size() == 4);
        REQUIRE(detail::pull_le<uint32_t>(buf, 0) == 0xDEADBEEF);
}

TEST_CASE("pull_le at non-zero offset") {
        Data buf = {0x00, 0x00, 0xEF, 0xBE};
        REQUIRE(detail::pull_le<uint16_t>(buf, 2) == 0xBEEF);
}

TEST_CASE("push_le multiple values are sequential") {
        Data buf;
        detail::push_le<uint8_t>(buf, 0x01);
        detail::push_le<uint16_t>(buf, 0x0203);
        REQUIRE(buf.size() == 3);
        REQUIRE(detail::pull_le<uint8_t>(buf, 0) == 0x01);
        REQUIRE(detail::pull_le<uint16_t>(buf, 1) == 0x0203);
}

enum class TestEnum : uint8_t { A = 0x01, B = 0x02 };

TEST_CASE("push_le and pull_le round trip enum") {
        Data buf;
        detail::push_le(buf, TestEnum::B);
        REQUIRE(buf.size() == 1);
        REQUIRE(detail::pull_le<TestEnum>(buf, 0) == TestEnum::B);
}

enum class TestEnum16 : uint16_t { X = 0x1234 };

TEST_CASE("push_le and pull_le round trip 16-bit enum") {
        Data buf;
        detail::push_le(buf, TestEnum16::X);
        REQUIRE(buf.size() == 2);
        REQUIRE(detail::pull_le<TestEnum16>(buf, 0) == TestEnum16::X);
}
