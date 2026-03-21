#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "i_future.hpp"
#include "result.hpp"

namespace transport {
using SendCallback =
    std::function<result::Result<bool>(std::span<const uint8_t>)>;

struct ISender {
        using CompleteCallback = std::function<void()>;
        virtual ~ISender() = default;
        virtual result::Result<bool> send(uint8_t session_id, uint8_t command,
                                          std::span<const uint8_t> data,
                                          SendCallback sender,
                                          CompleteCallback on_complete) = 0;
        virtual result::Result<bool> receive(std::span<const uint8_t> data) = 0;
};

struct IReceiver {
        using CompleteCallback =
            std::function<void(std::vector<uint8_t> result)>;
        virtual ~IReceiver() = default;
        virtual result::Result<bool> start(uint8_t session_id, uint8_t command,
                                           std::span<const uint8_t> payload,
                                           SendCallback sender,
                                           CompleteCallback on_complete) = 0;
        virtual result::Result<bool> receive(std::span<const uint8_t> data) = 0;
};

struct FeedResult {
        uint8_t session_id;
        result::Result<bool> result;
};

struct ITransporter {
        using ErrorCallback = std::function<void(std::string_view error)>;
        virtual ~ITransporter() = default;
        virtual result::Result<bool> send(uint8_t command,
                                          std::span<const uint8_t> data,
                                          ISender::CompleteCallback on_complete,
                                          ErrorCallback on_error) = 0;
        virtual std::shared_ptr<async::IFuture<result::Result<bool>>>
        send_async(uint8_t command, std::span<const uint8_t> data) = 0;
        virtual result::Result<bool>
        request(uint8_t command, std::span<const uint8_t> payload,
                IReceiver::CompleteCallback on_complete,
                ErrorCallback on_error) = 0;
        virtual std::shared_ptr<
            async::IFuture<result::Result<std::vector<uint8_t>>>>
        request_async(uint8_t command, std::span<const uint8_t> payload) = 0;
        virtual result::Result<FeedResult>
        feed(std::span<const uint8_t> raw) = 0;
};
} // namespace transport
