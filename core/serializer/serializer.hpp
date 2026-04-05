#pragma once
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include "result.hpp"

namespace serializer {
using Data = std::vector<uint8_t>;
struct Writer {
        Data buf;
        template <typename T> void write(const T &v) {
                auto *p = reinterpret_cast<const uint8_t *>(&v);
                buf.insert(buf.end(), p, p + sizeof(T));
        }
};

struct Reader {
        std::span<const uint8_t> buf;
        size_t pos = 0;
        template <typename T> result::Result<T> read() {
                if (pos + sizeof(T) > buf.size()) {
                        return result::err("buffer too small");
                }
                T v;
                std::memcpy(&v, buf.data() + pos, sizeof(T));
                pos += sizeof(T);
                return result::ok(v);
        }
};

template <typename T>
concept Serializable = requires(T t, std::span<const uint8_t> buf) {
        { t.serialize() } -> std::same_as<Data>;
        { T::deserialize(buf) } -> std::same_as<result::Result<T>>;
};
} // namespace serializer
