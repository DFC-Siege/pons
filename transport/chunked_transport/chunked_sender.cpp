#include <cstdint>
#include <string_view>

#include "chunked_sender.hpp"
#include "i_transport.hpp"
#include "result.hpp"

namespace transport {
ChunkedSender::ChunkedSender(uint16_t mtu, uint8_t max_attempts)
    : mtu(mtu), max_attempts(max_attempts) {
}

result::Result<bool>
ChunkedSender::send(uint8_t session_id, uint8_t command,
                    std::span<const uint8_t> data, SendCallback sender,
                    ISender::CompleteCallback on_complete) {
        this->session_id = session_id;
        this->command = command;
        this->sender = sender;
        this->on_complete = on_complete;
        chunked_data.clear();
        current_attempt = 0;
        current_index = 0;

        const auto result = create_chunks(data);
        if (result.failed()) {
                return result::err(result.error());
        }
        chunked_data = result.value();

        const auto chunk_result = get_chunk();
        if (chunk_result.failed()) {
                return result::err(chunk_result.error());
        }
        const auto chunk = chunk_result.value();
        return this->sender(chunk.to_buf());
}

result::Result<bool> ChunkedSender::receive(std::span<const uint8_t> data) {
        assert(sender != nullptr && "sender should not be null");
        const auto result = Ack::from_buf(data);
        if (result.failed()) {
                return result::err(result.error());
        }

        const auto ack = result.value();
        if (!ack.success) {
                const auto repeat_result = repeat();
                if (repeat_result.failed()) {
                        return result::err(repeat_result.error());
                }
                const auto chunk = repeat_result.value();
                return sender(chunk.to_buf());
        }

        if (chunked_data.empty()) {
                return result::err("chunked data is empty");
        }

        if (current_index == chunked_data.size() - 1) {
                on_complete();
                return result::ok();
        }

        const auto next_result = get_next();
        if (next_result.failed()) {
                return result::err(next_result.error());
        }

        const auto chunk = next_result.value();
        return sender(chunk.to_buf());
}

result::Result<std::vector<Chunk>>
ChunkedSender::create_chunks(std::span<const uint8_t> data) const {
        if (mtu <= Chunk::HEADER_SIZE) {
                return result::err("mtu too small");
        }
        const auto payload_size = mtu - Chunk::HEADER_SIZE;

        const auto total_chunks = static_cast<uint16_t>(
            (data.size() + payload_size - 1) / payload_size);

        std::vector<Chunk> chunks;
        chunks.reserve(total_chunks);
        for (uint16_t i = 0; i < total_chunks; i++) {
                const auto offset = i * payload_size;
                const auto size =
                    std::min<size_t>(payload_size, data.size() - offset);
                auto payload = std::vector<uint8_t>(
                    data.begin() + offset, data.begin() + offset + size);
                const auto checksum = crc16(data);
                chunks.emplace_back(std::move(payload), i, total_chunks,
                                    checksum, session_id, command);
        }

        if (chunks.empty()) {
                const auto payload = std::vector<uint8_t>();
                const auto checksum = crc16(data);
                chunks.emplace_back(std::move(payload), 0, 1, checksum,
                                    session_id, command);
        }

        return result::ok(std::move(chunks));
}

result::Result<Chunk> ChunkedSender::get_next() {
        if (++current_index >= chunked_data.size()) {
                return result::err("index out of bounds getting next");
        }

        current_attempt = 0;
        return get_chunk();
}

result::Result<Chunk> ChunkedSender::repeat() {
        if (++current_attempt >= max_attempts) {
                return result::err("max attempts reached");
        }
        return get_chunk();
}

result::Result<Chunk> ChunkedSender::get_chunk() {
        if (current_index >= chunked_data.size()) {
                return result::err("index out of bound repeating");
        }

        return result::ok(chunked_data.at(current_index));
}
} // namespace transport
