#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <optional>
#include <thread>
#include <vector>

#include "chunked_transporter.hpp"

using namespace transport;
struct MockState {
        using ReceiverFn = std::function<void(result::Result<Data>)>;
        MTU mtu = 64;
        std::vector<Data> sent;
        ReceiverFn receiver;
        void deliver(Data data) {
                if (receiver) {
                        receiver(result::ok(std::move(data)));
                }
        }
};

struct MockTransporter {
        std::shared_ptr<MockState> state;
        MockTransporter(std::shared_ptr<MockState> s) : state(std::move(s)) {}
        void set_receiver(MockState::ReceiverFn fn) {
                state->receiver = std::move(fn);
        }
        result::Try send(Data &&data) {
                state->sent.push_back(data);
                return result::ok(true);
        }
        MTU get_mtu() const {
                return state->mtu;
        }
};

static Chunk make_chunk(SessionId session_id, Indexer index,
                        Indexer total_chunks, Data payload) {
        Chunk chunk;
        chunk.session_id = session_id;
        chunk.index = index;
        chunk.total_chunks = total_chunks;
        chunk.payload = payload;
        chunk.checksum = crc16(payload);
        return chunk;
}

static Ack make_ack(SessionId session_id, Indexer index) {
        Ack ack;
        ack.session_id = session_id;
        ack.index = index;
        return ack;
}

static Nack make_nack(SessionId session_id, Indexer index) {
        Nack nack;
        nack.session_id = session_id;
        nack.index = index;
        return nack;
}

TEST_CASE("ChunkedTransporter send fragments data into chunks") {
        auto s = std::make_shared<MockState>();
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));
        const Data payload(100, 0xAB);
        const auto result = ct.send(Data(payload));
        REQUIRE(!result.failed());
        REQUIRE(!s->sent.empty());
}

TEST_CASE("ChunkedTransporter get_mtu subtracts header size") {
        auto s = std::make_shared<MockState>();
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));
        REQUIRE(ct.get_mtu() == s->mtu - Chunk::HEADER_SIZE);
}

TEST_CASE("ChunkedTransporter send returns error on empty data") {
        auto s = std::make_shared<MockState>();
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));
        const auto result = ct.send({});
        REQUIRE(result.failed());
}

TEST_CASE("ChunkedTransporter receives and reassembles single chunk") {
        auto s = std::make_shared<MockState>();
        std::optional<Data> received;
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));
        ct.set_receiver([&](result::Result<Data> r) {
                if (!r.failed())
                        received = r.value();
        });
        const Data payload = {0x01, 0x02, 0x03};
        s->deliver(make_chunk(1, 0, 1, payload).to_buf());
        REQUIRE(received.has_value());
        REQUIRE(received.value() == payload);
}

TEST_CASE("ChunkedTransporter sends ack on receiving valid chunk") {
        auto s = std::make_shared<MockState>();
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));
        ct.set_receiver([](result::Result<Data>) {});
        s->deliver(make_chunk(2, 0, 1, {0xDE, 0xAD}).to_buf());
        REQUIRE(!s->sent.empty());
        const auto type_result = get_packet_type(s->sent.back());
        REQUIRE(!type_result.failed());
        REQUIRE(type_result.value() == PacketType::ack);
}

TEST_CASE("ChunkedTransporter sends nack on out of order chunk") {
        auto s = std::make_shared<MockState>();
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));
        ct.set_receiver([](result::Result<Data>) {});
        s->deliver(make_chunk(3, 1, 2, {0x01}).to_buf());
        REQUIRE(!s->sent.empty());
        const auto type_result = get_packet_type(s->sent.back());
        REQUIRE(!type_result.failed());
        REQUIRE(type_result.value() == PacketType::nack);
}

TEST_CASE("ChunkedTransporter reassembles multi-chunk message") {
        auto s = std::make_shared<MockState>();
        s->mtu = 16;
        std::optional<Data> received;
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));
        ct.set_receiver([&](result::Result<Data> r) {
                if (!r.failed())
                        received = r.value();
        });

        const Data part0 = {0x01, 0x02, 0x03};
        const Data part1 = {0x04, 0x05, 0x06};
        s->deliver(make_chunk(1, 0, 2, part0).to_buf());
        REQUIRE(!received.has_value());
        s->deliver(make_chunk(1, 1, 2, part1).to_buf());
        REQUIRE(received.has_value());

        Data expected;
        expected.insert(expected.end(), part0.begin(), part0.end());
        expected.insert(expected.end(), part1.begin(), part1.end());
        REQUIRE(received.value() == expected);
}

