#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <vector>

#include "data.hpp"
#include "result.hpp"

namespace transport {
template <typename T>
concept Protocol = requires(T t, Data data) {
        { t.send(data) } -> std::same_as<result::Result<Data>>;
};
} // namespace transport
