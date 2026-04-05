#pragma once

#include "base_transporter.hpp"
#include "i_logger.hpp"
#include "logger.hpp"
#include "result.hpp"
#include "transporter.hpp"

namespace transport {
template <Transporter T> class DirectTransporter : public BaseTransporter {
      public:
        explicit DirectTransporter(T &transporter) : transporter(transporter) {
                transporter.set_receiver([this](result::Result<Data> result) {
                        const auto callback_result = try_callback(result);
                        if (callback_result.failed()) {
                                logging::logger().println(
                                    logging::LogLevel::Error, TAG,
                                    result.error());
                                return;
                        }
                });
        }

        result::Status send(Data &&data) override {
                return transporter.send(std::move(data));
        }

        MTU get_mtu() const override {
                return transporter.get_mtu();
        }

      private:
        static constexpr auto TAG = "DirectTransporter";
        T &transporter;
};
} // namespace transport
