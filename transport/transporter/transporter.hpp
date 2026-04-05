#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <vector>

#include "result.hpp"
#include "transport_data.hpp"

namespace transport {
using ReceiveCallback = std::function<void(result::Result<Data>)>;

template <typename T>
concept Transporter =
    requires(T t, const T ct, Data &&data, ReceiveCallback callback) {
            {
                    t.send(static_cast<Data &&>(data))
            } -> std::same_as<result::Try>;
            { t.set_receiver(callback) } -> std::same_as<void>;
            { ct.get_mtu() } -> std::same_as<MTU>;
    };
} // namespace transport
