#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

#include "direct_transporter.hpp"

using namespace transport;

struct MockTransporter {
        MTU mtu = 64;
        std::vector<Data> sent;
        std::function<void(result::Result<Data>)> receiver;

        result::Try send(Data &&data) {
                sent.push_back(std::move(data));
                return result::ok(true);
        }

        void set_receiver(std::function<void(result::Result<Data>)> cb) {
                receiver = std::move(cb);
        }

        MTU get_mtu() const {
                return mtu;
        }

        void deliver(result::Result<Data> data) {
                if (receiver) {
                        receiver(std::move(data));
                }
        }
};

TEST_CASE("DirectTransporter send forwards data to underlying transporter") {
        MockTransporter mock;
        DirectTransporter<MockTransporter> dt(mock);

        Data data = {0x01, 0x02, 0x03};
        const auto result = dt.send(std::move(data));

        REQUIRE(!result.failed());
        REQUIRE(mock.sent.size() == 1);
        REQUIRE(mock.sent[0] == Data{0x01, 0x02, 0x03});
}

TEST_CASE("DirectTransporter get_mtu returns underlying transporter mtu") {
        MockTransporter mock;
        mock.mtu = 128;
        DirectTransporter<MockTransporter> dt(mock);

        REQUIRE(dt.get_mtu() == 128);
}

TEST_CASE("DirectTransporter receive forwards data to callback") {
        MockTransporter mock;
        DirectTransporter<MockTransporter> dt(mock);

        std::optional<Data> received;
        dt.set_receiver([&](result::Result<Data> result) {
                if (!result.failed()) {
                        received = std::move(result).value();
                }
        });

        mock.deliver(result::ok(Data{0xDE, 0xAD}));

        REQUIRE(received.has_value());
        REQUIRE(received.value() == Data{0xDE, 0xAD});
}

TEST_CASE("DirectTransporter receive with no callback does not crash") {
        MockTransporter mock;
        DirectTransporter<MockTransporter> dt(mock);

        REQUIRE_NOTHROW(mock.deliver(result::ok(Data{0x01})));
}

TEST_CASE("DirectTransporter forwards failed result to callback") {
        MockTransporter mock;
        DirectTransporter<MockTransporter> dt(mock);

        std::optional<std::string> error;
        dt.set_receiver([&](result::Result<Data> result) {
                if (result.failed()) {
                        error = std::string(result.error());
                }
        });

        mock.deliver(result::err("test error"));

        REQUIRE(error.has_value());
        REQUIRE(error.value() == "test error");
}
