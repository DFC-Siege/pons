#pragma once

#include <concepts>

namespace locking {
template <typename M>
concept Mutex = requires(M m) {
        { m.lock() };
        { m.unlock() };
};
} // namespace locking
