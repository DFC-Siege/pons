#pragma once

#include "transporter/base_transporter.hpp"
#include "transporter/transporter.hpp"

namespace transport {
template <Transporter T> class ChunkedTransporter : public BaseTransporter {
      public:
        ChunkedTransporter(T &transporter) : transporter(transporter) {
                transporter.set_receiver([this](result::Result<Data> data) {
                        if (data.failed()) {
                                // TODO: Add log
                                return;
                        }
                        handle_data(std::move(data));
                });
        };

        result::Result<bool> send(Data &&data) override {
                // TODO: Actually chunk it
                transporter.send(std::move(data));
                return result::err("not implemented");
        }

        MTU get_mtu() const override {
                return transporter.get_mtu();
        }

      private:
        T &transporter;

        void handle_data(Data &&data) {
                // TODO: Handle chunks
                const auto result = try_callback(std::move(data));
                if (result.failed()) {
                        // TODO: Add log
                }
        }
};
} // namespace transport
