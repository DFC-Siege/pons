#include <catch2/catch_test_macros.hpp>
#include <optional>
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

        result::Status send(Data &&data) {
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

TEST_CASE("ChunkedTransporter send fragments data into chunks") {
        MockTransporter mock;
        ChunkedTransporter<MockTransporter> ct(mock, 3);

        const Data payload(100, 0xAB);
        const auto result = ct.send(Data(payload));
        REQUIRE(!result.failed());
        REQUIRE(!mock.sent.empty());
}

TEST_CASE("ChunkedTransporter get_mtu subtracts header size") {
        MockTransporter mock;
        ChunkedTransporter<MockTransporter> ct(mock, 3);
        REQUIRE(ct.get_mtu() == mock.mtu - Chunk::HEADER_SIZE);
}

TEST_CASE("ChunkedTransporter send returns error on empty data") {
        MockTransporter mock;
        ChunkedTransporter<MockTransporter> ct(mock, 3);
        const auto result = ct.send({});
        REQUIRE(result.failed());
}

TEST_CASE("ChunkedTransporter receives and reassembles single chunk") {
        MockTransporter mock;
        std::optional<Data> received;
        ChunkedTransporter<MockTransporter> ct(mock, 3);
        ct.set_receiver([&](result::Result<Data> r) {
                if (!r.failed())
                        received = r.value();
        });

        const Data payload = {0x01, 0x02, 0x03};
        Chunk chunk;
        chunk.session_id = 1;
        chunk.index = 0;
        chunk.total_chunks = 1;
        chunk.payload = payload;
        chunk.checksum = crc16(payload);
        mock.deliver(chunk.to_buf());

        REQUIRE(received.has_value());
        REQUIRE(received.value() == payload);
}

TEST_CASE("ChunkedTransporter sends ack on receiving valid chunk") {
        MockTransporter mock;
        ChunkedTransporter<MockTransporter> ct(mock, 3);
        ct.set_receiver([](result::Result<Data>) {});

        const Data payload = {0xDE, 0xAD};
        Chunk chunk;
        chunk.session_id = 2;
        chunk.index = 0;
        chunk.total_chunks = 1;
        chunk.payload = payload;
        chunk.checksum = crc16(payload);
        mock.deliver(chunk.to_buf());

        REQUIRE(!mock.sent.empty());
        const auto type_result = get_packet_type(mock.sent.back());
        REQUIRE(!type_result.failed());
        REQUIRE(type_result.value() == PacketType::ack);
}

TEST_CASE("ChunkedTransporter sends nack on out of order chunk") {
        MockTransporter mock;
        ChunkedTransporter<MockTransporter> ct(mock, 3);
        ct.set_receiver([](result::Result<Data>) {});

        const Data payload = {0x01};
        Chunk chunk;
        chunk.session_id = 3;
        chunk.index = 1;
        chunk.total_chunks = 2;
        chunk.payload = payload;
        chunk.checksum = crc16(payload);
        mock.deliver(chunk.to_buf());

        REQUIRE(!mock.sent.empty());
        const auto type_result = get_packet_type(mock.sent.back());
        REQUIRE(!type_result.failed());
        REQUIRE(type_result.value() == PacketType::nack);
}
