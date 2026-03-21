#include <catch2/catch_test_macros.hpp>
#include <span>
#include <vector>

#include "i_serial_hal.hpp"
#include "serial_transporter.hpp"

struct MockSerialHal : serial::ISerialHal {
        std::vector<std::vector<uint8_t>> sent;
        serial::ReceiveCallback receive_cb;

        result::Result<bool> send(std::span<const uint8_t> data) override {
                sent.push_back({data.begin(), data.end()});
                return result::ok();
        }

        void on_receive(serial::ReceiveCallback cb) override {
                receive_cb = std::move(cb);
        }

        result::Result<bool> loop() override {
                return result::ok();
        }

        void simulate_receive(std::span<const uint8_t> data) {
                if (receive_cb)
                        receive_cb(data);
        }
};

TEST_CASE("concrete_send forwards data to serial hal") {
        MockSerialHal hal;
        transport::SerialTransporter transporter(512, 1, hal);

        const std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        const auto result =
            transporter.send(0x01, data, [] {}, [](std::string_view) {});
        REQUIRE(!result.failed());
        REQUIRE(!hal.sent.empty());
}

TEST_CASE("send_async returns non-null future") {
        MockSerialHal hal;
        transport::SerialTransporter transporter(512, 1, hal);

        const std::vector<uint8_t> data = {0x01};
        auto future = transporter.send_async(0x01, data);
        REQUIRE(future != nullptr);
}

TEST_CASE("feed returns error on empty data") {
        MockSerialHal hal;
        transport::SerialTransporter transporter(512, 1, hal);

        const auto result = transporter.feed({});
        REQUIRE(result.failed());
}

TEST_CASE("feed returns error on unknown session") {
        MockSerialHal hal;
        transport::SerialTransporter transporter(512, 1, hal);

        const std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00, 0x00};
        const auto result = transporter.feed(data);
        REQUIRE(result.failed());
}

TEST_CASE("receive callback triggers feed") {
        MockSerialHal hal;
        transport::SerialTransporter transporter(512, 1, hal);

        const std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00, 0x00};
        hal.simulate_receive(data);
}
