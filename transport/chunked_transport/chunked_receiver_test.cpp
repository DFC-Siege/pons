#include <catch2/catch_test_macros.hpp>
#include <span>
#include <vector>

#include "chunked_receiver.hpp"
#include "packet.hpp"

static std::vector<std::vector<uint8_t>> captured_sends;

static result::Result<bool> mock_sender(std::span<const uint8_t> data) {
        captured_sends.push_back({data.begin(), data.end()});
        return result::ok();
}

static transport::Chunk make_chunk(std::vector<uint8_t> payload, uint16_t index,
                                   uint16_t total, uint8_t session,
                                   uint8_t cmd) {
        return {payload, index, total, transport::crc16(payload), session, cmd};
}

TEST_CASE("start returns error if payload exceeds mtu") {
        captured_sends.clear();
        transport::ChunkedReceiver receiver(16, 3);

        const std::vector<uint8_t> payload(100);
        const auto result =
            receiver.start(0, 0x01, payload, mock_sender, [](auto) {});
        REQUIRE(result.failed());
}

TEST_CASE("start sends initial chunk") {
        captured_sends.clear();
        transport::ChunkedReceiver receiver(512, 3);

        const std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
        const auto result =
            receiver.start(0, 0x01, payload, mock_sender, [](auto) {});
        REQUIRE(!result.failed());
        REQUIRE(!captured_sends.empty());
}

TEST_CASE("receive calls on_complete when last chunk arrives") {
        captured_sends.clear();
        transport::ChunkedReceiver receiver(512, 3);

        std::vector<uint8_t> completed_data;
        const std::vector<uint8_t> initial_payload = {0x01};
        receiver.start(0, 0x01, initial_payload, mock_sender,
                       [&](std::vector<uint8_t> data) {
                               completed_data = std::move(data);
                       });

        const std::vector<uint8_t> payload = {0x04, 0x05, 0x06};
        const auto chunk = make_chunk(payload, 0, 1, 0, 0x01);
        const auto result = receiver.receive(chunk.to_buf());
        REQUIRE(!result.failed());
        REQUIRE(completed_data == payload);
}

TEST_CASE("receive ignores duplicate chunks before completion") {
        captured_sends.clear();
        transport::ChunkedReceiver receiver(512, 3);

        int complete_count = 0;
        receiver.start(0, 0x01, {}, mock_sender,
                       [&](auto) { complete_count++; });

        const auto chunk1 = make_chunk({0x01}, 0, 2, 0, 0x01);
        const auto chunk2 = make_chunk({0x02}, 1, 2, 0, 0x01);

        receiver.receive(chunk1.to_buf());
        receiver.receive(chunk1.to_buf());
        receiver.receive(chunk2.to_buf());
        REQUIRE(complete_count == 1);
}

TEST_CASE("receive rejects chunk with bad checksum") {
        captured_sends.clear();
        transport::ChunkedReceiver receiver(512, 3);

        receiver.start(0, 0x01, {}, mock_sender, [](auto) {});

        transport::Chunk chunk{{0x01, 0x02}, 0, 1, 0xFFFF, 0, 0x01};
        const auto result = receiver.receive(chunk.to_buf());
        REQUIRE(!result.failed());
}

TEST_CASE("receive returns error after max attempts exceeded") {
        captured_sends.clear();
        transport::ChunkedReceiver receiver(512, 2);

        receiver.start(0, 0x01, {}, mock_sender, [](auto) {});

        const std::vector<uint8_t> bad_data = {0x00};
        result::Result<bool> result = result::ok();
        for (int i = 0; i <= 3; i++)
                result = receiver.receive(bad_data);

        REQUIRE(result.failed());
}

TEST_CASE(
    "on_complete is only called once even if last chunk is retransmitted") {
        captured_sends.clear();
        transport::ChunkedReceiver receiver(512, 3);

        int complete_count = 0;
        receiver.start(0, 0x01, {}, mock_sender,
                       [&](auto) { complete_count++; });

        const auto chunk = make_chunk({0x01}, 0, 1, 0, 0x01);
        receiver.receive(chunk.to_buf());
        receiver.receive(chunk.to_buf());
        REQUIRE(complete_count == 1);
}
