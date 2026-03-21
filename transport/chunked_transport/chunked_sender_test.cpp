#include <catch2/catch_test_macros.hpp>
#include <span>
#include <vector>

#include "chunked_sender.hpp"
#include "packet.hpp"

static std::vector<std::vector<uint8_t>> captured_sends;

static result::Result<bool> mock_sender(std::span<const uint8_t> data) {
        captured_sends.push_back({data.begin(), data.end()});
        return result::ok();
}

static transport::Ack make_ack(uint16_t index, uint8_t session, bool success) {
        return {index, session, success};
}

TEST_CASE("send returns error if mtu too small") {
        captured_sends.clear();
        transport::ChunkedSender sender(4, 3);

        const std::vector<uint8_t> data = {0x01};
        const auto result = sender.send(0, 0x01, data, mock_sender, [] {});
        REQUIRE(result.failed());
}

TEST_CASE("send transmits first chunk") {
        captured_sends.clear();
        transport::ChunkedSender sender(512, 3);

        const std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        const auto result = sender.send(0, 0x01, data, mock_sender, [] {});
        REQUIRE(!result.failed());
        REQUIRE(!captured_sends.empty());
}

TEST_CASE("send with empty data sends single empty chunk") {
        captured_sends.clear();
        transport::ChunkedSender sender(512, 3);

        const auto result = sender.send(0, 0x01, {}, mock_sender, [] {});
        REQUIRE(!result.failed());
        REQUIRE(captured_sends.size() == 1);
}

TEST_CASE("receive with success ack advances to next chunk") {
        captured_sends.clear();
        transport::ChunkedSender sender(12, 3);

        const std::vector<uint8_t> data(20, 0xAB);
        sender.send(0, 0x01, data, mock_sender, [] {});

        const auto prev_sends = captured_sends.size();
        const auto ack = make_ack(0, 0, true);
        const auto result = sender.receive(ack.to_buf());
        REQUIRE(!result.failed());
        REQUIRE(captured_sends.size() > prev_sends);
}

TEST_CASE("receive with failed ack retransmits current chunk") {
        captured_sends.clear();
        transport::ChunkedSender sender(512, 3);

        const std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        sender.send(0, 0x01, data, mock_sender, [] {});

        const auto first_chunk = captured_sends.back();
        const auto ack = make_ack(0, 0, false);
        sender.receive(ack.to_buf());
        REQUIRE(captured_sends.back() == first_chunk);
}

TEST_CASE("receive calls on_complete after last ack") {
        captured_sends.clear();
        transport::ChunkedSender sender(512, 3);

        bool completed = false;
        const std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        sender.send(0, 0x01, data, mock_sender, [&] { completed = true; });

        const auto ack = make_ack(0, 0, true);
        sender.receive(ack.to_buf());
        REQUIRE(completed);
}

TEST_CASE("receive returns error after max attempts") {
        captured_sends.clear();
        transport::ChunkedSender sender(512, 2);

        const std::vector<uint8_t> data = {0x01};
        sender.send(0, 0x01, data, mock_sender, [] {});

        const auto ack = make_ack(0, 0, false);
        result::Result<bool> result = result::ok();
        for (int i = 0; i <= 2; i++)
                result = sender.receive(ack.to_buf());

        REQUIRE(result.failed());
}

TEST_CASE("large data is split into multiple chunks") {
        captured_sends.clear();
        transport::ChunkedSender sender(16, 3);

        const std::vector<uint8_t> data(100, 0xFF);
        sender.send(0, 0x01, data, mock_sender, [] {});
        REQUIRE(captured_sends.size() == 1);

        INFO("first byte: " << (int)captured_sends[0][0]);
        INFO("buffer size: " << captured_sends[0].size());
        const auto chunk_result = transport::Chunk::from_buf(captured_sends[0]);
        REQUIRE(!chunk_result.failed());
        REQUIRE(chunk_result.value().total_chunks > 1);
}
