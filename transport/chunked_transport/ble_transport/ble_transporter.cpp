#include <cstdint>

#include "ble_transporter.hpp"
#include "chunked_sender.hpp"
#include "chunked_transporter.hpp"
#include "logger.hpp"
#include "result.hpp"

namespace transport {
BleTransporter::BleTransporter(uint16_t mtu, uint8_t max_attempts,
                               ble::IBleHal &ble_hal)
    : ChunkedTransporter(mtu, max_attempts), ble_hal(ble_hal) {
        ble_hal.on_receive([this](std::span<const uint8_t> data) {
                const auto result = feed(data);
                if (result.failed()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  result.error());
                        return;
                }
                const auto feed_result = result.value();
                if (feed_result.result.failed() &&
                    error_callbacks.find(feed_result.session_id) !=
                        error_callbacks.end()) {
                        error_callbacks[feed_result.session_id](
                            feed_result.result.error());
                }
        });
}

result::Result<bool>
BleTransporter::concrete_send(std::span<const uint8_t> data) {
        return this->ble_hal.send(data);
}
} // namespace transport