static SessionId extract_session_id(const Data &buf) {
        const auto result = Chunk::from_buf(buf);
        REQUIRE(!result.failed());
        return result.value().session_id;
}

TEST_CASE("ChunkedTransporter send triggers chunk-by-chunk flow via ack") {
        auto s = std::make_shared<MockState>();
        s->mtu = 16;
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));

        const Data payload(40, 0xCC);
        ct.send(Data(payload));
        REQUIRE(s->sent.size() == 1);

        const auto sid = extract_session_id(s->sent.front());

        s->deliver(make_ack(sid, 0).to_buf());
        REQUIRE(s->sent.size() == 2);

        s->deliver(make_ack(sid, 1).to_buf());
        REQUIRE(s->sent.size() == 3);
}

TEST_CASE("ChunkedTransporter retries chunk on nack") {
        auto s = std::make_shared<MockState>();
        s->mtu = 16;
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));

        const Data payload(40, 0xBB);
        ct.send(Data(payload));

        const auto sid = extract_session_id(s->sent.front());
        const size_t after_first = s->sent.size();

        s->deliver(make_nack(sid, 0).to_buf());
        REQUIRE(s->sent.size() == after_first + 1);

        const auto type_result = get_packet_type(s->sent.back());
        REQUIRE(!type_result.failed());
        REQUIRE(type_result.value() == PacketType::chunk);
}

TEST_CASE("ChunkedTransporter drops session after max_tries nacks") {
        auto s = std::make_shared<MockState>();
        s->mtu = 16;
        const uint16_t max_tries = 3;
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), max_tries,
                                               std::chrono::milliseconds(1000));

        const Data payload(40, 0xBB);
        ct.send(Data(payload));

        const auto sid = extract_session_id(s->sent.front());

        for (uint16_t i = 0; i < max_tries - 1; ++i) {
                s->deliver(make_nack(sid, 0).to_buf());
        }
        const size_t before_last = s->sent.size();

        s->deliver(make_nack(sid, 0).to_buf());
        REQUIRE(s->sent.size() == before_last);
}

TEST_CASE("ChunkedTransporter resets tries to zero on ack after nack") {
        auto s = std::make_shared<MockState>();
        s->mtu = 16;
        const uint16_t max_tries = 3;
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), max_tries,
                                               std::chrono::milliseconds(1000));

        const Data payload(40, 0xBB);
        ct.send(Data(payload));

        const auto sid = extract_session_id(s->sent.front());

        s->deliver(make_nack(sid, 0).to_buf());
        s->deliver(make_nack(sid, 0).to_buf());
        s->deliver(make_ack(sid, 0).to_buf());

        const size_t after_ack = s->sent.size();
        s->deliver(make_nack(sid, 1).to_buf());
        s->deliver(make_nack(sid, 1).to_buf());
        REQUIRE(s->sent.size() == after_ack + 2);

        s->deliver(make_nack(sid, 1).to_buf());
        REQUIRE(s->sent.size() == after_ack + 2);
}

TEST_CASE("ChunkedTransporter concurrent sessions are independent") {
        auto s = std::make_shared<MockState>();
        s->mtu = 16;
        std::vector<Data> received;
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));
        ct.set_receiver([&](result::Result<Data> r) {
                if (!r.failed())
                        received.push_back(r.value());
        });

        const Data payload_a = {0xAA};
        const Data payload_b = {0xBB};

        s->deliver(make_chunk(1, 0, 1, payload_a).to_buf());
        s->deliver(make_chunk(2, 0, 1, payload_b).to_buf());

        REQUIRE(received.size() == 2);
        REQUIRE(received[0] == payload_a);
        REQUIRE(received[1] == payload_b);
}

