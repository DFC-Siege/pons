#pragma once

#include <cassert>
#include <memory>
#include <mutex>

#include "mutex.hpp"
#include "platform_mutex.hpp"

#include "i_logger.hpp"

namespace logging {
class NullLogger : public BaseLogger {
      public:
        void print(LogLevel, std::string_view, std::string_view) override{};
        void println(LogLevel, std::string_view, std::string_view) override{};
};

template <locking::Mutex M = DefaultMutex> class Logger {
      public:
        static ILogger &instance() {
                std::lock_guard<M> lock(mutex);
                assert(logger != nullptr && "No logger set");
                return *logger;
        }

        static void set(std::unique_ptr<ILogger> l) {
                std::lock_guard<M> lock(mutex);
                logger = std::move(l);
        }

      private:
        static inline M mutex;
        static inline std::unique_ptr<ILogger> logger =
            std::make_unique<NullLogger>();
};

inline ILogger &logger() {
        return Logger<>::instance();
}

inline void set_logger(std::unique_ptr<ILogger> l) {
        Logger<>::set(std::move(l));
}
} // namespace logging
