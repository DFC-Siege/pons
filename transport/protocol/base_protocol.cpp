#include "base_protocol.hpp"

namespace transport {
void BaseProtocol::set_receiver(ReceiveCallback callback) {
        this->callback = callback;
}

result::Result<bool> BaseProtocol::handle_receive(Data data) {
        if (!callback.has_value()) {
                return result::err("callback not set");
        }

        callback.value()(handle_data(data));
        return result::ok();
}
} // namespace transport
