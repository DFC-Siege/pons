#pragma once

#include "result.hpp"
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace serial {

struct ISerialHal {
        virtual ~ISerialHal() = default;
        virtual result::Result<bool> send(std::span<const uint8_t> data) = 0;
        virtual result::Result<std::vector<uint8_t>> read(uint32_t timeout) = 0;
};

} // namespace serial
