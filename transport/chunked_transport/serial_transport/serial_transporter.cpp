#include <cstdint>

#include "chunked_sender.hpp"
#include "chunked_transporter.hpp"
#include "logger.hpp"
#include "result.hpp"
#include "serial_transporter.hpp"

namespace transport {
SerialTransporter::SerialTransporter(uint16_t mtu,
                                     serial::ISerialHal &serial_hal)
    : ChunkedTransporter(mtu), serial_hal(serial_hal) {
        serial_hal.on_receive([this](std::span<const uint8_t> data) {
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
SerialTransporter::concrete_send(std::span<const uint8_t> data) {
        return this->serial_hal.send(data);
}
} // namespace transport
