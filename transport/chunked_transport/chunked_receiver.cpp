#include <algorithm>
#include <cstdint>
#include <vector>

#include "chunked_receiver.hpp"
#include "i_transport.hpp"
#include "result.hpp"

namespace transport {
ChunkedReceiver::ChunkedReceiver(uint16_t mtu, uint8_t max_attempts)
    : mtu(mtu), max_attempts(max_attempts) {
}

result::Result<bool>
ChunkedReceiver::start(uint8_t session_id, uint8_t command,
                       std::span<const uint8_t> payload, SendCallback sender,
                       IReceiver::CompleteCallback on_complete) {
        if (payload.size() + Chunk::HEADER_SIZE > mtu) {
                return result::err("payload is too large");
        }

        this->session_id = session_id;
        this->command = command;
        this->sender = sender;
        this->on_complete = on_complete;
        current_attempt = 0;
        received_chunks.clear();

        const auto checksum = crc16(payload);
        const auto chunk =
            Chunk{std::vector<uint8_t>{payload.begin(), payload.end()},
                  0,
                  1,
                  checksum,
                  this->session_id,
                  this->command};

        return this->sender(chunk.to_buf());
}

result::Result<bool> ChunkedReceiver::receive(std::span<const uint8_t> data) {
        const auto chunk_result = Chunk::from_buf(data);
        if (chunk_result.failed()) {
                return ack(false);
        }

        const auto chunk = chunk_result.value();
        if (chunk.total_chunks == 0 || chunk.index >= chunk.total_chunks) {
                return ack(false);
        }

        const auto duplicate =
            std::any_of(received_chunks.begin(), received_chunks.end(),
                        [&](const Chunk &c) { return c.index == chunk.index; });
        if (duplicate) {
                return ack(true);
        }

        received_chunks.push_back(chunk);

        if (chunk.index == chunk.total_chunks - 1) {
                on_complete(reconstruct_data(received_chunks));
                received_chunks.clear();
        }

        return ack(true);
}

std::vector<uint8_t>
ChunkedReceiver::reconstruct_data(std::vector<Chunk> chunks) const {
        std::sort(
            chunks.begin(), chunks.end(),
            [](const Chunk &a, const Chunk &b) { return a.index < b.index; });

        std::vector<uint8_t> reconstruct_data;
        for (const auto &chunk : chunks) {
                reconstruct_data.insert(reconstruct_data.end(),
                                        chunk.payload.begin(),
                                        chunk.payload.end());
        }

        return reconstruct_data;
}

result::Result<bool> ChunkedReceiver::ack(bool success) {
        if (!success && ++current_attempt > max_attempts) {
                return result::err("max attempts reached");
        } else if (success) {
                current_attempt = 0;
        }

        const uint16_t index =
            received_chunks.empty()
                ? 0
                : static_cast<uint16_t>(received_chunks.size() - 1);
        const auto ack = Ack{
            index,
            session_id,
            success,
        };
        return this->sender(ack.to_buf());
}
} // namespace transport
