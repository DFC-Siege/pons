#pragma once

#include "i_logger.hpp"

namespace logging {
class ConsoleLogger : public BaseLogger {
      public:
        void print(LogLevel level, std::string_view tag,
                   std::string_view value) override;
        void println(LogLevel level, std::string_view tag,
                     std::string_view value) override;

      private:
        bool check_level(LogLevel level);
};
} // namespace logging
