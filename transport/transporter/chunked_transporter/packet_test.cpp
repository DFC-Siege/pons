#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "packet.hpp"

TEST_CASE("Ack to_buf and from_buf round trip") {
        const transport::Ack ack{42, 3, true};
        const auto buf = ack.to_buf();
        const auto result = transport::Ack::from_buf(buf);
        REQUIRE(!result.failed());
        REQUIRE(result.value().index == 42);
        REQUIRE(result.value().session_id == 3);
        REQUIRE(result.value().success == true);
}

TEST_CASE("Ack from_buf returns error on buffer too small") {
        const std::vector<uint8_t> buf = {0x02, 0x00};
        const auto result = transport::Ack::from_buf(buf);
        REQUIRE(result.failed());
}

TEST_CASE("Ack from_buf returns error on invalid packet type") {
        const std::vector<uint8_t> buf = {0x01, 0x00, 0x00, 0x00, 0x00};
        const auto result = transport::Ack::from_buf(buf);
        REQUIRE(result.failed());
}

TEST_CASE("Ack failed round trip") {
        const transport::Ack ack{0, 0, false};
        const auto result = transport::Ack::from_buf(ack.to_buf());
        REQUIRE(!result.failed());
        REQUIRE(result.value().success == false);
}

TEST_CASE("Chunk to_buf and from_buf round trip") {
        const std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
        const transport::Chunk chunk{payload, 0,   1, transport::crc16(payload),
                                     1,       0x42};
        const auto buf = chunk.to_buf();
        const auto result = transport::Chunk::from_buf(buf);
        REQUIRE(!result.failed());
        REQUIRE(result.value().index == 0);
        REQUIRE(result.value().total_chunks == 1);
        REQUIRE(result.value().session_id == 1);
        REQUIRE(result.value().command == 0x42);
        REQUIRE(result.value().payload == payload);
}

TEST_CASE("Chunk from_buf returns error on buffer too small") {
        const std::vector<uint8_t> buf = {0x01, 0x00, 0x00};
        const auto result = transport::Chunk::from_buf(buf);
        REQUIRE(result.failed());
}

TEST_CASE("Chunk from_buf returns error on invalid packet type") {
        const std::vector<uint8_t> buf(transport::Chunk::HEADER_SIZE, 0x00);
        const auto result = transport::Chunk::from_buf(buf);
        REQUIRE(result.failed());
}

TEST_CASE("Chunk from_buf returns error on checksum mismatch") {
        const std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
        const transport::Chunk chunk{payload, 0, 1, 0xFFFF, 0, 0x01};
        const auto result = transport::Chunk::from_buf(chunk.to_buf());
        REQUIRE(result.failed());
}

TEST_CASE("Chunk with empty payload round trips") {
        const std::vector<uint8_t> payload = {};
        const transport::Chunk chunk{payload, 0,   1, transport::crc16(payload),
                                     0,       0x01};
        const auto result = transport::Chunk::from_buf(chunk.to_buf());
        REQUIRE(!result.failed());
        REQUIRE(result.value().payload.empty());
}

TEST_CASE("crc16 is consistent") {
        const std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        REQUIRE(transport::crc16(data) == transport::crc16(data));
}

TEST_CASE("crc16 differs for different data") {
        const std::vector<uint8_t> a = {0x01};
        const std::vector<uint8_t> b = {0x02};
        REQUIRE(transport::crc16(a) != transport::crc16(b));
}
