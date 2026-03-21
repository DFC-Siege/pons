#pragma once

#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace async {
class Esp32Semaphore {
      public:
        Esp32Semaphore() : handle(xSemaphoreCreateBinary()) {
        }
        ~Esp32Semaphore() {
                vSemaphoreDelete(handle);
        }

        Esp32Semaphore(const Esp32Semaphore &) = delete;
        Esp32Semaphore &operator=(const Esp32Semaphore &) = delete;

        void give() {
                xSemaphoreGive(handle);
        }

        void give_from_isr() {
                BaseType_t higher_priority_task_woken = pdFALSE;
                xSemaphoreGiveFromISR(handle, &higher_priority_task_woken);
                portYIELD_FROM_ISR(higher_priority_task_woken);
        }

        bool take() {
                return xSemaphoreTake(handle, portMAX_DELAY) == pdTRUE;
        }

        bool take(uint32_t timeout_ms) {
                return xSemaphoreTake(handle, pdMS_TO_TICKS(timeout_ms)) ==
                       pdTRUE;
        }

      private:
        SemaphoreHandle_t handle;
};
} // namespace async
