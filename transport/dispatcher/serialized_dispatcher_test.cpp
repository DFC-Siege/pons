#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

#include "serialized_dispatcher.hpp"

using namespace transport;

struct MockTransporter {
        MTU mtu = 64;
        std::vector<Data> sent;
        std::function<void(result::Result<Data>)> receiver;

        result::Try send(Data &&data) {
                sent.push_back(data);
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

struct TestPayload {
        uint32_t value;

        Data serialize() const {
                serializer::Writer w;
                w.write(value);
                return std::move(w.buf);
        }

        static result::Result<TestPayload> deserialize(serializer::DataView buf) {
                serializer::Reader r{buf};
                auto v = r.read<uint32_t>();
                if (v.failed())
                        return result::err(v.error());
                return result::ok(TestPayload{std::move(v).value()});
        }

        bool operator==(const TestPayload &) const = default;
};

static constexpr CommandId CMD_SEND = 0x0001;
static constexpr CommandId CMD_RECV = 0x0002;
static constexpr TransporterId TID = 0x00;

struct TestFixture {
        SerializedDispatcher<MockTransporter> sd;
        MockTransporter *mock;

        TestFixture()
            : sd(std::make_unique<Dispatcher<MockTransporter>>()) {
                auto owned = std::make_unique<MockTransporter>();
                mock = owned.get();
                sd.get_dispatcher().register_transporter(TID, std::move(owned));
        }
};

TEST_CASE("SerializedDispatcher send serializes and forwards data") {
        TestFixture f;

        auto result = f.sd.send(TID, CMD_SEND, TestPayload{42});
        REQUIRE(!result.failed());
        REQUIRE(f.mock->sent.size() == 1);

        // Unwrap and verify payload
        auto unwrap = WrappedData::unwrap_data(Data(f.mock->sent[0]));
        REQUIRE(!unwrap.failed());
        REQUIRE(unwrap.value().command_id == CMD_SEND);

        auto payload = TestPayload::deserialize(unwrap.value().data);
        REQUIRE(!payload.failed());
        REQUIRE(payload.value().value == 42);
}

TEST_CASE("SerializedDispatcher send to non-existent transporter fails") {
        TestFixture f;

        auto result = f.sd.send(0xFF, CMD_SEND, TestPayload{42});
        REQUIRE(result.failed());
}

TEST_CASE("SerializedDispatcher register_handler deserializes received data") {
        TestFixture f;

        std::optional<TestPayload> received;
        f.sd.register_handler<TestPayload>(
            CMD_RECV, [&](result::Result<TestPayload> result) {
                    if (!result.failed()) {
                            received = std::move(result).value();
                    }
            });

        auto payload = TestPayload{99}.serialize();
        auto wrapped = WrappedData::wrap_data(CMD_RECV, std::move(payload));
        f.mock->deliver(std::move(wrapped).value());

        REQUIRE(received.has_value());
        REQUIRE(received->value == 99);
}

TEST_CASE("SerializedDispatcher register_handler reports deserialization "
           "failure") {
        TestFixture f;

        bool got_error = false;
        f.sd.register_handler<TestPayload>(
            CMD_RECV, [&](result::Result<TestPayload> result) {
                    if (result.failed()) {
                            got_error = true;
                    }
            });

        // Payload too small to deserialize as TestPayload (needs 4 bytes)
        auto wrapped = WrappedData::wrap_data(CMD_RECV, Data{0x01});
        f.mock->deliver(std::move(wrapped).value());

        REQUIRE(got_error);
}

TEST_CASE("SerializedDispatcher send and receive round trip") {
        TestFixture f;

        std::optional<TestPayload> received;
        f.sd.register_handler<TestPayload>(
            CMD_SEND, [&](result::Result<TestPayload> result) {
                    if (!result.failed()) {
                            received = std::move(result).value();
                    }
            });

        auto send_result = f.sd.send(TID, CMD_SEND, TestPayload{123});
        REQUIRE(!send_result.failed());
        REQUIRE(f.mock->sent.size() == 1);

        // Feed the raw sent data back as if received
        f.mock->deliver(Data(f.mock->sent[0]));

        REQUIRE(received.has_value());
        REQUIRE(received->value == 123);
}

TEST_CASE("SerializedDispatcher get_dispatcher returns underlying dispatcher") {
        TestFixture f;

        auto &dispatcher = f.sd.get_dispatcher();

        std::optional<Data> received;
        dispatcher.register_handler(CMD_RECV, [&](result::Result<Data> result) {
                if (!result.failed()) {
                        received = std::move(result).value();
                }
        });

        auto wrapped = WrappedData::wrap_data(CMD_RECV, Data{0xAA, 0xBB});
        f.mock->deliver(std::move(wrapped).value());

        REQUIRE(received.has_value());
        REQUIRE(received.value() == Data{0xAA, 0xBB});
}

TEST_CASE("SerializedDispatcher dispatches to correct handler by command id") {
        TestFixture f;

        std::optional<TestPayload> received_a;
        std::optional<TestPayload> received_b;
        f.sd.register_handler<TestPayload>(
            CMD_SEND, [&](result::Result<TestPayload> result) {
                    if (!result.failed())
                            received_a = std::move(result).value();
            });
        f.sd.register_handler<TestPayload>(
            CMD_RECV, [&](result::Result<TestPayload> result) {
                    if (!result.failed())
                            received_b = std::move(result).value();
            });

        auto payload = TestPayload{55}.serialize();
        auto wrapped = WrappedData::wrap_data(CMD_RECV, std::move(payload));
        f.mock->deliver(std::move(wrapped).value());

        REQUIRE(!received_a.has_value());
        REQUIRE(received_b.has_value());
        REQUIRE(received_b->value == 55);
}
