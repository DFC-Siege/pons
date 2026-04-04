#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <span>

#include "result.hpp"

namespace transport {
using DataView = std::span<const uint8_t>;
using ReceiveCallback = std::function<void(DataView)>;
using MTU = uint16_t;

template <typename T>
concept Transporter =
    requires(T t, const T ct, DataView data, ReceiveCallback callback) {
            { t.send(data) } -> std::same_as<result::Result<bool>>;
            { t.set_receiver(callback) } -> std::same_as<void>;
            { ct.get_mtu() } -> std::same_as<MTU>;
    };
} // namespace transport
