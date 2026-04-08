#include "multiplexer.hpp"
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

using namespace transport;

struct MockState {
        MTU mtu = 64;
        std::vector<Data> sent;
        std::function<void(result::Result<Data>)> receiver;
        void deliver(Data data) {
                if (receiver) {
                        receiver(result::ok(std::move(data)));
                }
        }
};

struct MockTransporter {
        std::shared_ptr<MockState> state;
        MockTransporter(std::shared_ptr<MockState> s) : state(std::move(s)) {}
        result::Try send(Data &&data) {
                state->sent.push_back(std::move(data));
                return result::ok(true);
        }
        void set_receiver(std::function<void(result::Result<Data>)> cb) {
                state->receiver = std::move(cb);
        }
        MTU get_mtu() const {
                return state->mtu;
        }
};

TEST_CASE("Multiplexer create_inner_channel and send prepends id") {
        auto s = std::make_shared<MockState>();
        Multiplexer<MockTransporter> mux(MockTransporter{s});
        auto &channel = mux.create_inner_channel(0x01);

        Data data = {0xAA, 0xBB};
        const auto result = channel.send(std::move(data));

        REQUIRE(!result.failed());
        REQUIRE(s->sent.size() == 1);
        REQUIRE(s->sent[0].size() == 3);
        REQUIRE(s->sent[0][0] == 0x01);
        REQUIRE(s->sent[0][1] == 0xAA);
        REQUIRE(s->sent[0][2] == 0xBB);
}

TEST_CASE("Multiplexer create_inner_channel get_mtu subtracts id size") {
        auto s = std::make_shared<MockState>();
        s->mtu = 64;
        Multiplexer<MockTransporter> mux(MockTransporter{s});
        auto &channel = mux.create_inner_channel(0x01);

        REQUIRE(channel.get_mtu() == 63);
}

TEST_CASE("Multiplexer receive dispatches to inner channel") {
        auto s = std::make_shared<MockState>();
        Multiplexer<MockTransporter> mux(MockTransporter{s});
        auto &channel = mux.create_inner_channel(0x05);

        std::vector<uint8_t> received_data;
        channel.set_receiver([&](result::Result<Data> result) {
                if (!result.failed()) {
                        received_data = std::move(result).value();
                }
        });

        s->deliver({0x05, 0xCA, 0xFE});

        REQUIRE(received_data == std::vector<uint8_t>{0xCA, 0xFE});
}

TEST_CASE("Multiplexer handle_receive ignores unknown channel id") {
        auto s = std::make_shared<MockState>();
        Multiplexer<MockTransporter> mux(MockTransporter{s});

        REQUIRE_NOTHROW(s->deliver({0xFF, 0x01, 0x02}));
}

TEST_CASE("Multiplexer handle_receive ignores empty data") {
        auto s = std::make_shared<MockState>();
        Multiplexer<MockTransporter> mux(MockTransporter{s});

        REQUIRE_NOTHROW(s->deliver({}));
}

TEST_CASE("Multiplexer send on registered channel succeeds") {
        auto s = std::make_shared<MockState>();
        Multiplexer<MockTransporter> mux(MockTransporter{s});
        auto &channel = mux.create_inner_channel(0x01);

        Data data = {0xAA};
        const auto result = channel.send(std::move(data));
        REQUIRE(!result.failed());
        REQUIRE(s->sent[0][0] == 0x01);
}

TEST_CASE("Multiplexer multiple channels route to correct receiver") {
        auto s = std::make_shared<MockState>();
        Multiplexer<MockTransporter> mux(MockTransporter{s});
        auto &ch_a = mux.create_inner_channel(0x0A);
        auto &ch_b = mux.create_inner_channel(0x0B);

        std::optional<Data> received_a;
        std::optional<Data> received_b;
        ch_a.set_receiver([&](result::Result<Data> r) {
                if (!r.failed())
                        received_a = std::move(r).value();
        });
        ch_b.set_receiver([&](result::Result<Data> r) {
                if (!r.failed())
                        received_b = std::move(r).value();
        });

        s->deliver({0x0A, 0x01, 0x02});
        s->deliver({0x0B, 0x03, 0x04});

        REQUIRE(received_a.has_value());
        REQUIRE(received_a.value() == Data{0x01, 0x02});
        REQUIRE(received_b.has_value());
        REQUIRE(received_b.value() == Data{0x03, 0x04});
}

TEST_CASE("Multiplexer channel send does not affect other channels") {
        auto s = std::make_shared<MockState>();
        Multiplexer<MockTransporter> mux(MockTransporter{s});
        auto &ch_a = mux.create_inner_channel(0x0A);
        auto &ch_b = mux.create_inner_channel(0x0B);

        ch_a.send(Data{0x01});
        ch_b.send(Data{0x02});

        REQUIRE(s->sent.size() == 2);
        REQUIRE(s->sent[0][0] == 0x0A);
        REQUIRE(s->sent[1][0] == 0x0B);
}
