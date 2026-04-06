#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "i_logger.hpp"
#include "logger.hpp"
#include "mutex.hpp"
#include "packet.hpp"
#include "platform_mutex.hpp"
#include "transporter/base_transporter.hpp"
#include "transporter/transporter.hpp"

namespace transport {
struct SessionWrapper {
        std::vector<Chunk> chunks;
        uint16_t tries;
        std::chrono::steady_clock::time_point timestamp;
};

template <Transporter T, locking::Mutex M = DefaultMutex>
class ChunkedTransporter : public BaseTransporter {
      public:
        ChunkedTransporter(T &transporter, uint16_t max_tries,
                           std::chrono::milliseconds timeout)
            : transporter(transporter), max_tries(max_tries), timeout(timeout) {
                transporter.set_receiver([this](result::Result<Data> result) {
                        if (result.failed()) {
                                logging::logger().println(
                                    logging::LogLevel::Error, TAG,
                                    result.error());
                                return;
                        }
                        handle_data(std::move(result).value());
                });
        };

        result::Try send(Data &&data) override {
                logging::logger().println(logging::LogLevel::Debug, TAG,
                                          "send called, payload size: " +
                                              std::to_string(data.size()));
                Data packet;
                {
                        std::lock_guard<M> lock_e(egress_mutex);
                        std::lock_guard<M> lock_i(ingress_mutex);
                        remove_stale();
                        const auto session_result =
                            generate_session_id(egress_map, next_session_id);
                        if (session_result.failed()) {
                                return result::err(session_result.error());
                        }
                        next_session_id = session_result.value();

                        const auto result = Chunk::fragment(
                            data, transporter.get_mtu(), next_session_id);
                        if (result.failed()) {
                                logging::logger().println(
                                    logging::LogLevel::Error, TAG,
                                    "fragment failed: " +
                                        std::string(result.error()));
                                return result::err(result.error());
                        }

                        auto chunks = result.value();
                        if (chunks.empty()) {
                                return result::err("chunks is empty");
                        }

                        logging::logger().println(
                            logging::LogLevel::Debug, TAG,
                            "fragmented into " + std::to_string(chunks.size()) +
                                " chunks");
                        logging::logger().println(
                            logging::LogLevel::Debug, TAG,
                            "session id: " + std::to_string(next_session_id));

                        auto &session = egress_map[next_session_id];
                        session.chunks = std::move(chunks);
                        session.tries = 0;
                        session.timestamp = std::chrono::steady_clock::now();
                        packet = session.chunks.at(0).to_buf();
                }

                logging::logger().println(logging::LogLevel::Debug, TAG,
                                          "sending first chunk, size: " +
                                              std::to_string(packet.size()));

                return transporter.send(std::move(packet));
        }

        MTU get_mtu() const override {
                const auto mtu = transporter.get_mtu();
                assert(mtu > Chunk::HEADER_SIZE &&
                       "mtu too small for chunking");
                return mtu - Chunk::HEADER_SIZE;
        }

      private:
        static constexpr auto TAG = "ChunkedTransporter";
        T &transporter;
        std::unordered_map<SessionId, SessionWrapper> egress_map;
        std::unordered_map<SessionId, SessionWrapper> ingress_map;
        uint16_t max_tries = 0;
        std::chrono::milliseconds timeout;
        SessionId next_session_id = 0;
        M egress_mutex;
        M ingress_mutex;

        result::Result<SessionId> generate_session_id(
            const std::unordered_map<SessionId, SessionWrapper> &sessions,
            const SessionId next_session_id) const {
                if (sessions.size() >= std::numeric_limits<SessionId>::max()) {
                        return result::err(
                            "maximum concurrent sessions reached");
                }

                SessionId candidate = next_session_id;
                const SessionId start_search = candidate;

                while (sessions.find(candidate) != sessions.end()) {
                        candidate++;
                        if (candidate == start_search) {
                                return result::err(
                                    "no available session slots");
                        }
                }

                return result::ok(candidate);
        }

        void handle_data(Data &&data) {
                logging::logger().println(logging::LogLevel::Debug, TAG,
                                          "received data, size: " +
                                              std::to_string(data.size()));
                const auto type_result = get_packet_type(data);
                if (type_result.failed()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  type_result.error());
                        return;
                }
                std::function<void()> defer;
                {
                        std::lock_guard<M> lock_e(egress_mutex);
                        std::lock_guard<M> lock_i(ingress_mutex);
                        remove_stale();
                        switch (type_result.value()) {
                        case PacketType::ack:
                                logging::logger().println(
                                    logging::LogLevel::Debug, TAG,
                                    "received ack");
                                defer = handle_ack(std::move(data));
                                break;
                        case PacketType::nack:
                                logging::logger().println(
                                    logging::LogLevel::Debug, TAG,
                                    "received nack");
                                defer = handle_nack(std::move(data));
                                break;
                        case PacketType::chunk:
                                logging::logger().println(
                                    logging::LogLevel::Debug, TAG,
                                    "received chunk");
                                defer = handle_chunk(std::move(data));
                                break;
                        }
                }
                if (defer) {
                        defer();
                }
        }

