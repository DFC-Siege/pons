#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <span>

#include "result.hpp"

namespace transport {
using DataView = std::span<const uint8_t>;

template <typename T>
concept Protocol = requires(T t, DataView data) {
        { t.send(data) } -> std::same_as<result::Result<DataView>>;
};
} // namespace transport
