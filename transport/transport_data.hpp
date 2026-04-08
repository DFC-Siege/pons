#pragma once

#include <cassert>
#include <cstdint>
#include <span>
#include <vector>

namespace transport {
using Unit = uint8_t;
using Data = std::vector<Unit>;
using DataView = std::span<const Unit>;
using MTU = uint16_t;

namespace detail {
template <typename T> void push_le(Data &buf, T val) {
        if constexpr (std::is_enum_v<T>) {
                push_le(buf, static_cast<std::underlying_type_t<T>>(val));
        } else {
                constexpr size_t bits_per_unit = sizeof(Unit) * 8;
                for (size_t i = 0; i < sizeof(T); ++i) {
                        buf.push_back(static_cast<Unit>(
                            (val >> (i * bits_per_unit)) & 0xFF));
                }
        }
}

template <typename T> T pull_le(DataView buf, size_t offset) {
        if constexpr (std::is_enum_v<T>) {
                return static_cast<T>(
                    pull_le<std::underlying_type_t<T>>(buf, offset));
        } else {
                assert(offset + sizeof(T) <= buf.size() &&
                       "pull_le: out of bounds");
                T val = 0;
                constexpr size_t bits_per_unit = sizeof(Unit) * 8;
                for (size_t i = 0; i < sizeof(T); ++i) {
                        val |= (static_cast<T>(buf[offset + i])
                                << (i * bits_per_unit));
                }
                return val;
        }
}
} // namespace detail

} // namespace transport
