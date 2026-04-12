#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
#include <vector>

#include "serialized_requester.hpp"

using namespace transport;
using namespace std::chrono_literals;

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

static constexpr CommandId CMD_REQUEST = 0x0001;
static constexpr CommandId CMD_RESPONSE = 0x0002;
static constexpr TransporterId TID = 0x00;

struct TestFixture {
        Dispatcher<MockTransporter> dispatcher;
        SerializedRequester<MockTransporter> sr;
        MockTransporter *mock;

        TestFixture()
            : sr(std::make_unique<Requester<MockTransporter>>(dispatcher)) {
                auto owned = std::make_unique<MockTransporter>();
                mock = owned.get();
                dispatcher.register_transporter(TID, std::move(owned));
        }

        void deliver_response(SessionId session_id, uint32_t value,
                              bool success = true) {
                auto payload = TestPayload{value}.serialize();
                RequestWrapper wrapper{session_id, success, std::move(payload)};
                auto wrapped = WrappedData::wrap_data(
                    CMD_RESPONSE, RequestWrapper::to_data(std::move(wrapper)));
                mock->deliver(std::move(wrapped).value());
        }
};

TEST_CASE("SerializedRequester send_request serializes and forwards") {
        TestFixture f;

        auto handle_result = f.sr.send_request(TID, CMD_REQUEST, CMD_RESPONSE,
                                               TestPayload{42});
        REQUIRE(!handle_result.failed());
        REQUIRE(f.mock->sent.size() == 1);

        // Unwrap: command_id + request_wrapper(session_id + success + payload)
        auto unwrap = WrappedData::unwrap_data(Data(f.mock->sent[0]));
        REQUIRE(!unwrap.failed());
        REQUIRE(unwrap.value().command_id == CMD_REQUEST);

        auto req_wrapper =
            RequestWrapper::from_data(Data(unwrap.value().data));
        REQUIRE(!req_wrapper.failed());
        REQUIRE(req_wrapper.value().success == true);

        auto payload = TestPayload::deserialize(req_wrapper.value().data);
        REQUIRE(!payload.failed());
        REQUIRE(payload.value().value == 42);
}

TEST_CASE("SerializedRequester send_request to non-existent transporter "
           "fails") {
        TestFixture f;

        auto result = f.sr.send_request(0xFF, CMD_REQUEST, CMD_RESPONSE,
                                        TestPayload{42});
        REQUIRE(result.failed());
}

TEST_CASE("SerializedRequester await deserializes response") {
        TestFixture f;

        auto handle_result = f.sr.send_request(TID, CMD_REQUEST, CMD_RESPONSE,
                                               TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        std::thread responder([&] {
                std::this_thread::sleep_for(10ms);
                f.deliver_response(0, 99);
        });

        auto result = handle.await<TestPayload>(1000ms);
        responder.join();

        REQUIRE(!result.failed());
        REQUIRE(result.value().value == 99);
}

TEST_CASE("SerializedRequester await times out") {
        TestFixture f;

        auto handle_result = f.sr.send_request(TID, CMD_REQUEST, CMD_RESPONSE,
                                               TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        auto result = handle.await<TestPayload>(10ms);
        REQUIRE(result.failed());
}

TEST_CASE("SerializedRequester has_response before and after") {
        TestFixture f;

        auto handle_result = f.sr.send_request(TID, CMD_REQUEST, CMD_RESPONSE,
                                               TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        REQUIRE(!handle.has_response());

        f.deliver_response(0, 99);

        REQUIRE(handle.has_response());
}

TEST_CASE("SerializedRequester take_response deserializes data") {
        TestFixture f;

        auto handle_result = f.sr.send_request(TID, CMD_REQUEST, CMD_RESPONSE,
                                               TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        f.deliver_response(0, 77);

        auto result = handle.take_response<TestPayload>();
        REQUIRE(!result.failed());
        REQUIRE(result.value().value == 77);
}

TEST_CASE("SerializedRequester take_response returns error when none") {
        TestFixture f;

        auto handle_result = f.sr.send_request(TID, CMD_REQUEST, CMD_RESPONSE,
                                               TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        auto result = handle.take_response<TestPayload>();
        REQUIRE(result.failed());
}

TEST_CASE("SerializedRequester register_requestable handles request") {
        TestFixture f;

        f.sr.register_requestable<TestPayload, TestPayload>(
            CMD_REQUEST, CMD_RESPONSE, TID,
            [](TestPayload query) -> result::Result<TestPayload> {
                    return result::ok(TestPayload{query.value * 2});
            });

        // Simulate incoming request with session_id=0x42 and value=10
        RequestWrapper req{0x42, true, TestPayload{10}.serialize()};
        auto wrapped = WrappedData::wrap_data(
            CMD_REQUEST, RequestWrapper::to_data(std::move(req)));
        f.mock->deliver(std::move(wrapped).value());

        // Should have sent a response
        REQUIRE(f.mock->sent.size() == 1);

        auto unwrap = WrappedData::unwrap_data(Data(f.mock->sent[0]));
        REQUIRE(!unwrap.failed());
        REQUIRE(unwrap.value().command_id == CMD_RESPONSE);

        auto resp_wrapper =
            RequestWrapper::from_data(Data(unwrap.value().data));
        REQUIRE(!resp_wrapper.failed());
        REQUIRE(resp_wrapper.value().session_id == 0x42);
        REQUIRE(resp_wrapper.value().success == true);

        auto response = TestPayload::deserialize(resp_wrapper.value().data);
        REQUIRE(!response.failed());
        REQUIRE(response.value().value == 20);
}

TEST_CASE("SerializedRequester register_requestable sends error on handler "
           "failure") {
        TestFixture f;

        f.sr.register_requestable<TestPayload, TestPayload>(
            CMD_REQUEST, CMD_RESPONSE, TID,
            [](TestPayload) -> result::Result<TestPayload> {
                    return result::err("handler failed");
            });

        RequestWrapper req{0x10, true, TestPayload{5}.serialize()};
        auto wrapped = WrappedData::wrap_data(
            CMD_REQUEST, RequestWrapper::to_data(std::move(req)));
        f.mock->deliver(std::move(wrapped).value());

        REQUIRE(f.mock->sent.size() == 1);

        auto unwrap = WrappedData::unwrap_data(Data(f.mock->sent[0]));
        REQUIRE(!unwrap.failed());

        auto resp_wrapper =
            RequestWrapper::from_data(Data(unwrap.value().data));
        REQUIRE(!resp_wrapper.failed());
        REQUIRE(resp_wrapper.value().session_id == 0x10);
        REQUIRE(resp_wrapper.value().success == false);
}
