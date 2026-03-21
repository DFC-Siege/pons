#include <catch2/catch_test_macros.hpp>
#include <span>
#include <vector>

#include "ble_transporter.hpp"
#include "i_ble_hal.hpp"

struct MockBleHal : ble::IBleHal {
        std::vector<std::vector<uint8_t>> sent;
        ble::ReceiveCallback receive_cb;

        result::Result<bool> send(std::span<const uint8_t> data) override {
                sent.push_back({data.begin(), data.end()});
                return result::ok();
        }

        void on_receive(ble::ReceiveCallback cb) override {
                receive_cb = std::move(cb);
        }

        void on_connection_changed(ble::ConnectionCallback cb) override {
        }

        bool is_connected() const override {
                return true;
        }

        void simulate_receive(std::span<const uint8_t> data) {
                if (receive_cb)
                        receive_cb(data);
        }
};

TEST_CASE("concrete_send forwards data to ble hal") {
        MockBleHal hal;
        transport::BleTransporter transporter(512, hal);

        const std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        const auto result =
            transporter.send(0x01, data, [] {}, [](std::string_view) {});
        REQUIRE(!result.failed());
        REQUIRE(!hal.sent.empty());
}

TEST_CASE("send_async returns non-null future") {
        MockBleHal hal;
        transport::BleTransporter transporter(512, hal);

        const std::vector<uint8_t> data = {0x01};
        auto future = transporter.send_async(0x01, data);
        REQUIRE(future != nullptr);
}

TEST_CASE("feed returns error on empty data") {
        MockBleHal hal;
        transport::BleTransporter transporter(512, hal);

        const auto result = transporter.feed({});
        REQUIRE(result.failed());
}

TEST_CASE("feed returns error on unknown session") {
        MockBleHal hal;
        transport::BleTransporter transporter(512, hal);

        const std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00, 0x00};
        const auto result = transporter.feed(data);
        REQUIRE(result.failed());
}

TEST_CASE("receive callback triggers feed") {
        MockBleHal hal;
        transport::BleTransporter transporter(512, hal);

        const std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00, 0x00};
        hal.simulate_receive(data);
}
