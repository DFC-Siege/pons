#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "result.hpp"

namespace transport {
template <typename T>
concept Transporter = requires(T t, std::span<const uint8_t> data) {
        { t.send(data) } -> std::same_as<result::Result<bool>>;
        { t.receive() } -> std::same_as<result::Result<std::vector<uint8_t>>>;
};
} // namespace transport
