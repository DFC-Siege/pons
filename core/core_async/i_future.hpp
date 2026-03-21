#pragma once

#include <cstdint>

namespace async {
template <typename T> class IFuture {
      public:
        virtual ~IFuture() = default;
        virtual T get() = 0;
        virtual bool wait_for(uint32_t timeout_ms) = 0;
        virtual bool is_ready() = 0;
};
} // namespace async
