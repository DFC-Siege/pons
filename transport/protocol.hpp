#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <span>

#include "result.hpp"

namespace transport {

using DataView = std::span<const uint8_t>;
using ReceiveCallback = std::function<void(DataView)>;

template <typename T>
concept Protocol = requires(T t, DataView data, ReceiveCallback callback) {
        { t.send(data) } -> std::same_as<result::Result<DataView>>;
        { t.add_receiver(callback) } -> std::same_as<result::Result<bool>>;
};

} // namespace transport
