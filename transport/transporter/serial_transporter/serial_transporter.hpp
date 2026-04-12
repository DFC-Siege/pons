#pragma once

#include "base_transporter.hpp"
#include "logger.hpp"
#include "result.hpp"
#include "serial_hal.hpp"
#include "transporter/base_transporter.hpp"

namespace transport {
template <serial::SerialHal S>
class SerialTransporter : public BaseTransporter {
      public:
        SerialTransporter(S &serial_hal, MTU mtu)
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
                        const auto result =
                            this->try_callback(result::ok(data));
                        if (result.failed()) {
                                logging::logger().println(
                                    logging::LogLevel::Error, TAG,
                                    result.error());
                        }
                });
        }

        result::Try send(Data &&data) {
                if (data.size() > mtu) {
                        logging::logger().println(
                            logging::LogLevel::Error, TAG,
                            "send failed: data size " +
                                std::to_string(data.size()) + " exceeds MTU " +
                                std::to_string(mtu));
                        return result::err(
                            "data bigger(" + std::to_string(data.size()) +
                            ") than mtu: " + std::to_string(mtu));
                }

                logging::logger().println(
                    logging::LogLevel::Debug, TAG,
                    "serial: sending " + std::to_string(data.size()) +
                        " bytes: " + [&] {
                                std::string hex;
                                for (auto b : data)
                                        hex += std::to_string(b) + " ";
                                return hex;
                        }());

                return this->serial_hal.send(std::move(data));
        }

        MTU get_mtu() const {
                return mtu;
        }

      private:
        static constexpr auto TAG = "SerialTransporter";
        S &serial_hal;
        MTU mtu;
};
} // namespace transport
