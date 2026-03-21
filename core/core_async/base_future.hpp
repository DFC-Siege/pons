#pragma once

#include <algorithm>
#include <cstdint>

#include "i_future.hpp"
#include "semaphore_concept.hpp"

namespace async {

template <typename T, SemaphoreConcept Sem>
class BaseFuture : public IFuture<T> {
      public:
        BaseFuture() = default;
        BaseFuture(const BaseFuture &) = delete;
        BaseFuture &operator=(const BaseFuture &) = delete;

        void set_value(T val) {
                value = std::move(val);
                sem.give();
        }

        T get() override {
                sem.take();
                return std::move(value);
        }

        bool wait_for(uint32_t timeout_ms) override {
                return sem.take(timeout_ms);
        }

        bool is_ready() override {
                return wait_for(0);
        }

      private:
        Sem sem;
        T value;
};

} // namespace async
