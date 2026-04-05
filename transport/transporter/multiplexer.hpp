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
#include "result.hpp"
#include "transport_data.hpp"
#include "transporter.hpp"

namespace transport {

using TransporterId = uint8_t;
using SendFn = std::function<result::Result<bool>(Data &&)>;

template <Transporter T> class Multiplexer {
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

        class Channel : public BaseTransporter {
              public:
                Channel(Multiplexer &parent, T &transporter, TransporterId id)
                    : parent(parent), transporter(transporter), id(id) {
                        transporter.set_receiver(
                            [this](result::Result<Data> data) {
                                    const auto result = try_callback(data);
                                    if (result.failed()) {
                                            logging::logger().println(
                                                logging::LogLevel::Error, TAG,
                                                result.error());
                                    }
                            });
                }

                result::Result<bool> send(Data &&data) {
                        logging::logger().println(
                            logging::LogLevel::Debug, TAG,
                            "channel: " + std::to_string(id) +
                                " sending packet, size: " +
                                std::to_string(data.size()));
                        return transporter.send(std::move(data));
                }

                MTU get_mtu() const {
                        return transporter.get_mtu();
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
                static constexpr auto TAG = "Channel";
                Multiplexer &parent;
                T &transporter;
                TransporterId id;
        };

        class InnerChannel : public BaseTransporter {
              public:
                InnerChannel(Multiplexer &parent, TransporterId id)
                    : parent(parent), id(id) {
                }

                result::Result<bool> send(Data &&data) override {
                        logging::logger().println(
                            logging::LogLevel::Debug, TAG,
                            "inner channel: " + std::to_string(id) +
                                " forwarding to parent, size: " +
                                std::to_string(data.size()));
                        return parent.send_with_id(id, std::move(data));
                }

                MTU get_mtu() const override {
                        const auto mtu = parent.transporter.get_mtu();
                        assert(mtu >= sizeof(TransporterId) &&
                               "attempted to use mtu <= 0");
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
                TransporterId id;
                Multiplexer &parent;
        };

        InnerChannel &create_inner_channel(TransporterId id) {
                const std::scoped_lock lock(mutex);
                auto [it, _] = inner_channels.emplace(
                    id, std::make_unique<InnerChannel>(*this, id));
                return *it->second;
        }

        BaseTransporter &
        register_channel(TransporterId id,
                         std::unique_ptr<BaseTransporter> transporter) {
                const std::scoped_lock lock(mutex);
                auto [it, _] = channels.emplace(id, std::move(transporter));
                return *it->second;
        }

      private:
        static constexpr auto TAG = "Multiplexer";
        friend class Channel;
        friend class InnerChannel;

        T &transporter;
        std::unordered_map<TransporterId, std::unique_ptr<InnerChannel>>
            inner_channels;
        std::unordered_map<TransporterId, std::unique_ptr<BaseTransporter>>
            channels;
        std::mutex mutex;

        result::Result<bool> send_with_id(TransporterId id, Data &&data) {
                logging::logger().println(
                    logging::LogLevel::Debug, TAG,
                    "multiplexer: wrapping data for channel: " +
                        std::to_string(id) +
                        " size: " + std::to_string(data.size()));

                {
                        const std::scoped_lock lock(mutex);
                        auto it = inner_channels.find(id);
                        if (it == inner_channels.end()) {
                                logging::logger().println(
                                    logging::LogLevel::Error, TAG,
                                    "multiplexer: failed to find channel: " +
                                        std::to_string(id));
                                return result::err("channel not registered");
                        }
                }

                data.insert(data.begin(), id);
                return transporter.send(std::move(data));
        }

        void handle_receive(Data &&data) {
                if (data.size() < sizeof(TransporterId)) {
                        logging::logger().println(
                            logging::LogLevel::Error, TAG,
                            "data is smaller than minimal size");
                        return;
                }

                TransporterId id;
                std::memcpy(&id, data.data(), sizeof(TransporterId));
                data.erase(data.begin(), data.begin() + sizeof(TransporterId));

                logging::logger().println(
                    logging::LogLevel::Debug, TAG,
                    "received packet for channel: " + std::to_string(id) +
                        " size: " + std::to_string(data.size()));

                const std::scoped_lock lock(mutex);
                auto it = inner_channels.find(id);
                if (it == inner_channels.end()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  "channel not found: " +
                                                      std::to_string(id));
                        return;
                }

                it->second->receive(result::ok(std::move(data)));
        }
};

} // namespace transport
