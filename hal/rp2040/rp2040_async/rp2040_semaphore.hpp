#pragma once

#include <cstdint>
#include <pico/sem.h>

namespace async {
class RP2040Semaphore {
      public:
        RP2040Semaphore() {
                sem_init(&sem, 0, 1);
        }

        void give() {
                sem_release(&sem);
        }

        void give_from_isr() {
                sem_release(&sem);
        }

        bool take() {
                sem_acquire_blocking(&sem);
                return true;
        }

        bool take(uint32_t timeout_ms) {
                return sem_acquire_timeout_ms(&sem, timeout_ms);
        }

      private:
        semaphore_t sem;
};
} // namespace async
