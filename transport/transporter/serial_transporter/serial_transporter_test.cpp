#include "serial_transporter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

using namespace transport;

struct MockSerialHal : public serial::ISerialHal {
        std::vector<Data> sent;
        serial::ReceiveCallback receiver;

        result::Try send(serial::Data &&data) override {
                sent.push_back(data);
                return result::ok(true);
        }

        void on_receive(serial::ReceiveCallback callback) override {
                receiver = std::move(callback);
        }

        result::Try loop() override {
                return result::ok(true);
        }

        void deliver(serial::Data data) {
                if (receiver) {
                        receiver(std::move(data));
                }
        }
};

TEST_CASE("SerialTransporter send passes data to serial hal") {
        MockSerialHal mock;
        SerialTransporter st(mock, 64);

        Data data = {0x01, 0x02, 0x03};
        const auto result = st.send(std::move(data));

        REQUIRE(!result.failed());
        REQUIRE(mock.sent.size() == 1);
        REQUIRE(mock.sent[0] == Data{0x01, 0x02, 0x03});
}

TEST_CASE("SerialTransporter get_mtu returns configured mtu") {
        MockSerialHal mock;
        SerialTransporter st(mock, 128);

        REQUIRE(st.get_mtu() == 128);
}

TEST_CASE("SerialTransporter receive triggers callback") {
        MockSerialHal mock;
        SerialTransporter st(mock, 64);

        std::optional<Data> received;
        st.set_receiver([&](result::Result<Data> result) {
                if (!result.failed()) {
                        received = std::move(result).value();
                }
        });

        mock.deliver({0xDE, 0xAD, 0xBE, 0xEF});

        REQUIRE(received.has_value());
        REQUIRE(received.value() == Data{0xDE, 0xAD, 0xBE, 0xEF});
}

TEST_CASE("SerialTransporter receive with no callback set does not crash") {
        MockSerialHal mock;
        SerialTransporter st(mock, 64);

        REQUIRE_NOTHROW(mock.deliver({0x01}));
}

TEST_CASE("SerialTransporter send empty data") {
        MockSerialHal mock;
        SerialTransporter st(mock, 64);

        Data data = {};
        const auto result = st.send(std::move(data));

        REQUIRE(!result.failed());
        REQUIRE(mock.sent.size() == 1);
        REQUIRE(mock.sent[0].empty());
}

TEST_CASE("SerialTransporter send exceeding MTU returns error") {
        MockSerialHal mock;
        SerialTransporter st(mock, 4);

        Data data = {0x01, 0x02, 0x03, 0x04, 0x05};
        const auto result = st.send(std::move(data));

        REQUIRE(result.failed());
        REQUIRE(mock.sent.empty());
}

TEST_CASE("SerialTransporter send at exact MTU succeeds") {
        MockSerialHal mock;
        SerialTransporter st(mock, 4);

        Data data = {0x01, 0x02, 0x03, 0x04};
        const auto result = st.send(std::move(data));

        REQUIRE(!result.failed());
        REQUIRE(mock.sent.size() == 1);
}