TEST_CASE("ChunkedTransporter nack for duplicate chunk index") {
        auto s = std::make_shared<MockState>();
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));
        ct.set_receiver([](result::Result<Data>) {});

        s->deliver(make_chunk(5, 0, 3, {0x01}).to_buf());
        s->sent.clear();

        s->deliver(make_chunk(5, 0, 3, {0x01}).to_buf());
        REQUIRE(!s->sent.empty());
        const auto type_result = get_packet_type(s->sent.back());
        REQUIRE(!type_result.failed());
        REQUIRE(type_result.value() == PacketType::nack);
}

TEST_CASE("ChunkedTransporter session is cleaned up after full send") {
        auto s = std::make_shared<MockState>();
        s->mtu = 16;
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));

        const Data payload(10, 0x11);
        ct.send(Data(payload));
        REQUIRE(!s->sent.empty());

        const auto sid = extract_session_id(s->sent.front());

        Indexer index = 0;
        while (!s->sent.empty()) {
                const auto chunk_result = Chunk::from_buf(s->sent.back());
                REQUIRE(!chunk_result.failed());
                index = chunk_result.value().index;
                s->sent.clear();
                s->deliver(make_ack(sid, index).to_buf());
        }

        s->deliver(make_ack(sid, index).to_buf());
        REQUIRE(s->sent.empty());
}

TEST_CASE("ChunkedTransporter send can reuse session id after completion") {
        auto s = std::make_shared<MockState>();
        s->mtu = 16;
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               std::chrono::milliseconds(1000));

        const Data payload(10, 0x22);
        ct.send(Data(payload));

        const auto sid = extract_session_id(s->sent.front());

        while (!s->sent.empty()) {
                const auto chunk_result = Chunk::from_buf(s->sent.back());
                REQUIRE(!chunk_result.failed());
                const auto index = chunk_result.value().index;
                s->sent.clear();
                s->deliver(make_ack(sid, index).to_buf());
        }

        const auto result = ct.send(Data(payload));
        REQUIRE(!result.failed());
}

TEST_CASE("ChunkedTransporter prunes stale egress session on timeout") {
        auto s = std::make_shared<MockState>();
        s->mtu = 16;
        const uint16_t timeout_ms = 50;
        ChunkedTransporter<MockTransporter> ct(
            std::make_unique<MockTransporter>(s), 3, std::chrono::milliseconds(timeout_ms));

        const Data payload(40, 0xAA);
        ct.send(Data(payload));
        REQUIRE(s->sent.size() == 1);
        const auto sid = extract_session_id(s->sent.front());

        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms + 10));

        ct.send({0x01});

        s->sent.clear();
        s->deliver(make_ack(sid, 0).to_buf());

        REQUIRE(s->sent.empty());
}

TEST_CASE("ChunkedTransporter prunes stale ingress session on timeout") {
        auto s = std::make_shared<MockState>();
        s->mtu = 16;
        const uint16_t timeout_ms = 50;
        ChunkedTransporter<MockTransporter> ct(
            std::make_unique<MockTransporter>(s), 3, std::chrono::milliseconds(timeout_ms));

        ct.set_receiver([&](result::Result<Data> r) {});

        const SessionId sid = 99;
        s->deliver(make_chunk(sid, 0, 2, {0x01, 0x02}).to_buf());

        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms + 10));

        s->deliver(make_chunk(100, 0, 1, {0xFF}).to_buf());

        s->sent.clear();
        s->deliver(make_chunk(sid, 1, 2, {0x03, 0x04}).to_buf());

        REQUIRE(!s->sent.empty());
        const auto type_result = get_packet_type(s->sent.back());
        REQUIRE(type_result.value() == PacketType::nack);
}

TEST_CASE("ChunkedTransporter does not prune sessions before timeout") {
        auto s = std::make_shared<MockState>();
        s->mtu = 16;
        const auto timeout_ms = std::chrono::milliseconds(1000);
        ChunkedTransporter<MockTransporter> ct(std::make_unique<MockTransporter>(s), 3,
                                               timeout_ms);

        const Data payload(40, 0xAA);
        ct.send(Data(payload));
        const auto sid = extract_session_id(s->sent.front());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        s->sent.clear();
        s->deliver(make_ack(sid, 0).to_buf());

        REQUIRE(s->sent.size() == 1);
        const auto chunk_result = Chunk::from_buf(s->sent.back());
        REQUIRE(chunk_result.value().index == 1);
}
