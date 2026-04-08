#include <cassert>

#include "i_logger.hpp"
#include "logger.hpp"

namespace logging {
std::mutex Logger::mutex;
std::unique_ptr<ILogger> Logger::logger = std::make_unique<NullLogger>();

ILogger &Logger::instance() {
        std::lock_guard<std::mutex> lock(mutex);
        assert(logger != nullptr && "No logger set");
        return *logger;
}

void Logger::set(std::unique_ptr<ILogger> l) {
        std::lock_guard<std::mutex> lock(mutex);
        logger = std::move(l);
}
} // namespace logging
