#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <thread>
#include <vector>

#include "chunked_transporter.hpp"

using namespace transport;
struct MockTransporter {
        using ReceiverFn = std::function<void(result::Result<Data>)>;
        MTU mtu = 64;
        std::vector<Data> sent;
        ReceiverFn receiver;
        void set_receiver(ReceiverFn fn) {
                receiver = std::move(fn);
        }
        result::Try send(Data &&data) {
                sent.push_back(data);
                return result::ok(true);
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
        MockTransporter mock;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);
        const Data payload(100, 0xAB);
        const auto result = ct.send(Data(payload));
        REQUIRE(!result.failed());
        REQUIRE(!mock.sent.empty());
}

TEST_CASE("ChunkedTransporter get_mtu subtracts header size") {
        MockTransporter mock;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);
        REQUIRE(ct.get_mtu() == mock.mtu - Chunk::HEADER_SIZE);
}

TEST_CASE("ChunkedTransporter send returns error on empty data") {
        MockTransporter mock;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);
        const auto result = ct.send({});
        REQUIRE(result.failed());
}

TEST_CASE("ChunkedTransporter receives and reassembles single chunk") {
        MockTransporter mock;
        std::optional<Data> received;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);
        ct.set_receiver([&](result::Result<Data> r) {
                if (!r.failed())
                        received = r.value();
        });
        const Data payload = {0x01, 0x02, 0x03};
        mock.deliver(make_chunk(1, 0, 1, payload).to_buf());
        REQUIRE(received.has_value());
        REQUIRE(received.value() == payload);
}

TEST_CASE("ChunkedTransporter sends ack on receiving valid chunk") {
        MockTransporter mock;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);
        ct.set_receiver([](result::Result<Data>) {});
        mock.deliver(make_chunk(2, 0, 1, {0xDE, 0xAD}).to_buf());
        REQUIRE(!mock.sent.empty());
        const auto type_result = get_packet_type(mock.sent.back());
        REQUIRE(!type_result.failed());
        REQUIRE(type_result.value() == PacketType::ack);
}

TEST_CASE("ChunkedTransporter sends nack on out of order chunk") {
        MockTransporter mock;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);
        ct.set_receiver([](result::Result<Data>) {});
        mock.deliver(make_chunk(3, 1, 2, {0x01}).to_buf());
        REQUIRE(!mock.sent.empty());
        const auto type_result = get_packet_type(mock.sent.back());
        REQUIRE(!type_result.failed());
        REQUIRE(type_result.value() == PacketType::nack);
}

TEST_CASE("ChunkedTransporter reassembles multi-chunk message") {
        MockTransporter mock;
        mock.mtu = 16;
        std::optional<Data> received;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);
        ct.set_receiver([&](result::Result<Data> r) {
                if (!r.failed())
                        received = r.value();
        });

        const Data part0 = {0x01, 0x02, 0x03};
        const Data part1 = {0x04, 0x05, 0x06};
        mock.deliver(make_chunk(1, 0, 2, part0).to_buf());
        REQUIRE(!received.has_value());
        mock.deliver(make_chunk(1, 1, 2, part1).to_buf());
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
        MockTransporter mock;
        mock.mtu = 16;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);

        const Data payload(40, 0xCC);
        ct.send(Data(payload));
        REQUIRE(mock.sent.size() == 1);

        const auto sid = extract_session_id(mock.sent.front());

        mock.deliver(make_ack(sid, 0).to_buf());
        REQUIRE(mock.sent.size() == 2);

        mock.deliver(make_ack(sid, 1).to_buf());
        REQUIRE(mock.sent.size() == 3);
}

TEST_CASE("ChunkedTransporter retries chunk on nack") {
        MockTransporter mock;
        mock.mtu = 16;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);

        const Data payload(40, 0xBB);
        ct.send(Data(payload));

        const auto sid = extract_session_id(mock.sent.front());
        const size_t after_first = mock.sent.size();

        mock.deliver(make_nack(sid, 0).to_buf());
        REQUIRE(mock.sent.size() == after_first + 1);

        const auto type_result = get_packet_type(mock.sent.back());
        REQUIRE(!type_result.failed());
        REQUIRE(type_result.value() == PacketType::chunk);
}

TEST_CASE("ChunkedTransporter drops session after max_tries nacks") {
        MockTransporter mock;
        mock.mtu = 16;
        const uint16_t max_tries = 3;
        ChunkedTransporter<MockTransporter> ct(mock, max_tries, 1000);

        const Data payload(40, 0xBB);
        ct.send(Data(payload));

        const auto sid = extract_session_id(mock.sent.front());

        for (uint16_t i = 0; i < max_tries - 1; ++i) {
                mock.deliver(make_nack(sid, 0).to_buf());
        }
        const size_t before_last = mock.sent.size();

        mock.deliver(make_nack(sid, 0).to_buf());
        REQUIRE(mock.sent.size() == before_last);
}

TEST_CASE("ChunkedTransporter resets tries to zero on ack after nack") {
        MockTransporter mock;
        mock.mtu = 16;
        const uint16_t max_tries = 3;
        ChunkedTransporter<MockTransporter> ct(mock, max_tries, 1000);

        const Data payload(40, 0xBB);
        ct.send(Data(payload));

        const auto sid = extract_session_id(mock.sent.front());

        mock.deliver(make_nack(sid, 0).to_buf());
        mock.deliver(make_nack(sid, 0).to_buf());
        mock.deliver(make_ack(sid, 0).to_buf());

        const size_t after_ack = mock.sent.size();
        mock.deliver(make_nack(sid, 1).to_buf());
        mock.deliver(make_nack(sid, 1).to_buf());
        REQUIRE(mock.sent.size() == after_ack + 2);

        mock.deliver(make_nack(sid, 1).to_buf());
        REQUIRE(mock.sent.size() == after_ack + 2);
}

