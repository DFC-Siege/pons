#pragma once

#ifdef PICO_BUILD
#include <pico/critical_section.h>

struct PicoMutex {
        PicoMutex() {
                critical_section_init(&cs);
        }
        ~PicoMutex() {
                critical_section_deinit(&cs);
        }
        void lock() {
                critical_section_enter_blocking(&cs);
        }
        void unlock() {
                critical_section_exit(&cs);
        }

      private:
        critical_section_t cs;
};

using DefaultMutex = PicoMutex;
#else
#include <mutex>
using DefaultMutex = std::mutex;
#endif
