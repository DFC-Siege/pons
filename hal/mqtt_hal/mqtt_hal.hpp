#pragma once

#include "result.hpp"
#include <cstdint>
#include <functional>
#include <span>

namespace mqtt {
using Data = std::vector<uint8_t>;
using ReceiveCallback =
    std::function<void(std::string_view topic, Data &&data)>;

template <typename T>
concept MqttHal = requires(T t, Data &&data, ReceiveCallback callback,
                           std::string_view topic) {
        {
                t.send(static_cast<std::string_view>(topic),
                       static_cast<Data &&>(topic, data))
        } -> std::same_as<result::Try>;
        {
                t.on_receive(static_cast<ReceiveCallback>(callback))
        } -> std::same_as<void>;
        { t.loop() } -> std::same_as<result::Try>;
};
} // namespace mqtt
