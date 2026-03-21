#include <catch2/catch_test_macros.hpp>
#include <span>
#include <thread>
#include <vector>

#include "chunked_transporter.hpp"
#include "packet.hpp"

struct TestTransporter : transport::ChunkedTransporter {
        static constexpr auto MAX_ATTEMPTS = 1;
        std::vector<std::vector<uint8_t>> sent;
        bool send_fails = false;

        TestTransporter(uint16_t mtu) : ChunkedTransporter(mtu, MAX_ATTEMPTS) {
        }

        result::Result<bool>
        concrete_send(std::span<const uint8_t> data) override {
                if (send_fails)
                        return result::err("send failed");
                sent.push_back({data.begin(), data.end()});
                return result::ok();
        }

        void simulate_receive(std::span<const uint8_t> data) {
                feed(data);
        }
};

TEST_CASE("send returns error when no sessions available") {
        TestTransporter transporter(512);
        for (int i = 0; i < 255; i++)
                transporter.send(0x01, {}, [] {}, [](auto) {});

        const auto result = transporter.send(0x01, {}, [] {}, [](auto) {});
        REQUIRE(result.failed());
}

TEST_CASE("send transmits data") {
        TestTransporter transporter(512);
        const std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        const auto result = transporter.send(0x01, data, [] {}, [](auto) {});
        REQUIRE(!result.failed());
        REQUIRE(!transporter.sent.empty());
}

TEST_CASE("send calls on_complete after ack") {
        TestTransporter transporter(512);

        bool completed = false;
        const std::vector<uint8_t> data = {0x01};
        transporter.send(0x01, data, [&] { completed = true; }, [](auto) {});

        const auto sent = transporter.sent.back();
        const auto chunk = transport::Chunk::from_buf(sent);
        REQUIRE(!chunk.failed());

        const auto session_id = sent[transport::Chunk::SESSION_ID_OFFSET];
        const transport::Ack ack{chunk.value().index, session_id, true};
        transporter.simulate_receive(ack.to_buf());
        REQUIRE(completed);
}

TEST_CASE("request returns error when no sessions available") {
        TestTransporter transporter(512);
        for (int i = 0; i < 255; i++)
                transporter.request(0x01, {}, [](auto) {}, [](auto) {});

        const auto result =
            transporter.request(0x01, {}, [](auto) {}, [](auto) {});
        REQUIRE(result.failed());
}

TEST_CASE("request transmits data") {
        TestTransporter transporter(512);
        const std::vector<uint8_t> payload = {0x01, 0x02};
        const auto result =
            transporter.request(0x01, payload, [](auto) {}, [](auto) {});
        REQUIRE(!result.failed());
        REQUIRE(!transporter.sent.empty());
}

TEST_CASE("feed returns error on empty data") {
        TestTransporter transporter(512);
        const auto result = transporter.feed({});
        REQUIRE(result.failed());
}

TEST_CASE("feed returns error on unknown session") {
        TestTransporter transporter(512);
        const std::vector<uint8_t> data = {0x01, 0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0x00, 0x00};
        const auto result = transporter.feed(data);
        REQUIRE(result.failed());
}

TEST_CASE("send_async returns non-null future") {
        TestTransporter transporter(512);
        const std::vector<uint8_t> data = {0x01};
        auto future = transporter.send_async(0x01, data);
        REQUIRE(future != nullptr);
}

TEST_CASE("request_async returns non-null future") {
        TestTransporter transporter(512);
        const std::vector<uint8_t> payload = {0x01};
        auto future = transporter.request_async(0x01, payload);
        REQUIRE(future != nullptr);
}

TEST_CASE("send_async future resolves after ack") {
        TestTransporter transporter(512);

        const std::vector<uint8_t> data = {0x01};
        auto future = transporter.send_async(0x01, data);

        const auto sent = transporter.sent.back();
        const auto chunk = transport::Chunk::from_buf(sent);
        REQUIRE(!chunk.failed());

        const auto session_id = sent[transport::Chunk::SESSION_ID_OFFSET];
        const transport::Ack ack{chunk.value().index, session_id, true};
        transporter.simulate_receive(ack.to_buf());

        REQUIRE(future->wait_for(1000));
        const auto result = future->get();
        REQUIRE(!result.failed());
}
