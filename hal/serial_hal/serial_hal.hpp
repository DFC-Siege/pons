#pragma once

#include "result.hpp"
#include <cstdint>
#include <functional>
#include <span>

namespace serial {
using Data = std::vector<uint8_t>;
using ReceiveCallback = std::function<void(Data &&data)>;

template <typename T>
concept SerialHal = requires(T t, Data &&data, ReceiveCallback callback) {
        { t.send(static_cast<Data &&>(data)) } -> std::same_as<result::Try>;
        {
                t.on_receive(static_cast<ReceiveCallback>(callback))
        } -> std::same_as<void>;
        { t.loop() } -> std::same_as<result::Try>;
};
} // namespace serial
