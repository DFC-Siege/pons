#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace transport {
using Unit = uint8_t;
using Data = std::vector<Unit>;
using DataView = std::span<const Unit>;
using MTU = uint16_t;
} // namespace transport
