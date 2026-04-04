#pragma once
#include <cassert>
#include <cstring>
#include <functional>
#include <mutex>
#include <unordered_map>

#include "base_transporter.hpp"
#include "i_logger.hpp"
#include "logger.hpp"
#include "result.hpp"
#include "transporter.hpp"

namespace transport {

using TransporterId = uint8_t;
using SendFn = std::function<result::Result<bool>(Data &&)>;

template <Transporter T, Transporter U> class Multiplexer {
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
                const TransporterId id;

                Channel(Multiplexer &parent, TransporterId id)
                    : parent(parent), id(id) {
                }

                result::Result<bool> send(Data &&data) {
                        return parent.send_with_id(id, std::move(data));
                }

                MTU get_mtu() const {
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
                static constexpr auto TAG = "Channel";
                Multiplexer &parent;
        };

        Channel &create_channel(TransporterId id) {
                const std::scoped_lock lock(mutex);
                auto [it, _] =
                    channels.emplace(id, std::make_unique<Channel>(*this, id));
                return *it->second;
        }

      private:
        static constexpr auto TAG = "Multiplexer";
        friend class Channel;

        T &transporter;
        std::unordered_map<TransporterId, std::unique_ptr<Channel>> channels;
        std::mutex mutex;

        result::Result<bool> send_with_id(TransporterId id, Data &&data) {
                const std::scoped_lock lock(mutex);
                auto it = channels.find(id);
                if (it == channels.end()) {
                        return result::err("channel not registered");
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

                const std::scoped_lock lock(mutex);
                auto it = channels.find(id);
                if (it == channels.end()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  "channel not found");
                        return;
                }

                it->second->receive(result::ok(std::move(data)));
        }
};

} // namespace transport
