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

        // Simulate response: wrap response_command_id + session_id + payload
        void deliver_response(SessionId session_id, uint32_t value) {
                Data response_data;
                detail::push_le(response_data, session_id);
                detail::push_le(response_data, value);

                auto wrapped = WrappedData::wrap_data(CMD_RESPONSE,
                                                      std::move(response_data));
                mock->deliver(std::move(wrapped).value());
        }
};

TEST_CASE("RequestWrapper round trips") {
        RequestWrapper original{0x12345678, {0xAA, 0xBB, 0xCC}};
        auto data = RequestWrapper::to_data(std::move(original));

        REQUIRE(data.size() == sizeof(SessionId) + 3);

        auto result = RequestWrapper::from_data(std::move(data));
        REQUIRE(!result.failed());
        REQUIRE(result.value().session_id == 0x12345678);
        REQUIRE(result.value().data == Data{0xAA, 0xBB, 0xCC});
}

TEST_CASE("RequestWrapper from_data fails on too small data") {
        Data small = {0x01, 0x02};
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

        // Sent data should be: command_id(2) + session_id(4) + payload(4)
        REQUIRE(f.mock->sent[0].size() ==
                sizeof(CommandId) + sizeof(SessionId) + sizeof(uint32_t));
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
        Data request_data;
        detail::push_le(request_data, SessionId{0x42});
        detail::push_le(request_data, uint32_t{10});
        auto wrapped = WrappedData::wrap_data(CMD_REQUEST, std::move(request_data));
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
        Data response_data;
        detail::push_le(response_data, session_id);
        detail::push_le(response_data, uint32_t{21});
        auto wrapped =
            WrappedData::wrap_data(CMD_RESPONSE, std::move(response_data));
        f.mock->deliver(std::move(wrapped).value());

        auto result = handle->await<TestPayload>(100ms);
        REQUIRE(!result.failed());
        REQUIRE(result.value().value == 21);
}
