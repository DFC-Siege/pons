#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

#include "base_transporter.hpp"

using namespace transport;

struct ConcreteTransporter : public BaseTransporter {
        MTU mtu = 64;
        std::vector<Data> sent;

        result::Try send(Data &&data) override {
                sent.push_back(std::move(data));
                return result::ok(true);
        }

        MTU get_mtu() const override {
                return mtu;
        }

        result::Try emit(result::Result<Data> data) {
                return try_callback(std::move(data));
        }
};

TEST_CASE("BaseTransporter try_callback returns error when no callback set") {
        ConcreteTransporter ct;
        const auto result = ct.emit(result::ok(Data{0x01}));
        REQUIRE(result.failed());
}

TEST_CASE("BaseTransporter set_receiver and try_callback invokes callback") {
        ConcreteTransporter ct;
        std::optional<Data> received;
        ct.set_receiver([&](result::Result<Data> result) {
                if (!result.failed()) {
                        received = std::move(result).value();
                }
        });

        const auto result = ct.emit(result::ok(Data{0x01, 0x02, 0x03}));

        REQUIRE(!result.failed());
        REQUIRE(received.has_value());
        REQUIRE(received.value() == Data{0x01, 0x02, 0x03});
}

TEST_CASE("BaseTransporter try_callback forwards failed result to callback") {
        ConcreteTransporter ct;
        std::optional<std::string> error;
        ct.set_receiver([&](result::Result<Data> result) {
                if (result.failed()) {
                        error = std::string(result.error());
                }
        });

        const auto result = ct.emit(result::err("something failed"));

        REQUIRE(!result.failed());
        REQUIRE(error.has_value());
        REQUIRE(error.value() == "something failed");
}

TEST_CASE("BaseTransporter set_receiver replaces previous callback") {
        ConcreteTransporter ct;
        int call_count = 0;
        ct.set_receiver([&](result::Result<Data>) { call_count++; });
        ct.set_receiver([&](result::Result<Data>) { call_count += 10; });

        ct.emit(result::ok(Data{0x01}));

        REQUIRE(call_count == 10);
}

TEST_CASE("BaseTransporter send stores data") {
        ConcreteTransporter ct;
        Data data = {0xDE, 0xAD};
        const auto result = ct.send(std::move(data));
        REQUIRE(!result.failed());
        REQUIRE(ct.sent.size() == 1);
        REQUIRE(ct.sent[0] == Data{0xDE, 0xAD});
}

TEST_CASE("BaseTransporter get_mtu returns configured value") {
        ConcreteTransporter ct;
        ct.mtu = 128;
        REQUIRE(ct.get_mtu() == 128);
}
