#pragma once

#include <memory>

#include "i_logger.hpp"

namespace logging {
class NullLogger : public BaseLogger {
      public:
        void print(LogLevel, std::string_view, std::string_view) override{};
        void println(LogLevel, std::string_view, std::string_view) override{};
};

class Logger {
      public:
        static ILogger &instance();

        static void set(std::unique_ptr<ILogger> l);

      private:
        static std::unique_ptr<ILogger> logger;
};

inline ILogger &logger() {
        return Logger::instance();
}
} // namespace logging
