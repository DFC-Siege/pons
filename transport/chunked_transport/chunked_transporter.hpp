#pragma once

#include <cstdint>
#include <set>
#include <span>

#include "i_future.hpp"
#include "i_transport.hpp"
#include "result.hpp"

namespace transport {
class ChunkedTransporter : public ITransporter {
      public:
        ChunkedTransporter(uint16_t mtu);

        result::Result<FeedResult> feed(std::span<const uint8_t> data) override;

        result::Result<bool>
        send(uint8_t command, std::span<const uint8_t> data,
             ISender::CompleteCallback on_complete,
             ITransporter::ErrorCallback on_error) override;

        std::shared_ptr<async::IFuture<result::Result<bool>>>
        send_async(uint8_t command, std::span<const uint8_t> data) override;

        result::Result<bool> request(uint8_t command,
                                     std::span<const uint8_t> payload,
                                     IReceiver::CompleteCallback on_complete,
                                     ErrorCallback on_error) override;

        std::shared_ptr<async::IFuture<result::Result<std::vector<uint8_t>>>>
        request_async(uint8_t command,
                      std::span<const uint8_t> payload) override;

      protected:
        std::unordered_map<uint8_t, std::unique_ptr<ISender>> senders;
        std::unordered_map<uint8_t, std::unique_ptr<IReceiver>> receivers;
        std::unordered_map<uint8_t, ErrorCallback> error_callbacks;
        std::set<uint8_t> available_sender_sessions;
        std::set<uint8_t> available_receiver_sessions;
        uint8_t next_session_id = 0;
        uint16_t mtu;

        result::Result<uint8_t> next_receiver_session();
        result::Result<uint8_t> next_sender_session();
        void remove_sender(uint8_t session_id);
        void remove_receiver(uint8_t session_id);

        virtual result::Result<bool>
        concrete_send(std::span<const uint8_t> data) = 0;
};
} // namespace transport
