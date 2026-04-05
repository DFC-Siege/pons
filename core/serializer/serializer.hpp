#pragma once
#include "result.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace serializer {
using Unit = uint8_t;
using Data = std::vector<Unit>;
using DataView = std::span<const Unit>;

struct Writer {
        Data buf;

        template <typename T> void write(const T &v) {
                static_assert(std::is_trivially_copyable_v<T>,
                              "use a specialization for non-trivial types");
                const auto *p = reinterpret_cast<const Unit *>(&v);
                buf.insert(buf.end(), p, p + sizeof(T));
        }

        void write(const std::string &v) {
                write(static_cast<uint32_t>(v.size()));
                buf.insert(buf.end(), v.begin(), v.end());
        }

        void write(const std::vector<Unit> &v) {
                write(static_cast<uint32_t>(v.size()));
                buf.insert(buf.end(), v.begin(), v.end());
        }

        template <typename T> void write(const std::vector<T> &v) {
                write(static_cast<uint32_t>(v.size()));
                for (const auto &item : v) {
                        write(item);
                }
        }

        template <typename T, size_t N> void write(const std::array<T, N> &v) {
                for (const auto &item : v) {
                        write(item);
                }
        }

        template <typename A, typename B> void write(const std::pair<A, B> &v) {
                write(v.first);
                write(v.second);
        }

        template <typename T> void write(const std::optional<T> &v) {
                write(static_cast<uint8_t>(v.has_value()));
                if (v)
                        write(*v);
        }
};

struct Reader {
        DataView buf;
        size_t pos = 0;

        template <typename T> result::Result<T> read() {
                static_assert(std::is_trivially_copyable_v<T>,
                              "use a named read method for non-trivial types");
                if (pos + sizeof(T) > buf.size()) {
                        return result::err("buffer too small");
                }
                T v;
                std::memcpy(&v, buf.data() + pos, sizeof(T));
                pos += sizeof(T);
                return result::ok(v);
        }

        result::Result<std::string> read_string() {
                const auto size = read<uint32_t>();
                if (size.failed())
                        return result::err(size.error());
                if (pos + size.value() > buf.size())
                        return result::err("buffer too small");
                std::string v(reinterpret_cast<const char *>(buf.data() + pos),
                              size.value());
                pos += size.value();
                return result::ok(std::move(v));
        }

        template <typename T> result::Result<std::vector<T>> read_vec() {
                const auto size = read<uint32_t>();
                if (size.failed())
                        return result::err(size.error());
                std::vector<T> v;
                v.reserve(size.value());
                for (uint32_t i = 0; i < size.value(); ++i) {
                        auto item = read<T>();
                        if (item.failed())
                                return result::err(item.error());
                        v.push_back(std::move(item).value());
                }
                return result::ok(std::move(v));
        }

        template <typename T, size_t N>
        result::Result<std::array<T, N>> read_array() {
                std::array<T, N> v;
                for (auto &item : v) {
                        auto r = read<T>();
                        if (r.failed())
                                return result::err(r.error());
                        item = std::move(r).value();
                }
                return result::ok(std::move(v));
        }

        template <typename A, typename B>
        result::Result<std::pair<A, B>> read_pair() {
                auto a = read<A>();
                if (a.failed())
                        return result::err(a.error());
                auto b = read<B>();
                if (b.failed())
                        return result::err(b.error());
                return result::ok(std::pair<A, B>{std::move(a).value(),
                                                  std::move(b).value()});
        }

        template <typename T> result::Result<std::optional<T>> read_optional() {
                const auto has_value = read<uint8_t>();
                if (has_value.failed())
                        return result::err(has_value.error());
                if (!has_value.value())
                        return result::ok(std::optional<T>{});
                auto v = read<T>();
                if (v.failed())
                        return result::err(v.error());
                return result::ok(std::optional<T>{std::move(v).value()});
        }
};

template <typename T>
concept Serializable = requires(T t, DataView buf) {
        { t.serialize() } -> std::same_as<Data>;
        { T::deserialize(buf) } -> std::same_as<result::Result<T>>;
};
} // namespace serializer
