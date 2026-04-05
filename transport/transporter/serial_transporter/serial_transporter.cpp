#include "serial_transporter.hpp"
#include "i_logger.hpp"
#include "logger.hpp"
#include "result.hpp"
#include "transporter/base_transporter.hpp"
#include <cstdint>
#include <string>

namespace transport {

SerialTransporter::SerialTransporter(serial::ISerialHal &serial_hal, MTU mtu)
    : BaseTransporter(), serial_hal(serial_hal), mtu(mtu) {
        serial_hal.on_receive([this](Data data) {
                logging::logger().println(
                    logging::LogLevel::Debug, TAG,
                    "received " + std::to_string(data.size()) +
                        " bytes: " + [&] {
                                std::string hex;
                                for (auto b : data)
                                        hex += std::to_string(b) + " ";
                                return hex;
                        }());
                const auto result = this->try_callback(result::ok(data));
                if (result.failed()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  result.error());
                }
        });
}

result::Result<bool> SerialTransporter::send(Data &&data) {
        if (data.size() > mtu) {
                return result::err("data bigger(" +
                                   std::to_string(data.size()) +
                                   ") than mtu: " + std::to_string(mtu));
        }
        logging::logger().println(
            logging::LogLevel::Debug, TAG,
            "sending " + std::to_string(data.size()) + " bytes: " + [&] {
                    std::string hex;
                    for (auto b : data)
                            hex += std::to_string(b) + " ";
                    return hex;
            }());
        return this->serial_hal.send(std::move(data));
}

MTU SerialTransporter::get_mtu() const {
        return mtu;
}

} // namespace transport
