#pragma once

#include "i_logger.hpp"

namespace logging {
class Logger : public BaseLogger {
      public:
        // INFO: ESP logging automatically appends newlines so print and println
        // are the same
        void print(LogLevel level, Tag tag, std::string_view value) override;
        void println(LogLevel level, Tag tag, std::string_view value) override;
};
} // namespace logging
