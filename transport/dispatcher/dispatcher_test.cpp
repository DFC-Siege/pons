#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

#include "dispatcher.hpp"

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
                state->sent.push_back(data);
                return result::ok(true);
        }
        void set_receiver(std::function<void(result::Result<Data>)> cb) {
                state->receiver = std::move(cb);
        }
        MTU get_mtu() const {
                return state->mtu;
        }
};

TEST_CASE("Dispatcher send wraps and forwards data") {
        auto s = std::make_shared<MockState>();
        Dispatcher<MockTransporter> dispatcher;
        dispatcher.register_transporter(0x00, MockTransporter{s});

        Data data = {0xAA, 0xBB};
        const auto result = dispatcher.send(0x00, 0x0001, std::move(data));

        REQUIRE(!result.failed());
        REQUIRE(s->sent.size() == 1);
        REQUIRE(s->sent[0].size() == sizeof(CommandId) + 2);
        REQUIRE(s->sent[0][0] == 0x01);
        REQUIRE(s->sent[0][1] == 0x00);
        REQUIRE(s->sent[0][2] == 0xAA);
        REQUIRE(s->sent[0][3] == 0xBB);
}

TEST_CASE("Dispatcher receive dispatches to registered handler") {
        auto s = std::make_shared<MockState>();
        Dispatcher<MockTransporter> dispatcher;
        dispatcher.register_transporter(0x00, MockTransporter{s});

        std::optional<Data> received;
        dispatcher.register_handler(0x0001, [&](result::Result<Data> result) {
                if (!result.failed()) {
                        received = std::move(result).value();
                }
        });

        s->deliver({0x01, 0x00, 0xAA, 0xBB});

        REQUIRE(received.has_value());
        REQUIRE(received.value() == Data{0xAA, 0xBB});
}

TEST_CASE("Dispatcher receive with no handler does not crash") {
        auto s = std::make_shared<MockState>();
        Dispatcher<MockTransporter> dispatcher;
        dispatcher.register_transporter(0x00, MockTransporter{s});

        REQUIRE_NOTHROW(s->deliver({0x01, 0x00, 0xAA, 0xBB}));
}

TEST_CASE("Dispatcher receive with data too small does not crash") {
        auto s = std::make_shared<MockState>();
        Dispatcher<MockTransporter> dispatcher;
        dispatcher.register_transporter(0x00, MockTransporter{s});

        REQUIRE_NOTHROW(s->deliver({0x01}));
}

TEST_CASE("Dispatcher receive with empty data does not crash") {
        auto s = std::make_shared<MockState>();
        Dispatcher<MockTransporter> dispatcher;
        dispatcher.register_transporter(0x00, MockTransporter{s});

        REQUIRE_NOTHROW(s->deliver({}));
}

TEST_CASE("Dispatcher register_handler replaces existing handler") {
        auto s = std::make_shared<MockState>();
        Dispatcher<MockTransporter> dispatcher;
        dispatcher.register_transporter(0x00, MockTransporter{s});

        int call_count = 0;
        dispatcher.register_handler(
            0x0001, [&](result::Result<Data>) { call_count++; });
        dispatcher.register_handler(
            0x0001, [&](result::Result<Data>) { call_count += 10; });

        s->deliver({0x01, 0x00, 0xAA});

        REQUIRE(call_count == 10);
}

TEST_CASE("Dispatcher dispatches to correct handler by command id") {
        auto s = std::make_shared<MockState>();
        Dispatcher<MockTransporter> dispatcher;
        dispatcher.register_transporter(0x00, MockTransporter{s});

        std::optional<Data> received_a;
        std::optional<Data> received_b;
        dispatcher.register_handler(0x0001, [&](result::Result<Data> result) {
                if (!result.failed())
                        received_a = std::move(result).value();
        });
        dispatcher.register_handler(0x0002, [&](result::Result<Data> result) {
                if (!result.failed())
                        received_b = std::move(result).value();
        });

        s->deliver({0x02, 0x00, 0xFF});

        REQUIRE(!received_a.has_value());
        REQUIRE(received_b.has_value());
        REQUIRE(received_b.value() == Data{0xFF});
}

TEST_CASE("WrappedData wrap and unwrap round trips") {
        Data original = {0x01, 0x02, 0x03};
        const auto wrap_result = WrappedData::wrap_data(0x0042, Data(original));
        REQUIRE(!wrap_result.failed());

        const auto unwrap_result =
            WrappedData::unwrap_data(Data(wrap_result.value()));
        REQUIRE(!unwrap_result.failed());
        REQUIRE(unwrap_result.value().command_id == 0x0042);
        REQUIRE(unwrap_result.value().data == original);
}

TEST_CASE("WrappedData unwrap returns error on data too small") {
        const auto result = WrappedData::unwrap_data({0x01});
        REQUIRE(result.failed());
}

TEST_CASE("WrappedData unwrap returns error on empty data") {
        const auto result = WrappedData::unwrap_data({});
        REQUIRE(result.failed());
}

TEST_CASE("Dispatcher send to non-existent transporter returns error") {
        Dispatcher<MockTransporter> dispatcher;
        const auto result = dispatcher.send(0x42, 0x01, Data{0xAA});
        REQUIRE(result.failed());
}

TEST_CASE("Dispatcher multiple transporters route independently") {
        auto s_a = std::make_shared<MockState>();
        auto s_b = std::make_shared<MockState>();
        Dispatcher<MockTransporter> dispatcher;
        dispatcher.register_transporter(0x01, MockTransporter{s_a});
        dispatcher.register_transporter(0x02, MockTransporter{s_b});

        dispatcher.send(0x01, 0x00, Data{0xAA});
        dispatcher.send(0x02, 0x00, Data{0xBB});

        REQUIRE(s_a->sent.size() == 1);
        REQUIRE(s_b->sent.size() == 1);
}

TEST_CASE("WrappedData wrap produces correct little-endian command id") {
        const auto result = WrappedData::wrap_data(0x0102, Data{0xFF});
        REQUIRE(!result.failed());
        const auto &buf = result.value();
        REQUIRE(buf[0] == 0x02);
        REQUIRE(buf[1] == 0x01);
        REQUIRE(buf[2] == 0xFF);
}
