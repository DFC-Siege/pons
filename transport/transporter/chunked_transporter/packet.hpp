#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <vector>

#include "data.hpp"
#include "result.hpp"

namespace transport {
using CRC = uint16_t;
using SessionId = uint8_t;
using Indexer = uint16_t;

static CRC crc16(DataView data) {
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
        fin = 0x03,
};

namespace detail {
template <typename T> void push_le(Data &buf, T val) {
        if constexpr (std::is_enum_v<T>) {
                push_le(buf, static_cast<std::underlying_type_t<T>>(val));
        } else {
                constexpr size_t bits_per_unit = sizeof(Unit) * 8;
                for (size_t i = 0; i < sizeof(T); ++i) {
                        buf.push_back(static_cast<Unit>(
                            (val >> (i * bits_per_unit)) & 0xFF));
                }
        }
}

template <typename T> T pull_le(DataView buf, size_t offset) {
        if constexpr (std::is_enum_v<T>) {
                return static_cast<T>(
                    pull_le<std::underlying_type_t<T>>(buf, offset));
        } else {
                T val = 0;
                constexpr size_t bits_per_unit = sizeof(Unit) * 8;
                for (size_t i = 0; i < sizeof(T); ++i) {
                        val |= (static_cast<T>(buf[offset + i])
                                << (i * bits_per_unit));
                }
                return val;
        }
}
} // namespace detail

struct Ack {
        Indexer index;
        SessionId session_id;

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

                return result::ok(Ack{idx, sid});
        }
};

struct Nack {
        Indexer index;
        SessionId session_id;

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

                return result::ok(Nack{idx, sid});
        }
};

struct Chunk {
        Data payload;
        Indexer index;
        Indexer total_chunks;
        CRC checksum;

        static constexpr size_t HEADER_SIZE =
            sizeof(PacketType) + (sizeof(Indexer) * 2) + sizeof(CRC);

        Data to_buf() const {
                Data buf;
                buf.reserve(HEADER_SIZE + payload.size());
                detail::push_le(buf, PacketType::chunk);
                detail::push_le(buf, index);
                detail::push_le(buf, total_chunks);
                detail::push_le(buf, checksum);
                buf.insert(buf.end(), payload.begin(), payload.end());
                return buf;
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
                chunk.checksum = detail::pull_le<CRC>(buf, offset);

                chunk.payload.assign(buf.begin() + HEADER_SIZE, buf.end());

                if (crc16(chunk.payload) != chunk.checksum)
                        return result::err("checksum mismatch");

                return result::ok(std::move(chunk));
        }
};
} // namespace transport
