#include "base_transporter.hpp"
#include "result.hpp"

namespace transport {
void BaseTransporter::set_receiver(ReceiveCallback callback) {
        this->callback = callback;
}

result::Status BaseTransporter::try_callback(result::Result<Data> data) {
        if (!callback.has_value()) {
                return result::err("callback not set");
        }
        callback.value()(std::move(data));
        return result::ok();
}
} // namespace transport
