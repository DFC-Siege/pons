#include "console_logger.hpp"
#include "i_logger.hpp"

namespace logging {
void ConsoleLogger::print(LogLevel level, std::string_view tag,
                          std::string_view value) {
        if (!check_level(level)) {
                return;
        }

        auto level_str = level_to_string(level);
        ::printf("[%.*s] %.*s: %.*s", (int)level_str.size(), level_str.data(),
                 (int)tag.size(), tag.data(), (int)value.size(), value.data());
}

void ConsoleLogger::println(LogLevel level, std::string_view tag,
                            std::string_view value) {
        if (!check_level(level)) {
                return;
        }

        print(level, tag, value);
        ::printf("\n");
}

bool ConsoleLogger::check_level(LogLevel level) {
        return this->level >= level;
}
} // namespace logging
