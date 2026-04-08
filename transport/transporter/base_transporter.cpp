#include "base_transporter.hpp"
#include "logger.hpp"
#include "result.hpp"

namespace transport {
void BaseTransporter::set_receiver(ReceiveCallback callback) {
        if (this->callback.has_value()) {
                logging::logger().println(
                    logging::LogLevel::Warning, "BaseTransporter",
                    "receiver callback replaced");
        }
        this->callback = callback;
}

result::Try BaseTransporter::try_callback(result::Result<Data> data) {
        if (!callback.has_value()) {
                return result::err("callback not set");
        }
        callback.value()(std::move(data));
        return result::ok();
}
} // namespace transport
