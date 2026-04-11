#pragma once

#include <chrono>
#include <concepts>

namespace locking {
template <typename S>
concept Semaphore = requires(S s, std::chrono::milliseconds timeout) {
        { s.acquire(timeout) } -> std::same_as<bool>;
        { s.release() } -> std::same_as<void>;
};
} // namespace locking
