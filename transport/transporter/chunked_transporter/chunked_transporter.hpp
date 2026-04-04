#pragma once

#include <cstdio>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "packet.hpp"
#include "transporter/base_transporter.hpp"
#include "transporter/transporter.hpp"

namespace transport {
template <Transporter T> class ChunkedTransporter : public BaseTransporter {
      public:
        ChunkedTransporter(T &transporter, uint16_t max_tries)
            : transporter(transporter), max_tries(max_tries) {
                transporter.set_receiver([this](result::Result<Data> data) {
                        if (data.failed()) {
                                // TODO: Add log
                                return;
                        }
                        handle_data(std::move(data));
                });
        };

        result::Result<bool> send(Data &&data) override {
                Data packet;
                {
                        std::scoped_lock lock(egress_mutex);

                        const auto result = Chunk::fragment(
                            std::move(data), transporter.get_mtu());
                        if (result.failed()) {
                                return result::err(result.error());
                        }

                        chunked_payload = result.value();
                        if (chunked_payload.empty()) {
                                return result::err("chunks is empty");
                        }

                        const auto session_id = generate_session_id();
                        packet = chunked_payload[session_id].to_buf();
                }

                return transporter.send(std::move(packet));
        }

        MTU get_mtu() const override {
                const auto mtu = transporter.get_mtu();
                assert(mtu > Chunk::HEADER_SIZE &&
                       "mtu too small for chunking");

                return mtu - Chunk::HEADER_SIZE;
        }

      private:
        T &transporter;
        std::unordered_map<SessionId, std::vector<Chunk>> chunked_payload;
        std::unordered_map<SessionId, std::vector<Chunk>> received_chunks;
        uint16_t max_tries = 0;
        SessionId next_session_id = 0;
        std::mutex egress_mutex;
        std::mutex ingress_mutex;

        SessionId generate_session_id() {
                SessionId candidate = next_session_id;

                while (chunked_payload.find(candidate) !=
                       chunked_payload.end()) {
                        candidate++;
                }

                next_session_id = candidate + 1;
                return candidate;
        }

        void handle_data(Data &&data) {
                const auto type_result = get_packet_type(data);
                if (type_result.failed()) {
                        // TODO: Add log
                        return;
                }

                switch (type_result.value()) {
                case PacketType::ack:
                        handle_ack(std::move(data));
                        break;
                case PacketType::nack:
                        handle_nack(std::move(data));
                        break;
                case PacketType::chunk:
                        handle_chunk(std::move(data));
                        break;
                }
        }

        void send_nack(SessionId session_id, Indexer index) {
                Nack nack;
                nack.session_id = session_id;
                nack.index = index;
                const auto result = transporter.send(nack.to_buf());
                if (result.failed()) {
                        // TODO: Add log
                }
        }

        void send_ack(SessionId session_id, Indexer index) {
                Ack ack;
                ack.session_id = session_id;
                ack.index = index;
                const auto result = transporter.send(ack.to_buf());
                if (result.failed()) {
                        // TODO: Add log
                }
        }

        void handle_ack(Data &&data) {
        }

        void handle_nack(Data &&data) {
        }

        // TODO: Add max tries logic
        void handle_chunk(Data &&data) {
                const auto chunk_result = Chunk::from_buf(data);
                if (chunk_result.failed()) {
                        // TODO: Add log
                        return;
                }

                const auto chunk = chunk_result.value();
                const auto next_index = get_last_index() + 1;
                if (chunk.index != next_index) {
                        send_nack(chunk.session_id, next_index);
                        return;
                }

                const auto it = received_chunks.find(chunk.session_id);
                if (it == received_chunks.end()) {
                        std::vector<Chunk> chunks;
                        chunks.resize(chunk.total_chunks);
                        const auto idx = chunk.index;
                        chunks[idx] = std::move(chunk);
                        received_chunks[chunk.session_id] = std::move(chunks);
                        send_ack(chunk.session_id, next_index);
                        return;
                }

                auto &chunks = it->second;
                if (chunk.index >= chunks.size()) {
                        send_nack(chunk.session_id, next_index);
                        return;
                }
                chunks[chunk.index] = std::move(chunk);

                send_ack(chunk.session_id, next_index);
        }

        Indexer get_last_index(SessionId id) const {
                const auto it = received_chunks.find(id);
                if (it == received_chunks.end()) {
                        return 0;
                }

                const auto &chunks = it->second;
                if (chunks.empty()) {
                        return 0;
                }

                return chunks.back().index;
        }

        void handle_done(result::Result<Data> result) {
                const auto callback_result = try_callback(std::move(result));
                if (callback_result.failed()) {
                        // TODO: Add log
                }
        }
};
} // namespace transport
