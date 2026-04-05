#include "packet.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("Ack to_buf and from_buf round trip") {
        const transport::Ack ack{{3}, 42};
        const auto buf = ack.to_buf();
        const auto result = transport::Ack::from_buf(buf);
        REQUIRE(!result.failed());
        REQUIRE(result.value().index == 42);
        REQUIRE(result.value().session_id == 3);
}

TEST_CASE("Ack from_buf returns error on buffer too small") {
        const std::vector<uint8_t> buf = {0x01, 0x00};
        const auto result = transport::Ack::from_buf(buf);
        REQUIRE(result.failed());
}

TEST_CASE("Ack from_buf returns error on invalid packet type") {
        const std::vector<uint8_t> buf = {0x00, 0x00, 0x00, 0x00};
        const auto result = transport::Ack::from_buf(buf);
        REQUIRE(result.failed());
}

TEST_CASE("Nack to_buf and from_buf round trip") {
        const transport::Nack nack{{7}, 5};
        const auto buf = nack.to_buf();
        const auto result = transport::Nack::from_buf(buf);
        REQUIRE(!result.failed());
        REQUIRE(result.value().index == 5);
        REQUIRE(result.value().session_id == 7);
}

TEST_CASE("Nack from_buf returns error on buffer too small") {
        const std::vector<uint8_t> buf = {0x02, 0x00};
        const auto result = transport::Nack::from_buf(buf);
        REQUIRE(result.failed());
}

TEST_CASE("Nack from_buf returns error on invalid packet type") {
        const std::vector<uint8_t> buf = {0x00, 0x00, 0x00, 0x00, 0x00};
        const auto result = transport::Nack::from_buf(buf);
        REQUIRE(result.failed());
}

TEST_CASE("Chunk to_buf and from_buf round trip") {
        const std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
        transport::Chunk chunk;
        chunk.session_id = 1;
        chunk.index = 0;
        chunk.total_chunks = 1;
        chunk.payload = payload;
        chunk.checksum = transport::crc16(payload);
        const auto buf = chunk.to_buf();
        const auto result = transport::Chunk::from_buf(buf);
        REQUIRE(!result.failed());
        REQUIRE(result.value().index == 0);
        REQUIRE(result.value().total_chunks == 1);
        REQUIRE(result.value().session_id == 1);
        REQUIRE(result.value().payload == payload);
}

TEST_CASE("Chunk from_buf returns error on buffer too small") {
        const std::vector<uint8_t> buf = {0x00, 0x00, 0x00};
        const auto result = transport::Chunk::from_buf(buf);
        REQUIRE(result.failed());
}

TEST_CASE("Chunk from_buf returns error on invalid packet type") {
        std::vector<uint8_t> buf(transport::Chunk::HEADER_SIZE, 0x00);
        buf[0] = 0x01; // Not PacketType::chunk
        const auto result = transport::Chunk::from_buf(buf);
        REQUIRE(result.failed());
}

TEST_CASE("Chunk from_buf returns error on checksum mismatch") {
        transport::Chunk chunk;
        chunk.session_id = 1;
        chunk.index = 0;
        chunk.total_chunks = 1;
        chunk.payload = {0x01, 0x02, 0x03};
        chunk.checksum = 0xFFFF;
        const auto result = transport::Chunk::from_buf(chunk.to_buf());
        REQUIRE(result.failed());
}

TEST_CASE("Chunk fragment produces correct number of chunks") {
        const std::vector<uint8_t> data(100, 0xAB);
        const auto result = transport::Chunk::fragment(data, 32, 1);
        REQUIRE(!result.failed());
        const auto &chunks = result.value();
        const size_t max_payload = 32 - transport::Chunk::HEADER_SIZE;
        const size_t expected = (100 + max_payload - 1) / max_payload;
        REQUIRE(chunks.size() == expected);
}

TEST_CASE("Chunk fragment and assemble round trips") {
        const std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
        const transport::SessionId session_id = 1;
        auto frag_result = transport::Chunk::fragment(data, 32, session_id);
        REQUIRE(!frag_result.failed());
        auto chunks = std::move(frag_result).value();
        const transport::Indexer total =
            static_cast<transport::Indexer>(chunks.size());
        auto assemble_result =
            transport::Chunk::assemble(std::move(chunks), session_id, total);
        REQUIRE(!assemble_result.failed());
        REQUIRE(assemble_result.value() == data);
}

TEST_CASE("Chunk fragment returns error on empty data") {
        const auto result = transport::Chunk::fragment({}, 32, 1);
        REQUIRE(result.failed());
}

TEST_CASE("Chunk fragment returns error on MTU too small") {
        const auto result = transport::Chunk::fragment({0x01}, 1, 1);
        REQUIRE(result.failed());
}

TEST_CASE("Chunk assemble returns error on empty chunks") {
        const auto result = transport::Chunk::assemble({}, 0, 0);
        REQUIRE(result.failed());
}

TEST_CASE("Chunk assemble returns error on session id mismatch") {
        transport::Chunk chunk;
        chunk.session_id = 99;
        chunk.index = 0;
        chunk.total_chunks = 1;
        chunk.payload = {0x01};
        chunk.checksum = transport::crc16(chunk.payload);
        std::vector<transport::Chunk> chunks;
        chunks.push_back(std::move(chunk));
        const auto result = transport::Chunk::assemble(std::move(chunks), 1, 1);
        REQUIRE(result.failed());
}
