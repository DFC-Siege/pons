#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

#include "direct_transporter.hpp"

using namespace transport;

struct MockState {
        MTU mtu = 64;
        std::vector<Data> sent;
        std::function<void(result::Result<Data>)> receiver;
        void deliver(result::Result<Data> data) {
                if (receiver) {
                        receiver(std::move(data));
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

TEST_CASE("DirectTransporter send forwards data to underlying transporter") {
        auto s = std::make_shared<MockState>();
        DirectTransporter<MockTransporter> dt(std::make_unique<MockTransporter>(s));

        Data data = {0x01, 0x02, 0x03};
        const auto result = dt.send(std::move(data));

        REQUIRE(!result.failed());
        REQUIRE(s->sent.size() == 1);
        REQUIRE(s->sent[0] == Data{0x01, 0x02, 0x03});
}

TEST_CASE("DirectTransporter get_mtu returns underlying transporter mtu") {
        auto s = std::make_shared<MockState>();
        s->mtu = 128;
        DirectTransporter<MockTransporter> dt(std::make_unique<MockTransporter>(s));

        REQUIRE(dt.get_mtu() == 128);
}

TEST_CASE("DirectTransporter receive forwards data to callback") {
        auto s = std::make_shared<MockState>();
        DirectTransporter<MockTransporter> dt(std::make_unique<MockTransporter>(s));

        std::optional<Data> received;
        dt.set_receiver([&](result::Result<Data> result) {
                if (!result.failed()) {
                        received = std::move(result).value();
                }
        });

        s->deliver(result::ok(Data{0xDE, 0xAD}));

        REQUIRE(received.has_value());
        REQUIRE(received.value() == Data{0xDE, 0xAD});
}

TEST_CASE("DirectTransporter receive with no callback does not crash") {
        auto s = std::make_shared<MockState>();
        DirectTransporter<MockTransporter> dt(std::make_unique<MockTransporter>(s));

        REQUIRE_NOTHROW(s->deliver(result::ok(Data{0x01})));
}

TEST_CASE("DirectTransporter forwards failed result to callback") {
        auto s = std::make_shared<MockState>();
        DirectTransporter<MockTransporter> dt(std::make_unique<MockTransporter>(s));

        std::optional<std::string> error;
        dt.set_receiver([&](result::Result<Data> result) {
                if (result.failed()) {
                        error = std::string(result.error());
                }
        });

        s->deliver(result::err("test error"));

        REQUIRE(error.has_value());
        REQUIRE(error.value() == "test error");
}
