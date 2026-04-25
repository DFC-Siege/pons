#include <esp_log.h>
#include <esp_system.h>

#include "esp32_logger.hpp"
#include "i_logger.hpp"

namespace logging {

void Logger::print(LogLevel level, Tag tag, std::string_view value) {
        const char *t = tag.value.data();
        switch (level) {
        case LogLevel::Verbose:
                ESP_LOGV(t, "%.*s", (int)value.size(), value.data());
                break;
        case LogLevel::Debug:
                ESP_LOGD(t, "%.*s", (int)value.size(), value.data());
                break;
        case LogLevel::Info:
                ESP_LOGI(t, "%.*s", (int)value.size(), value.data());
                break;
        case LogLevel::Warning:
                ESP_LOGW(t, "%.*s", (int)value.size(), value.data());
                break;
        case LogLevel::Error:
                ESP_LOGE(t, "%.*s", (int)value.size(), value.data());
                break;
        case LogLevel::Fatal:
                ESP_LOGE(t, "%.*s", (int)value.size(), value.data());
                esp_restart();
                break;
        case LogLevel::None:
                break;
        }
}

void Logger::println(LogLevel level, Tag tag, std::string_view value) {
        print(level, tag, value);
}
} // namespace logging
