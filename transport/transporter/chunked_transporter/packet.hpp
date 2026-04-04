#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "data.hpp"
#include "result.hpp"

namespace transport {
using CRC = uint16_t;
using SessionId = uint8_t;
using Indexer = uint16_t;

static CRC crc16(DataView data) {
        CRC crc = 0;
        for (const auto byte : data) {
                crc ^= byte;
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

struct Ack {
        static constexpr uint8_t SESSION_ID_OFFSET = 3;
        Indexer index;
        SessionId session_id;

        Data to_buf() const {
                return {static_cast<Unit>(PacketType::ack),
                        static_cast<Unit>(index & 0xFF),
                        static_cast<Unit>((index >> 8) & 0xFF), session_id};
        }

        static result::Result<Ack> from_buf(DataView buf) {
                if (buf.size() < 5)
                        return result::err("buffer too small");
                if (static_cast<PacketType>(buf[0]) != PacketType::ack)
                        return result::err("invalid packet type");
                return result::ok(
                    Ack{static_cast<Indexer>(buf[1] | (buf[2] << 8)), buf[3]});
        }
};

struct Nack {
        static constexpr uint8_t SESSION_ID_OFFSET = 3;
        Indexer index;
        SessionId session_id;

        Data to_buf() const {
                return {static_cast<Unit>(PacketType::nack),
                        static_cast<Unit>(index & 0xFF),
                        static_cast<Unit>((index >> 8) & 0xFF), session_id};
        }

        static result::Result<Nack> from_buf(DataView buf) {
                if (buf.size() < 5)
                        return result::err("buffer too small");
                if (static_cast<PacketType>(buf[0]) != PacketType::nack)
                        return result::err("invalid packet type");
                return result::ok(
                    Nack{static_cast<Indexer>(buf[1] | (buf[2] << 8)), buf[3]});
        }
};

struct Chunk {
        static constexpr PacketType TYPE = PacketType::chunk;
        Data payload;
        Indexer index;
        Indexer total_chunks;
        CRC checksum;
        static constexpr auto TYPE_SIZE = sizeof(TYPE);
        static constexpr auto INDEX_SIZE = sizeof(index);
        static constexpr auto COUNT_SIZE = sizeof(total_chunks);
        static constexpr auto CHECKSUM_SIZE = sizeof(checksum);
        static constexpr auto HEADER_SIZE =
            TYPE_SIZE + INDEX_SIZE + COUNT_SIZE + CHECKSUM_SIZE;

        std::vector<uint8_t> to_buf() const {
                std::vector<uint8_t> buf;
                buf.reserve(HEADER_SIZE + payload.size());
                buf.push_back(static_cast<uint8_t>(PacketType::chunk));
                auto push16 = [&](uint16_t val) {
                        buf.push_back(val & 0xFF);
                        buf.push_back((val >> 8) & 0xFF);
                };
                push16(index);
                push16(total_chunks);
                push16(checksum);
                buf.insert(buf.end(), payload.begin(), payload.end());
                return buf;
        }

        static result::Result<Chunk> from_buf(std::span<const uint8_t> buf) {
                if (buf.size() < HEADER_SIZE)
                        return result::err("buffer too small");
                if (static_cast<PacketType>(buf[0]) != PacketType::chunk)
                        return result::err("invalid packet type");

                auto pull16 = [&](size_t offset) -> uint16_t {
                        return static_cast<uint16_t>(buf[offset] |
                                                     (buf[offset + 1] << 8));
                };

                Chunk chunk;
                chunk.index = pull16(TYPE_SIZE);
                chunk.total_chunks = pull16(TYPE_SIZE + INDEX_SIZE);
                chunk.checksum = pull16(TYPE_SIZE + INDEX_SIZE + COUNT_SIZE);
                chunk.payload =
                    std::vector<uint8_t>(buf.begin() + HEADER_SIZE, buf.end());

                const auto computed = crc16(chunk.payload);
                if (computed != chunk.checksum)
                        return result::err("checksum mismatch");

                return result::ok(std::move(chunk));
        }
};
} // namespace transport
