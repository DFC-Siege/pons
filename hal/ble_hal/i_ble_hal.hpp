#pragma once

#include <cstdint>
#include <functional>
#include <span>

#include "result.hpp"

namespace ble {

using ReceiveCallback = std::function<void(std::span<const uint8_t> data)>;
using ConnectionCallback = std::function<void(bool connected)>;

struct IBleHal {
        virtual ~IBleHal() = default;
        virtual result::Result<bool> send(std::span<const uint8_t> data) = 0;
        virtual void on_receive(ReceiveCallback cb) = 0;
        virtual void on_connection_changed(ConnectionCallback cb) = 0;
        virtual bool is_connected() const = 0;
};

} // namespace ble
