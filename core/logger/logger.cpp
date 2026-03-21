#include <cassert>

#include "i_logger.hpp"
#include "logger.hpp"

namespace logging {
std::unique_ptr<ILogger> Logger::logger = std::make_unique<NullLogger>();

ILogger &Logger::instance() {
        assert(logger != nullptr && "No logger set");
        return *logger;
}

void Logger::set(std::unique_ptr<ILogger> l) {
        logger = std::move(l);
}
} // namespace logging
