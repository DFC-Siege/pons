#pragma once

#include <concepts>
#include <cstdint>

namespace async {
template <typename Impl>
concept SemaphoreConcept = requires(Impl s) {
        { s.give() } -> std::same_as<void>;
        { s.give_from_isr() } -> std::same_as<void>;
        { s.take() } -> std::same_as<bool>;
        { s.take(uint32_t{}) } -> std::same_as<bool>;
};
} // namespace async
