#pragma once

#include <cstdio>
#include <functional>
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

                        auto chunks = result.value();
                        if (chunks.empty()) {
                                return result::err("chunks is empty");
                        }

                        const auto session_result =
                            generate_session_id(egress_map, next_session_id);
                        if (session_result.failed()) {
                                return result::err(session_result.error());
                        }
                        next_session_id = session_result.value();
                        egress_map[next_session_id] = std::move(chunks);

                        packet = egress_map[next_session_id].at(0).to_buf();
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
        std::unordered_map<SessionId, std::vector<Chunk>> egress_map;
        std::unordered_map<SessionId, std::vector<Chunk>> ingress_map;
        uint16_t max_tries = 0;
        SessionId next_session_id = 0;
        std::mutex egress_mutex;
        std::mutex ingress_mutex;

        result::Result<SessionId> generate_session_id(
            const std::unordered_map<SessionId, std::vector<Chunk>> chunks,
            const SessionId next_session_id) const {
                if (egress_map.size() >=
                    std::numeric_limits<SessionId>::max()) {
                        return result::err(
                            "transport: maximum concurrent sessions reached");
                }

                SessionId candidate = next_session_id;
                const SessionId start_search = candidate;

                while (egress_map.find(candidate) != egress_map.end()) {
                        candidate++;

                        if (candidate == start_search) {
                                return result::err(
                                    "no available session slots");
                        }
                }

                return result::ok(candidate);
        }

        void handle_data(Data &&data) {
                const auto type_result = get_packet_type(data);
                if (type_result.failed()) {
                        // TODO: Add log
                        return;
                }

                std::function<void()> defer;
                {
                        std::scoped_lock lock(ingress_mutex);
                        switch (type_result.value()) {
                        case PacketType::ack:
                                defer = handle_ack(std::move(data));
                                break;
                        case PacketType::nack:
                                defer = handle_nack(std::move(data));
                                break;
                        case PacketType::chunk:
                                defer = handle_chunk(std::move(data));
                                break;
                        }
                }

                if (defer) {
                        defer();
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

        std::function<void()> handle_ack(Data &&data) {
                std::function<void()> defered_function = []() {};

                const auto result = Ack::from_buf(data);
                if (result.failed()) {
                        // TODO: Add log
                        return defered_function;
                }

                const auto ack = result.value();
                const auto it = egress_map.find(ack.session_id);
                if (it == egress_map.end()) {
                        // TODO: Add log
                        return defered_function;
                }

                const auto &chunks = it->second;
                if (chunks.empty()) {
                        // TODO: Add log
                        return defered_function;
                }

                const auto &last_chunk = chunks.back();
                if (last_chunk.index == last_chunk.total_chunks) {
                        handle_done_sending(last_chunk.session_id);
                        return defered_function;
                }

                return [this, &last_chunk]() {
                        send_ack(last_chunk.session_id, last_chunk.index + 1);
                };
        }

        std::function<void()> handle_nack(Data &&data) {
                std::function<void()> defered_function = []() {};

                const auto result = Nack::from_buf(data);
                if (result.failed()) {
                        // TODO: Add log
                        return defered_function;
                }

                const auto nack = result.value();
                const auto it = egress_map.find(nack.session_id);
                if (it == egress_map.end()) {
                        // TODO: Add log
                        return defered_function;
                }

                const auto &chunks = it->second;
                if (chunks.empty()) {
                        // TODO: Add log
                        return defered_function;
                }

                const auto &last_chunk = chunks.back();
                return [this, &last_chunk]() {
                        send_nack(last_chunk.session_id, last_chunk.index);
                };
        }

        // TODO: Add max tries logic
        std::function<void()> handle_chunk(Data &&data) {
                std::function<void()> defered_function = []() {};

                const auto chunk_result = Chunk::from_buf(data);
                if (chunk_result.failed()) {
                        // TODO: Add log
                        return defered_function;
                }

                const auto chunk = chunk_result.value();
                const auto next_index = get_last_index() + 1;
                if (chunk.index != next_index) {
                        defered_function = [this, sid = chunk.session_id,
                                            next_index]() {
                                this->send_nack(sid, next_index);
                        };
                        return defered_function;
                }

                const auto it = ingress_map.find(chunk.session_id);
                if (it == ingress_map.end()) {
                        std::vector<Chunk> chunks;
                        chunks.resize(chunk.total_chunks);
                        const auto idx = chunk.index;
                        chunks[idx] = std::move(chunk);
                        ingress_map[chunk.session_id] = std::move(chunks);
                        defered_function = [this, sid = chunk.session_id,
                                            next_index]() {
                                this->send_ack(sid, next_index);
                        };
                        return defered_function;
                }

                auto &chunks = it->second;
                if (chunk.index >= chunks.size()) {
                        defered_function = [this, sid = chunk.session_id,
                                            next_index]() {
                                this->send_nack(sid, next_index);
                        };
                        return defered_function;
                }
                chunks[chunk.index] = std::move(chunk);

                defered_function = [this, sid = chunk.session_id,
                                    next_index]() {
                        this->send_ack(sid, next_index);
                };

                if (chunk.index == chunk.total_chunks - 1) {
                        return handle_done_receiving(result::ok(chunks));
                }
                return defered_function;
        }

        Indexer get_last_index(SessionId id) const {
                const auto it = ingress_map.find(id);
                if (it == ingress_map.end()) {
                        return 0;
                }

                const auto &chunks = it->second;
                if (chunks.empty()) {
                        return 0;
                }

                return chunks.back().index;
        }

        void handle_done_sending(SessionId id) {
                egress_map.erase(id);
        }

        std::function<void()>
        handle_done_receiving(result::Result<std::vector<Chunk>> result,
                              const SessionId session_id,
                              const Indexer total_chunks) {
                std::function<void()> defered_function = []() {};
                if (result.failed()) {
                        try_callback(result::err(result.error()));
                        return defered_function;
                }

                auto assemble_result = Chunk::assemble(
                    std::move(result).value(), session_id, total_chunks);
                return [this, &result]() {
                        const auto callback_result =
                            try_callback(std::move(result));
                        if (callback_result.failed()) {
                                // TODO: Add log
                        }
                };
        }
};
} // namespace transport