        void send_nack(SessionId session_id, Indexer index) {
                logging::logger().println(
                    logging::LogLevel::Debug, TAG,
                    "sending nack, session: " + std::to_string(session_id) +
                        " index: " + std::to_string(index));
                Nack nack;
                nack.session_id = session_id;
                nack.index = index;
                const auto result = transporter.send(nack.to_buf());
                if (result.failed()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  result.error());
                }
        }

        void send_ack(SessionId session_id, Indexer index) {
                logging::logger().println(
                    logging::LogLevel::Debug, TAG,
                    "sending ack, session: " + std::to_string(session_id) +
                        " index: " + std::to_string(index));
                Ack ack;
                ack.session_id = session_id;
                ack.index = index;
                const auto result = transporter.send(ack.to_buf());
                if (result.failed()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  result.error());
                }
        }

        std::function<void()> handle_ack(Data &&data) {
                std::function<void()> defered_function = []() {};

                const auto result = Ack::from_buf(data);
                if (result.failed()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  result.error());
                        return defered_function;
                }

                const auto ack = result.value();
                logging::logger().println(
                    logging::LogLevel::Debug, TAG,
                    "handling ack, session: " + std::to_string(ack.session_id) +
                        " index: " + std::to_string(ack.index));

                const auto it = egress_map.find(ack.session_id);
                if (it == egress_map.end()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  "no egress session found");
                        return defered_function;
                }

                auto &session = it->second;
                session.timestamp = std::chrono::steady_clock::now();
                const auto &chunks = session.chunks;
                if (chunks.empty()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  "chunks are empty");
                        return defered_function;
                }

                if (ack.index >= chunks.size()) {
                        logging::logger().println(
                            logging::LogLevel::Error, TAG,
                            "ack index is greater than chunks size");
                        return defered_function;
                }

                session.tries = 0;
                const uint16_t total_chunks = chunks[ack.index].total_chunks;
                if (ack.index >= total_chunks - 1) {
                        logging::logger().println(
                            logging::LogLevel::Debug, TAG,
                            "all chunks acked, done sending: " +
                                std::to_string(ack.index + 1) + "/" +
                                std::to_string(total_chunks));
                        handle_done_sending(ack.session_id);
                        return defered_function;
                }

                const auto sid = ack.session_id;
                const auto next_index = ack.index + 1;

                return [this, sid, next_index]() {
                        std::lock_guard<M> lock_e(this->egress_mutex);
                        std::lock_guard<M> lock_i(this->ingress_mutex);
                        auto it = this->egress_map.find(sid);
                        auto &session = it->second;
                        if (it != this->egress_map.end() &&
                            next_index < session.chunks.size()) {
                                auto packet =
                                    session.chunks.at(next_index).to_buf();
                                this->transporter.send(std::move(packet));
                        }
                };
        }

        std::function<void()> handle_nack(Data &&data) {
                std::function<void()> defered_function = []() {};

                const auto result = Nack::from_buf(data);
                if (result.failed()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  result.error());
                        return defered_function;
                }

                const auto nack = result.value();
                logging::logger().println(
                    logging::LogLevel::Debug, TAG,
                    "handling nack, session: " +
                        std::to_string(nack.session_id) +
                        " index: " + std::to_string(nack.index));

                const auto it = egress_map.find(nack.session_id);
                if (it == egress_map.end()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  "no egress session found");
                        return defered_function;
                }

                auto &session = it->second;
                session.timestamp = std::chrono::steady_clock::now();
                const auto &chunks = session.chunks;
                if (nack.index >= chunks.size()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  "nack index out of bounds");
                        return defered_function;
                }

                ++session.tries;
                if (session.tries >= max_tries) {
                        logging::logger().println(
                            logging::LogLevel::Error, TAG,
                            "max tries reached for session: " +
                                std::to_string(nack.session_id));
                        handle_done_sending(nack.session_id);
                        return defered_function;
                }

                const auto sid = nack.session_id;
                const auto retry_index = nack.index;

                return [this, sid, retry_index]() {
                        std::lock_guard<M> lock_e(this->egress_mutex);
                        std::lock_guard<M> lock_i(this->ingress_mutex);
                        auto it = this->egress_map.find(sid);
                        auto &session = it->second;
                        if (it != this->egress_map.end() &&
                            retry_index < session.chunks.size()) {
                                auto packet =
                                    session.chunks.at(retry_index).to_buf();
                                this->transporter.send(std::move(packet));
                        }
                };
        }

        std::function<void()> handle_chunk(Data &&data) {
                std::function<void()> defered_function = []() {};

                const auto chunk_result = Chunk::from_buf(data);
                if (chunk_result.failed()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  chunk_result.error());
                        return defered_function;
                }

                const auto chunk = chunk_result.value();
                logging::logger().println(
                    logging::LogLevel::Debug, TAG,
                    "handling chunk, session: " +
                        std::to_string(chunk.session_id) +
                        " index: " + std::to_string(chunk.index) + "/" +
                        std::to_string(chunk.total_chunks - 1) +
                        " payload size: " +
                        std::to_string(chunk.payload.size()));

                const auto expected_index = get_next_index(chunk.session_id);
                if (chunk.index != expected_index) {
                        logging::logger().println(
                            logging::LogLevel::Error, TAG,
                            "unexpected chunk index, expected: " +
                                std::to_string(expected_index) +
                                " got: " + std::to_string(chunk.index));
                        return
                            [this, sid = chunk.session_id, expected_index]() {
                                    this->send_nack(sid, expected_index);
                            };
                }

                const auto sid = chunk.session_id;
                const auto total_chunks = chunk.total_chunks;
                const auto chunk_index = chunk.index;

                const auto it = ingress_map.find(sid);
                if (it == ingress_map.end()) {
                        std::vector<Chunk> chunks;
                        chunks.resize(total_chunks);
                        chunks[chunk_index] = std::move(chunk);
                        auto &session = ingress_map[sid];
                        session.chunks = std::move(chunks);
                        session.tries = 0;
                        session.timestamp = std::chrono::steady_clock::now();

                        if (chunk_index == total_chunks - 1) {
                                logging::logger().println(
                                    logging::LogLevel::Debug, TAG,
                                    "last chunk received, assembling");
                                return handle_done_receiving(
                                    result::ok(
                                        std::move(ingress_map[sid].chunks)),
                                    sid, total_chunks, chunk_index);
                        }

                        return [this, sid, expected_index]() {
                                this->send_ack(sid, expected_index);
                        };
                }

                auto &session = it->second;
                session.timestamp = std::chrono::steady_clock::now();
                auto &chunks = session.chunks;
                if (chunk_index >= chunks.size()) {
                        logging::logger().println(
                            logging::LogLevel::Error, TAG,
                            "chunk index out of bounds: " +
                                std::to_string(chunk_index));
                        return [this, sid, expected_index]() {
                                this->send_nack(sid, expected_index);
                        };
                }

                chunks[chunk_index] = std::move(chunk);

                if (chunk_index == total_chunks - 1) {
                        logging::logger().println(
                            logging::LogLevel::Debug, TAG,
                            "last chunk received, assembling");
                        return handle_done_receiving(
                            result::ok(std::move(chunks)), sid, total_chunks,
                            chunk_index);
                }

                return [this, sid, expected_index]() {
                        this->send_ack(sid, expected_index);
                };
        }

        Indexer get_next_index(SessionId id) const {
                const auto it = ingress_map.find(id);
                if (it == ingress_map.end()) {
                        return 0;
                }

                const auto &chunks = it->second.chunks;
                if (chunks.empty()) {
                        return 0;
                }

                for (Indexer i = 0; i < static_cast<Indexer>(chunks.size());
                     ++i) {
                        if (chunks[i].payload.empty()) {
                                return i;
                        }
                }

                return static_cast<Indexer>(chunks.size());
        }

        void handle_done_sending(SessionId id) {
                logging::logger().println(logging::LogLevel::Debug, TAG,
                                          "done sending session: " +
                                              std::to_string(id));
                egress_map.erase(id);
        }

        std::function<void()>
        handle_done_receiving(result::Result<std::vector<Chunk>> result,
                              const SessionId session_id,
                              const Indexer total_chunks,
                              const Indexer last_index) {
                std::function<void()> defered_function = []() {};
                if (result.failed()) {
                        try_callback(result::err(result.error()));
                        return defered_function;
                }

                auto assemble_result = Chunk::assemble(
                    std::move(result).value(), session_id, total_chunks);

                ingress_map.erase(session_id);

                logging::logger().println(logging::LogLevel::Verbose, TAG,
                                          "ingress session " +
                                              std::to_string(session_id) +
                                              " erased");

                if (assemble_result.failed()) {
                        logging::logger().println(
                            logging::LogLevel::Error, TAG,
                            "assembly failed: " +
                                std::string(assemble_result.error()));
                } else {
                        logging::logger().println(
                            logging::LogLevel::Debug, TAG,
                            "assembly succeeded, payload size: " +
                                std::to_string(assemble_result.value().size()));
                }

                return
                    [this, session_id, last_index,
                     assemble_result = std::move(assemble_result)]() mutable {
                            send_ack(session_id, last_index);
                            const auto callback_result =
                                try_callback(std::move(assemble_result));
                            if (callback_result.failed()) {
                                    logging::logger().println(
                                        logging::LogLevel::Error, TAG,
                                        callback_result.error());
                            }
                    };
        }

        void remove_stale() {
                if (timeout == std::chrono::milliseconds::zero()) {
                        return;
                }

                const auto now = std::chrono::steady_clock::now();
                auto prune =
                    [&](std::unordered_map<SessionId, SessionWrapper> &map) {
                            for (auto it = map.begin(); it != map.end();) {
                                    if (now - it->second.timestamp > timeout) {
                                            it = map.erase(it);
                                            logging::logger().println(
                                                logging::LogLevel::Warning, TAG,
                                                "removed stale request");
                                    } else {
                                            ++it;
                                    }
                            }
                    };

                prune(egress_map);
                prune(ingress_map);
        }
};

static_assert(Transporter<ChunkedTransporter<BaseTransporter>>);
} // namespace transport
