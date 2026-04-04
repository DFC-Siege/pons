#pragma once

#include "result.hpp"
#include <cstdint>
#include <functional>
#include <span>

namespace serial {

using ReceiveCallback = std::function<void(std::span<const uint8_t> data)>;

struct ISerialHal {
        virtual ~ISerialHal() = default;
        virtual result::Result<bool> send(std::span<const uint8_t> data) = 0;
        virtual void on_receive(ReceiveCallback cb) = 0;
        virtual result::Result<bool> loop() = 0;
};

} // namespace serial
