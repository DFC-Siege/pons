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

TEST_CASE("Chunk assemble returns error on out of order chunks") {
        transport::Chunk c0;
        c0.session_id = 1;
        c0.index = 1;
        c0.total_chunks = 2;
        c0.payload = {0x01};
        c0.checksum = transport::crc16(c0.payload);

        transport::Chunk c1;
        c1.session_id = 1;
        c1.index = 0;
        c1.total_chunks = 2;
        c1.payload = {0x02};
        c1.checksum = transport::crc16(c1.payload);

        std::vector<transport::Chunk> chunks;
        chunks.push_back(std::move(c0));
        chunks.push_back(std::move(c1));
        const auto result = transport::Chunk::assemble(std::move(chunks), 1, 2);
        REQUIRE(result.failed());
}

TEST_CASE("Chunk assemble returns error on missing chunks") {
        transport::Chunk c0;
        c0.session_id = 1;
        c0.index = 0;
        c0.total_chunks = 3;
        c0.payload = {0x01};
        c0.checksum = transport::crc16(c0.payload);

        std::vector<transport::Chunk> chunks;
        chunks.push_back(std::move(c0));
        const auto result = transport::Chunk::assemble(std::move(chunks), 1, 3);
        REQUIRE(result.failed());
}

TEST_CASE("crc16 returns zero for empty data") {
        const auto result = transport::crc16({});
        REQUIRE(result == 0);
}

TEST_CASE("crc16 returns non-zero for non-empty data") {
        const std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        const auto result = transport::crc16(data);
        REQUIRE(result != 0);
}

TEST_CASE("crc16 is deterministic") {
        const std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
        REQUIRE(transport::crc16(data) == transport::crc16(data));
}

TEST_CASE("crc16 differs for different data") {
        const std::vector<uint8_t> a = {0x01, 0x02};
        const std::vector<uint8_t> b = {0x03, 0x04};
        REQUIRE(transport::crc16(a) != transport::crc16(b));
}

TEST_CASE("get_packet_type returns error on empty data") {
        const auto result = transport::get_packet_type({});
        REQUIRE(result.failed());
}

TEST_CASE("get_packet_type returns error on unknown type") {
        const std::vector<uint8_t> buf = {0xFF};
        const auto result = transport::get_packet_type(buf);
        REQUIRE(result.failed());
}

TEST_CASE("get_packet_type identifies chunk") {
        const std::vector<uint8_t> buf = {0x00};
        const auto result = transport::get_packet_type(buf);
        REQUIRE(!result.failed());
        REQUIRE(result.value() == transport::PacketType::chunk);
}

TEST_CASE("get_packet_type identifies ack") {
        const std::vector<uint8_t> buf = {0x01};
        const auto result = transport::get_packet_type(buf);
        REQUIRE(!result.failed());
        REQUIRE(result.value() == transport::PacketType::ack);
}

TEST_CASE("get_packet_type identifies nack") {
        const std::vector<uint8_t> buf = {0x02};
        const auto result = transport::get_packet_type(buf);
        REQUIRE(!result.failed());
        REQUIRE(result.value() == transport::PacketType::nack);
}
