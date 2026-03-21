#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace async {
class PosixSemaphore {
      public:
        PosixSemaphore() : count(0) {
        }

        void give() {
                std::unique_lock lock(mutex);
                count++;
                cv.notify_one();
        }

        void give_from_isr() {
                give();
        }

        bool take() {
                std::unique_lock lock(mutex);
                cv.wait(lock, [this] { return count > 0; });
                count--;
                return true;
        }

        bool take(uint32_t timeout_ms) {
                std::unique_lock lock(mutex);
                const auto result =
                    cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                [this] { return count > 0; });
                if (result)
                        count--;
                return result;
        }

      private:
        std::mutex mutex;
        std::condition_variable cv;
        int count;
};
} // namespace async
