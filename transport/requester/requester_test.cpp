#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
#include <vector>

#include "requester.hpp"

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
                Data buf;
                detail::push_le(buf, value);
                return buf;
        }

        static result::Result<TestPayload> deserialize(DataView buf) {
                if (buf.size() < sizeof(uint32_t)) {
                        return result::err("too small");
                }
                return result::ok(
                    TestPayload{detail::pull_le<uint32_t>(buf, 0)});
        }

        bool operator==(const TestPayload &) const = default;
};

static constexpr CommandId CMD_REQUEST = 0x0001;
static constexpr CommandId CMD_RESPONSE = 0x0002;
static constexpr TransporterId TID = 0x00;

struct TestFixture {
        Dispatcher<MockTransporter> dispatcher;
        Requester<MockTransporter> requester{dispatcher};
        MockTransporter *mock;

        TestFixture() {
                auto owned = std::make_unique<MockTransporter>();
                mock = owned.get();
                dispatcher.register_transporter(TID, std::move(owned));
        }

        void deliver_response(SessionId session_id, uint32_t value,
                              bool success = true) {
                Data payload;
                detail::push_le(payload, value);
                RequestWrapper wrapper{session_id, success, std::move(payload)};
                auto wrapped = WrappedData::wrap_data(
                    CMD_RESPONSE, RequestWrapper::to_data(std::move(wrapper)));
                mock->deliver(std::move(wrapped).value());
        }
};

TEST_CASE("RequestWrapper round trips") {
        RequestWrapper original{0x12345678, true, {0xAA, 0xBB, 0xCC}};
        auto data = RequestWrapper::to_data(std::move(original));

        REQUIRE(data.size() == sizeof(SessionId) + 1 + 3);

        auto result = RequestWrapper::from_data(std::move(data));
        REQUIRE(!result.failed());
        REQUIRE(result.value().session_id == 0x12345678);
        REQUIRE(result.value().success == true);
        REQUIRE(result.value().data == Data{0xAA, 0xBB, 0xCC});
}

TEST_CASE("RequestWrapper from_data fails on too small data") {
        Data small = {0x01, 0x02, 0x03, 0x04};
        auto result = RequestWrapper::from_data(std::move(small));
        REQUIRE(result.failed());
}

TEST_CASE("RequestWrapper from_data fails on empty data") {
        auto result = RequestWrapper::from_data({});
        REQUIRE(result.failed());
}

TEST_CASE("send_request sends wrapped data through dispatcher") {
        TestFixture f;

        auto handle_result = f.requester.send_request(
            TID, CMD_REQUEST, CMD_RESPONSE, TestPayload{42});
        REQUIRE(!handle_result.failed());
        REQUIRE(f.mock->sent.size() == 1);

        // Sent data should be: command_id(2) + session_id(4) + status(1) + payload(4)
        REQUIRE(f.mock->sent[0].size() ==
                sizeof(CommandId) + sizeof(SessionId) + 1 + sizeof(uint32_t));
}

TEST_CASE("send_request to non-existent transporter returns error") {
        TestFixture f;

        auto result = f.requester.send_request(0xFF, CMD_REQUEST, CMD_RESPONSE,
                                               TestPayload{42});
        REQUIRE(result.failed());
}

