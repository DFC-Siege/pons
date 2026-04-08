#pragma once
#include <cassert>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "base_transporter.hpp"
#include "i_logger.hpp"
#include "logger.hpp"
#include "mutex.hpp"
#include "platform_mutex.hpp"
#include "result.hpp"
#include "transport_data.hpp"
#include "transporter.hpp"

namespace transport {

using TransporterId = uint8_t;

template <Transporter T, locking::Mutex M = DefaultMutex> class Multiplexer {
      public:
        explicit Multiplexer(T &transporter) : transporter(transporter) {
                transporter.set_receiver([this](result::Result<Data> result) {
                        if (result.failed()) {
                                logging::logger().println(
                                    logging::LogLevel::Error, TAG,
                                    result.error());
                                return;
                        }
                        handle_receive(std::move(result).value());
                });
        }

        class InnerChannel : public BaseTransporter {
              public:
                InnerChannel(Multiplexer &parent, TransporterId id)
                    : parent(parent), id(id) {
                }

                result::Try send(Data &&data) override {
                        return parent.send_with_id(id, std::move(data));
                }

                MTU get_mtu() const override {
                        const auto mtu = parent.transporter.get_mtu();
                        assert(mtu >= sizeof(TransporterId));
                        return mtu - sizeof(TransporterId);
                }

                void receive(result::Result<Data> data) {
                        const auto result = try_callback(data);
                        if (result.failed()) {
                                logging::logger().println(
                                    logging::LogLevel::Error, TAG,
                                    result.error());
                        }
                }

              private:
                static constexpr auto TAG = "InnerChannel";
                Multiplexer &parent;
                TransporterId id;
        };

        static_assert(Transporter<InnerChannel>);

        InnerChannel &create_inner_channel(TransporterId id) {
                const std::lock_guard<M> lock(mutex);
                auto [it, inserted] = inner_channels.emplace(
                    id, std::make_unique<InnerChannel>(*this, id));
                assert(inserted && "duplicate channel id");
                return *it->second;
        }

      private:
        static constexpr auto TAG = "Multiplexer";
        friend class InnerChannel;

        T &transporter;
        std::unordered_map<TransporterId, std::unique_ptr<InnerChannel>>
            inner_channels;
        M mutex;

        result::Try send_with_id(TransporterId id, Data &&data) {
                {
                        const std::lock_guard<M> lock(mutex);
                        if (inner_channels.find(id) == inner_channels.end()) {
                                return result::err("channel not registered");
                        }
                }

                data.insert(data.begin(), id);
                return transporter.send(std::move(data));
        }

        void handle_receive(Data &&data) {
                if (data.size() < sizeof(TransporterId)) {
                        return;
                }

                TransporterId id;
                std::memcpy(&id, data.data(), sizeof(TransporterId));
                data.erase(data.begin(), data.begin() + sizeof(TransporterId));

                InnerChannel *channel_ptr = nullptr;
                {
                        const std::lock_guard<M> lock(mutex);
                        auto it = inner_channels.find(id);
                        if (it != inner_channels.end()) {
                                channel_ptr = it->second.get();
                        }
                }

                if (channel_ptr) {
                        channel_ptr->receive(result::ok(std::move(data)));
                } else {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  "channel not found: " +
                                                      std::to_string(id));
                }
        }
};

} // namespace transport
