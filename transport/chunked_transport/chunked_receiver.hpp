#pragma once

#include <cstdint>
#include <vector>

#include "i_transport.hpp"
#include "packet.hpp"
#include "result.hpp"

namespace transport {
class ChunkedReceiver : public IReceiver {
      public:
        ChunkedReceiver(uint16_t mtu, uint8_t max_attempts);
        result::Result<bool>
        start(uint8_t session_id, uint8_t command,
              std::span<const uint8_t> payload, SendCallback sender,
              IReceiver::CompleteCallback on_complete) override;
        result::Result<bool> receive(std::span<const uint8_t> data) override;

      protected:
        uint8_t session_id;
        uint8_t command;
        SendCallback sender;
        CompleteCallback on_complete;

      private:
        std::vector<Chunk> received_chunks;
        uint16_t mtu;
        uint16_t current_index = 0;
        uint8_t max_attempts;
        uint8_t current_attempt = 0;

        std::vector<uint8_t> reconstruct_data(std::vector<Chunk> chunks) const;
        result::Result<bool> ack(bool success);
};
} // namespace transport
