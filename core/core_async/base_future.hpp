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
                if (!ready)
                        sem.take();
                ready = true;
                return std::move(value);
        }

        bool wait_for(uint32_t timeout_ms) override {
                if (ready)
                        return true;
                const auto result = sem.take(timeout_ms);
                if (result)
                        ready = true;
                return result;
        }

        bool is_ready() override {
                if (ready)
                        return true;
                const auto result = sem.take(0);
                if (result)
                        ready = true;
                return result;
        }

      private:
        Sem sem;
        T value;
        bool ready = false;
};

} // namespace async
