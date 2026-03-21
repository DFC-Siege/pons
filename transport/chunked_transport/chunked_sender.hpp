#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "i_transport.hpp"
#include "packet.hpp"
#include "result.hpp"

namespace transport {
class ChunkedSender : public ISender {
      public:
        ChunkedSender(uint16_t mtu, uint8_t max_attempts);
        result::Result<bool>
        send(uint8_t session_id, uint8_t command, std::span<const uint8_t> data,
             SendCallback sender,
             ISender::CompleteCallback on_complete) override;
        result::Result<bool> receive(std::span<const uint8_t> data) override;

      protected:
        uint8_t session_id;
        uint8_t command;
        SendCallback sender;
        CompleteCallback on_complete;

      private:
        std::vector<Chunk> chunked_data;
        uint16_t mtu;
        uint16_t current_index = 0;
        uint8_t max_attempts;
        uint8_t current_attempt = 0;

        result::Result<std::vector<Chunk>>
        create_chunks(std::span<const uint8_t> data) const;
        result::Result<Chunk> get_next();
        result::Result<Chunk> repeat();
        result::Result<Chunk> get_chunk();
};
} // namespace transport
