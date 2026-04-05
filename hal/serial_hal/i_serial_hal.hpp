#pragma once

#include "result.hpp"
#include <cstdint>
#include <functional>
#include <span>

namespace serial {

using Data = std::vector<uint8_t>;
using ReceiveCallback = std::function<void(Data &&data)>;

struct ISerialHal {
        virtual ~ISerialHal() = default;
        virtual result::Status send(Data &&data) = 0;
        virtual void on_receive(ReceiveCallback cb) = 0;
        virtual result::Status loop() = 0;
};

} // namespace serial
