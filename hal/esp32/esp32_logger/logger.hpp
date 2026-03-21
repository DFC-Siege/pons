#pragma once

#include "i_logger.hpp"

namespace logging {
class Logger : public BaseLogger {
      public:
        // INFO: ESP logging automatically appends newlines so print and println
        // are the same
        virtual void print(LogLevel level, std::string_view tag,
                           std::string_view value) override;
        virtual void println(LogLevel level, std::string_view tag,
                             std::string_view value) override;
};
} // namespace logging
