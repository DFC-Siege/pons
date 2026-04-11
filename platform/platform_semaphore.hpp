#pragma once

#include <chrono>

#ifdef PICO_BUILD
#include <pico/sem.h>

struct PicoSemaphore {
        PicoSemaphore() {
                sem_init(&sem, 0, 1);
        }
        ~PicoSemaphore() = default;
        PicoSemaphore(const PicoSemaphore &) = delete;
        PicoSemaphore &operator=(const PicoSemaphore &) = delete;

        bool acquire(std::chrono::milliseconds timeout) {
                return sem_acquire_timeout_ms(&sem,
                                              static_cast<uint32_t>(timeout.count()));
        }

        void release() {
                sem_release(&sem);
        }

      private:
        semaphore_t sem;
};

using DefaultSemaphore = PicoSemaphore;
#else
#include <condition_variable>
#include <mutex>

struct StdSemaphore {
        StdSemaphore() = default;
        StdSemaphore(const StdSemaphore &) = delete;
        StdSemaphore &operator=(const StdSemaphore &) = delete;

        bool acquire(std::chrono::milliseconds timeout) {
                std::unique_lock<std::mutex> lock(mutex);
                return cv.wait_for(lock, timeout, [&] { return signaled; });
        }

        void release() {
                {
                        std::lock_guard<std::mutex> lock(mutex);
                        signaled = true;
                }
                cv.notify_one();
        }

      private:
        std::mutex mutex;
        std::condition_variable cv;
        bool signaled = false;
};

using DefaultSemaphore = StdSemaphore;
#endif
