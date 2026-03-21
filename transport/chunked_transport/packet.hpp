#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "result.hpp"

namespace transport {
static uint16_t crc16(std::span<const uint8_t> data) {
        uint16_t crc = 0;
        for (const auto byte : data) {
                crc ^= byte;
                for (int i = 0; i < 8; i++) {
                        crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
                }
        }
        return crc;
}

enum class PacketType : uint8_t {
        chunk = 0x01,
        ack = 0x02,
};

struct Ack {
        static constexpr uint8_t SESSION_ID_OFFSET = 3;
        uint16_t index;
        uint8_t session_id;
        bool success;

        std::vector<uint8_t> to_buf() const {
                return {static_cast<uint8_t>(PacketType::ack),
                        static_cast<uint8_t>(index & 0xFF),
                        static_cast<uint8_t>((index >> 8) & 0xFF), session_id,
                        static_cast<uint8_t>(success)};
        }

        static result::Result<Ack> from_buf(std::span<const uint8_t> buf) {
                if (buf.size() < 5)
                        return result::err("buffer too small");
                if (static_cast<PacketType>(buf[0]) != PacketType::ack)
                        return result::err("invalid packet type");
                return result::ok(
                    Ack{static_cast<uint16_t>(buf[1] | (buf[2] << 8)), buf[3],
                        static_cast<bool>(buf[4])});
        }
};

struct Chunk {
        static constexpr auto HEADER_SIZE = 9;
        static constexpr uint8_t SESSION_ID_OFFSET = 7;
        std::vector<uint8_t> payload;
        uint16_t index;
        uint16_t total_chunks;
        uint16_t checksum;
        uint8_t session_id;
        uint8_t command;

        std::vector<uint8_t> to_buf() const {
                std::vector<uint8_t> buf;
                buf.reserve(1 + sizeof(index) + sizeof(total_chunks) +
                            sizeof(checksum) + sizeof(session_id) +
                            sizeof(command) + payload.size());
                buf.push_back(static_cast<uint8_t>(PacketType::chunk));
                auto push16 = [&](uint16_t val) {
                        buf.push_back(val & 0xFF);
                        buf.push_back((val >> 8) & 0xFF);
                };
                push16(index);
                push16(total_chunks);
                push16(checksum);
                buf.push_back(session_id);
                buf.push_back(command);
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
                chunk.index = pull16(1);
                chunk.total_chunks = pull16(3);
                chunk.checksum = pull16(5);
                chunk.session_id = buf[7];
                chunk.command = buf[8];
                chunk.payload =
                    std::vector<uint8_t>(buf.begin() + HEADER_SIZE, buf.end());

                const auto computed = crc16(chunk.payload);
                if (computed != chunk.checksum)
                        return result::err("checksum mismatch");

                return result::ok(std::move(chunk));
        }
};
} // namespace transport
