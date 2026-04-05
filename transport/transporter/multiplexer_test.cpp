#include "multiplexer.hpp"
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

using namespace transport;

struct MockTransporter {
        MTU mtu = 64;
        std::vector<Data> sent;
        std::function<void(result::Result<Data>)> receiver;

        result::Status send(Data &&data) {
                sent.push_back(std::move(data));
                return result::ok(true);
        }

        void set_receiver(std::function<void(result::Result<Data>)> cb) {
                receiver = std::move(cb);
        }

        MTU get_mtu() const {
                return mtu;
        }

        void deliver(Data data) {
                if (receiver) {
                        receiver(result::ok(std::move(data)));
                }
        }
};

TEST_CASE("Multiplexer create_inner_channel and send prepends id") {
        MockTransporter mock;
        Multiplexer<MockTransporter> mux(mock);
        auto &channel = mux.create_inner_channel(0x01);

        Data data = {0xAA, 0xBB};
        const auto result = channel.send(std::move(data));

        REQUIRE(!result.failed());
        REQUIRE(mock.sent.size() == 1);
        REQUIRE(mock.sent[0].size() == 3);
        REQUIRE(mock.sent[0][0] == 0x01);
        REQUIRE(mock.sent[0][1] == 0xAA);
        REQUIRE(mock.sent[0][2] == 0xBB);
}

TEST_CASE("Multiplexer create_inner_channel get_mtu subtracts id size") {
        MockTransporter mock;
        mock.mtu = 64;
        Multiplexer<MockTransporter> mux(mock);
        auto &channel = mux.create_inner_channel(0x01);

        REQUIRE(channel.get_mtu() == 63);
}

TEST_CASE("Multiplexer receive dispatches to inner channel") {
        MockTransporter mock;
        Multiplexer<MockTransporter> mux(mock);
        auto &channel = mux.create_inner_channel(0x05);

        std::vector<uint8_t> received_data;
        channel.set_receiver([&](result::Result<Data> result) {
                if (!result.failed()) {
                        received_data = std::move(result).value();
                }
        });

        mock.deliver({0x05, 0xCA, 0xFE});

        REQUIRE(received_data == std::vector<uint8_t>{0xCA, 0xFE});
}

TEST_CASE("Multiplexer handle_receive ignores unknown channel id") {
        MockTransporter mock;
        Multiplexer<MockTransporter> mux(mock);

        REQUIRE_NOTHROW(mock.deliver({0xFF, 0x01, 0x02}));
}

TEST_CASE("Multiplexer handle_receive ignores empty data") {
        MockTransporter mock;
        Multiplexer<MockTransporter> mux(mock);

        REQUIRE_NOTHROW(mock.deliver({}));
}

TEST_CASE("Multiplexer create_inner_channel returns same channel on same id") {
        MockTransporter mock;
        Multiplexer<MockTransporter> mux(mock);
        auto &channel_a = mux.create_inner_channel(0x0A);
        auto &channel_b = mux.create_inner_channel(0x0A);

        REQUIRE(&channel_a == &channel_b);
}