TEST_CASE("await returns response on success") {
        TestFixture f;

        auto handle_result = f.requester.send_request(
            TID, CMD_REQUEST, CMD_RESPONSE, TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        std::thread responder([&] {
                std::this_thread::sleep_for(10ms);
                f.deliver_response(0, 99);
        });

        auto result = handle->await<TestPayload>(1000ms);
        responder.join();

        REQUIRE(!result.failed());
        REQUIRE(result.value().value == 99);
}

TEST_CASE("await times out when no response") {
        TestFixture f;

        auto handle_result = f.requester.send_request(
            TID, CMD_REQUEST, CMD_RESPONSE, TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        auto result = handle->await<TestPayload>(10ms);
        REQUIRE(result.failed());
}

TEST_CASE("multiple concurrent requests resolve independently") {
        TestFixture f;

        auto h1_result = f.requester.send_request(TID, CMD_REQUEST,
                                                  CMD_RESPONSE, TestPayload{1});
        auto h2_result = f.requester.send_request(TID, CMD_REQUEST,
                                                  CMD_RESPONSE, TestPayload{2});
        REQUIRE(!h1_result.failed());
        REQUIRE(!h2_result.failed());
        auto h1 = std::move(h1_result).value();
        auto h2 = std::move(h2_result).value();

        std::thread responder([&] {
                std::this_thread::sleep_for(10ms);
                // Respond to second request first
                f.deliver_response(1, 200);
                std::this_thread::sleep_for(10ms);
                f.deliver_response(0, 100);
        });

        auto r1 = h1->await<TestPayload>(1000ms);
        auto r2 = h2->await<TestPayload>(1000ms);
        responder.join();

        REQUIRE(!r1.failed());
        REQUIRE(!r2.failed());
        REQUIRE(r1.value().value == 100);
        REQUIRE(r2.value().value == 200);
}

TEST_CASE("has_response returns false before response arrives") {
        TestFixture f;

        auto handle_result = f.requester.send_request(
            TID, CMD_REQUEST, CMD_RESPONSE, TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        REQUIRE(!handle->has_response());
}

TEST_CASE("has_response returns true after response arrives") {
        TestFixture f;

        auto handle_result = f.requester.send_request(
            TID, CMD_REQUEST, CMD_RESPONSE, TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        f.deliver_response(0, 99);

        REQUIRE(handle->has_response());
}

TEST_CASE("take_response returns data after response arrives") {
        TestFixture f;

        auto handle_result = f.requester.send_request(
            TID, CMD_REQUEST, CMD_RESPONSE, TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        f.deliver_response(0, 99);

        auto result = handle->take_response<TestPayload>();
        REQUIRE(!result.failed());
        REQUIRE(result.value().value == 99);
}

TEST_CASE("take_response returns error when no response") {
        TestFixture f;

        auto handle_result = f.requester.send_request(
            TID, CMD_REQUEST, CMD_RESPONSE, TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        auto result = handle->take_response<TestPayload>();
        REQUIRE(result.failed());
}

TEST_CASE("response to unknown session id does not crash") {
        TestFixture f;

        auto handle_result = f.requester.send_request(
            TID, CMD_REQUEST, CMD_RESPONSE, TestPayload{42});
        REQUIRE(!handle_result.failed());

        REQUIRE_NOTHROW(f.deliver_response(999, 0));
}

TEST_CASE("session ids increment across requests") {
        TestFixture f;

        f.requester.send_request(TID, CMD_REQUEST, CMD_RESPONSE,
                                 TestPayload{1});
        f.requester.send_request(TID, CMD_REQUEST, CMD_RESPONSE,
                                 TestPayload{2});

        REQUIRE(f.mock->sent.size() == 2);

        // Extract session_id from each sent message (after command_id bytes)
        auto sid0 =
            detail::pull_le<SessionId>(f.mock->sent[0], sizeof(CommandId));
        auto sid1 =
            detail::pull_le<SessionId>(f.mock->sent[1], sizeof(CommandId));
        REQUIRE(sid0 != sid1);
}

TEST_CASE("register_requestable handles request and sends response") {
        TestFixture f;

        f.requester.register_requestable<TestPayload, TestPayload>(
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

        // Unwrap: command_id + session_id + payload
        auto &sent = f.mock->sent[0];
        auto unwrap = WrappedData::unwrap_data(Data(sent));
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

TEST_CASE("register_requestable end to end with send_request") {
        TestFixture f;

        // Register handler that doubles value
        f.requester.register_requestable<TestPayload, TestPayload>(
            CMD_REQUEST, CMD_RESPONSE, TID,
            [](TestPayload query) -> result::Result<TestPayload> {
                    return result::ok(TestPayload{query.value * 3});
            });

        // Send request
        auto handle_result = f.requester.send_request(
            TID, CMD_REQUEST, CMD_RESPONSE, TestPayload{7});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        // Grab what was sent, feed response back through mock
        // sent[0] is the outgoing request — extract session_id from it
        auto outgoing = WrappedData::unwrap_data(Data(f.mock->sent[0]));
        auto req_wrapper =
            RequestWrapper::from_data(Data(outgoing.value().data));
        auto session_id = req_wrapper.value().session_id;

        // Simulate the other side's requestable handler by delivering response
        f.deliver_response(session_id, 21);

        auto result = handle->await<TestPayload>(100ms);
        REQUIRE(!result.failed());
        REQUIRE(result.value().value == 21);
}

TEST_CASE("destroying requester unblocks pending awaits") {
        Dispatcher<MockTransporter> dispatcher;
        auto owned = std::make_unique<MockTransporter>();
        dispatcher.register_transporter(TID, std::move(owned));

        std::shared_ptr<RequestHandle<>> handle;
        {
                Requester<MockTransporter> requester(dispatcher);
                auto handle_result = requester.send_request(
                    TID, CMD_REQUEST, CMD_RESPONSE, TestPayload{42});
                REQUIRE(!handle_result.failed());
                handle = std::move(handle_result).value();
        } // requester destroyed here

        // Should already have a response (error)
        REQUIRE(handle->has_response());
        auto result = handle->await<TestPayload>(0ms);
        REQUIRE(result.failed());
}

TEST_CASE("register_requestable sends error response on handler failure") {
        TestFixture f;

        f.requester.register_requestable<TestPayload, TestPayload>(
            CMD_REQUEST, CMD_RESPONSE, TID,
            [](TestPayload) -> result::Result<TestPayload> {
                    return result::err("something went wrong");
            });

        // Send request
        auto handle_result = f.requester.send_request(
            TID, CMD_REQUEST, CMD_RESPONSE, TestPayload{42});
        REQUIRE(!handle_result.failed());
        auto handle = std::move(handle_result).value();

        // Extract session_id from outgoing request
        auto outgoing = WrappedData::unwrap_data(Data(f.mock->sent[0]));
        auto req_wrapper =
            RequestWrapper::from_data(Data(outgoing.value().data));
        auto session_id = req_wrapper.value().session_id;

        // Simulate incoming request that triggers the error handler
        Data request_data;
        detail::push_le(request_data, session_id);
        detail::push_le(request_data, uint32_t{42});
        auto request_wrapped =
            WrappedData::wrap_data(CMD_REQUEST, std::move(request_data));

        // The requestable handler sends error response — which should be
        // picked up by the response handler. But the requestable is on
        // CMD_REQUEST, so we need a separate requester-side test.
        // Instead, test that the mock received an error response.
        f.mock->sent.clear();

        RequestWrapper req{session_id, true, TestPayload{42}.serialize()};
        auto wrapped =
            WrappedData::wrap_data(CMD_REQUEST, RequestWrapper::to_data(std::move(req)));
        f.mock->deliver(std::move(wrapped).value());

        // Should have sent a response with success=false
        REQUIRE(f.mock->sent.size() == 1);
        auto resp_unwrap = WrappedData::unwrap_data(Data(f.mock->sent[0]));
        REQUIRE(!resp_unwrap.failed());
        REQUIRE(resp_unwrap.value().command_id == CMD_RESPONSE);

        auto resp_wrapper =
            RequestWrapper::from_data(Data(resp_unwrap.value().data));
        REQUIRE(!resp_wrapper.failed());
        REQUIRE(resp_wrapper.value().session_id == session_id);
        REQUIRE(resp_wrapper.value().success == false);
}
