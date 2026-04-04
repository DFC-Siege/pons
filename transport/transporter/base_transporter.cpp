#pragma once

#include "base_transporter.hpp"
#include "result.hpp"

namespace transport {
void BaseTransporter::set_receiver(ReceiveCallback callback) {
        this->callback = callback;
}

[[nodiscard]] result::Result<bool> BaseTransporter::try_callback(Data &&data) {
        if (!callback.has_value()) {
                return result::err("callback not set");
        }
        callback.value()(result::ok(std::move(data)));
        return result::ok();
}
} // namespace transport