TEST_CASE("ChunkedTransporter concurrent sessions are independent") {
        MockTransporter mock;
        mock.mtu = 16;
        std::vector<Data> received;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);
        ct.set_receiver([&](result::Result<Data> r) {
                if (!r.failed())
                        received.push_back(r.value());
        });

        const Data payload_a = {0xAA};
        const Data payload_b = {0xBB};

        mock.deliver(make_chunk(1, 0, 1, payload_a).to_buf());
        mock.deliver(make_chunk(2, 0, 1, payload_b).to_buf());

        REQUIRE(received.size() == 2);
        REQUIRE(received[0] == payload_a);
        REQUIRE(received[1] == payload_b);
}

TEST_CASE("ChunkedTransporter nack for duplicate chunk index") {
        MockTransporter mock;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);
        ct.set_receiver([](result::Result<Data>) {});

        mock.deliver(make_chunk(5, 0, 3, {0x01}).to_buf());
        mock.sent.clear();

        mock.deliver(make_chunk(5, 0, 3, {0x01}).to_buf());
        REQUIRE(!mock.sent.empty());
        const auto type_result = get_packet_type(mock.sent.back());
        REQUIRE(!type_result.failed());
        REQUIRE(type_result.value() == PacketType::nack);
}

TEST_CASE("ChunkedTransporter session is cleaned up after full send") {
        MockTransporter mock;
        mock.mtu = 16;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);

        const Data payload(10, 0x11);
        ct.send(Data(payload));
        REQUIRE(!mock.sent.empty());

        const auto sid = extract_session_id(mock.sent.front());

        Indexer index = 0;
        while (!mock.sent.empty()) {
                const auto chunk_result = Chunk::from_buf(mock.sent.back());
                REQUIRE(!chunk_result.failed());
                index = chunk_result.value().index;
                mock.sent.clear();
                mock.deliver(make_ack(sid, index).to_buf());
        }

        mock.deliver(make_ack(sid, index).to_buf());
        REQUIRE(mock.sent.empty());
}

TEST_CASE("ChunkedTransporter send can reuse session id after completion") {
        MockTransporter mock;
        mock.mtu = 16;
        ChunkedTransporter<MockTransporter> ct(mock, 3, 1000);

        const Data payload(10, 0x22);
        ct.send(Data(payload));

        const auto sid = extract_session_id(mock.sent.front());

        while (!mock.sent.empty()) {
                const auto chunk_result = Chunk::from_buf(mock.sent.back());
                REQUIRE(!chunk_result.failed());
                const auto index = chunk_result.value().index;
                mock.sent.clear();
                mock.deliver(make_ack(sid, index).to_buf());
        }

        const auto result = ct.send(Data(payload));
        REQUIRE(!result.failed());
}

TEST_CASE("ChunkedTransporter prunes stale egress session on timeout") {
        MockTransporter mock;
        mock.mtu = 16;
        const uint16_t timeout_ms = 50;
        ChunkedTransporter<MockTransporter> ct(mock, 3, timeout_ms);

        const Data payload(40, 0xAA);
        ct.send(Data(payload));
        REQUIRE(mock.sent.size() == 1);
        const auto sid = extract_session_id(mock.sent.front());

        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms + 10));

        ct.send({0x01});

        mock.sent.clear();
        mock.deliver(make_ack(sid, 0).to_buf());

        REQUIRE(mock.sent.empty());
}

TEST_CASE("ChunkedTransporter prunes stale ingress session on timeout") {
        MockTransporter mock;
        mock.mtu = 16;
        const uint16_t timeout_ms = 50;
        ChunkedTransporter<MockTransporter> ct(mock, 3, timeout_ms);

        ct.set_receiver([&](result::Result<Data> r) {});

        const SessionId sid = 99;
        mock.deliver(make_chunk(sid, 0, 2, {0x01, 0x02}).to_buf());

        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms + 10));

        mock.deliver(make_chunk(100, 0, 1, {0xFF}).to_buf());

        mock.sent.clear();
        mock.deliver(make_chunk(sid, 1, 2, {0x03, 0x04}).to_buf());

        REQUIRE(!mock.sent.empty());
        const auto type_result = get_packet_type(mock.sent.back());
        REQUIRE(type_result.value() == PacketType::nack);
}

TEST_CASE("ChunkedTransporter does not prune sessions before timeout") {
        MockTransporter mock;
        mock.mtu = 16;
        const uint16_t timeout_ms = 1000;
        ChunkedTransporter<MockTransporter> ct(mock, 3, timeout_ms);

        const Data payload(40, 0xAA);
        ct.send(Data(payload));
        const auto sid = extract_session_id(mock.sent.front());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        mock.sent.clear();
        mock.deliver(make_ack(sid, 0).to_buf());

        REQUIRE(mock.sent.size() == 1);
        const auto chunk_result = Chunk::from_buf(mock.sent.back());
        REQUIRE(chunk_result.value().index == 1);
}
