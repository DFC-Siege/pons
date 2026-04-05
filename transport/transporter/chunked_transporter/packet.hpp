#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <vector>

#include "result.hpp"
#include "transport_data.hpp"

namespace transport {
using CRC = uint16_t;
using SessionId = uint8_t;
using Indexer = uint16_t;

inline CRC crc16(DataView data) {
        CRC crc = 0;
        for (const auto unit : data) {
                crc ^= unit;
                for (int i = 0; i < sizeof(Unit) * 8; i++) {
                        crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
                }
        }
        return crc;
}

enum class PacketType : uint8_t {
        chunk = 0x00,
        ack = 0x01,
        nack = 0x02,
};

inline result::Result<PacketType> get_packet_type(DataView data) {
        if (data.empty()) {
                return result::err("empty data");
        }

        auto type_raw = detail::pull_le<uint8_t>(data, 0);

        switch (static_cast<PacketType>(type_raw)) {
        case PacketType::chunk:
        case PacketType::ack:
        case PacketType::nack:
                return result::ok(static_cast<PacketType>(type_raw));
        default:
                return result::err("unknown packet type");
        }
}

struct Packet {
        SessionId session_id;
};

struct Ack : public Packet {
        Indexer index;

        static constexpr size_t MIN_SIZE =
            sizeof(PacketType) + sizeof(Indexer) + sizeof(SessionId);

        Data to_buf() const {
                Data buf;
                buf.reserve(MIN_SIZE);
                detail::push_le(buf, PacketType::ack);
                detail::push_le(buf, index);
                detail::push_le(buf, session_id);
                return buf;
        }

        static result::Result<Ack> from_buf(DataView buf) {
                if (buf.size() < MIN_SIZE)
                        return result::err("buffer too small");
                if (detail::pull_le<PacketType>(buf, 0) != PacketType::ack)
                        return result::err("invalid packet type");

                size_t offset = sizeof(PacketType);
                Indexer idx = detail::pull_le<Indexer>(buf, offset);
                offset += sizeof(Indexer);
                SessionId sid = detail::pull_le<SessionId>(buf, offset);

                return result::ok(Ack{{sid}, idx});
        }
};

struct Nack : public Packet {
        Indexer index;

        static constexpr size_t MIN_SIZE =
            sizeof(PacketType) + sizeof(Indexer) + sizeof(SessionId);

        Data to_buf() const {
                Data buf;
                buf.reserve(MIN_SIZE);
                detail::push_le(buf, PacketType::nack);
                detail::push_le(buf, index);
                detail::push_le(buf, session_id);
                return buf;
        }

        static result::Result<Nack> from_buf(DataView buf) {
                if (buf.size() < MIN_SIZE)
                        return result::err("buffer too small");
                if (detail::pull_le<PacketType>(buf, 0) != PacketType::nack)
                        return result::err("invalid packet type");

                size_t offset = sizeof(PacketType);
                Indexer idx = detail::pull_le<Indexer>(buf, offset);
                offset += sizeof(Indexer);
                SessionId sid = detail::pull_le<SessionId>(buf, offset);

                return result::ok(Nack{{sid}, idx});
        }
};

struct Chunk : public Packet {
        Indexer index;
        Indexer total_chunks;
        CRC checksum;
        Data payload;

        static constexpr size_t HEADER_SIZE = sizeof(PacketType) +
                                              (sizeof(Indexer) * 2) +
                                              sizeof(SessionId) + sizeof(CRC);

        Data to_buf() const {
                Data buf;
                buf.reserve(HEADER_SIZE + payload.size());
                detail::push_le(buf, PacketType::chunk);
                detail::push_le(buf, index);
                detail::push_le(buf, total_chunks);
                detail::push_le(buf, session_id);
                detail::push_le(buf, checksum);
                buf.insert(buf.end(), payload.begin(), payload.end());
                return buf;
        }

        static result::Result<Data> assemble(std::vector<Chunk> &&chunks,
                                             const SessionId session_id,
                                             const Indexer total_chunks) {
                if (chunks.empty()) {
                        return result::err("chunks empty");
                }

                if (chunks.size() != total_chunks) {
                        return result::err("missing chunks");
                }

                size_t total_payload_size = 0;
                for (const auto &chunk : chunks) {
                        if (chunk.session_id != session_id) {
                                return result::err("session id mismatch");
                        }
                        total_payload_size += chunk.payload.size();
                }

                Data data;
                data.reserve(total_payload_size);

                Indexer last_index = 0;
                for (auto &chunk : chunks) {
                        if (chunk.index != last_index++) {
                                return result::err("chunk out of order");
                        }

                        data.insert(
                            data.end(),
                            std::make_move_iterator(chunk.payload.begin()),
                            std::make_move_iterator(chunk.payload.end()));
                }

                return result::ok(std::move(data));
        }

        static result::Result<std::vector<Chunk>>
        fragment(const Data &data, MTU raw_mtu, SessionId session_id) {
                if (data.empty())
                        return result::err("empty data");
                if (raw_mtu <= HEADER_SIZE)
                        return result::err("MTU too small");

                const size_t max_payload = raw_mtu - HEADER_SIZE;
                const size_t total_size = data.size();
                const Indexer total_chunks = static_cast<Indexer>(
                    (total_size + max_payload - 1) / max_payload);

                std::vector<Chunk> chunks;
                chunks.reserve(total_chunks);

                for (Indexer i = 0; i < total_chunks; ++i) {
                        const size_t offset = i * max_payload;
                        const size_t current_payload_size =
                            std::min(max_payload, total_size - offset);
                        Chunk chunk;
                        chunk.session_id = session_id;
                        chunk.index = i;
                        chunk.total_chunks = total_chunks;
                        chunk.payload.assign(data.begin() + offset,
                                             data.begin() + offset +
                                                 current_payload_size);
                        chunk.checksum = crc16(chunk.payload);
                        chunks.push_back(std::move(chunk));
                }

                return result::ok(std::move(chunks));
        }

        static result::Result<Chunk> from_buf(DataView buf) {
                if (buf.size() < HEADER_SIZE)
                        return result::err("buffer too small");
                if (detail::pull_le<PacketType>(buf, 0) != PacketType::chunk)
                        return result::err("invalid packet type");

                size_t offset = sizeof(PacketType);
                Chunk chunk;
                chunk.index = detail::pull_le<Indexer>(buf, offset);
                offset += sizeof(Indexer);
                chunk.total_chunks = detail::pull_le<Indexer>(buf, offset);
                offset += sizeof(Indexer);
                chunk.session_id = detail::pull_le<SessionId>(buf, offset);
                offset += sizeof(SessionId);
                chunk.checksum = detail::pull_le<CRC>(buf, offset);
                chunk.payload.assign(buf.begin() + HEADER_SIZE, buf.end());

                if (crc16(chunk.payload) != chunk.checksum) {
                        return result::err("checksum mismatch");
                }

                if (chunk.index >= chunk.total_chunks) {
                        return result::err("index >= total_chunks");
                }

                return result::ok(std::move(chunk));
        }
};
} // namespace transport
