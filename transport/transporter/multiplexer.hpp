#pragma once
#include <cassert>
#include <cstring>
#include <functional>
#include <mutex>
#include <unordered_map>

#include "i_logger.hpp"
#include "logger.hpp"
#include "transporter.hpp"

namespace transport {

using TransporterId = uint8_t;
using SendFn = std::function<result::Result<bool>(Data &&)>;

template <Transporter T> class Multiplexer {
      public:
        template <typename U>
        explicit Multiplexer(
            T &transporter,
            std::unordered_map<TransporterId, std::reference_wrapper<U>> map)
            : transporter(transporter) {
                for (auto &[id, ref] : map) {
                        send_fns.emplace(id, [&r = ref.get()](Data &&d) {
                                return r.send(std::move(d));
                        });
                }
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

        class Channel {
              public:
                Channel(Multiplexer &parent, TransporterId id)
                    : parent(parent), id(id) {
                }

                result::Result<bool> send(Data &&data) {
                        return parent.send_with_id(id, std::move(data));
                }

                void set_receiver(ReceiveCallback callback) {
                        parent.set_channel_receiver(id, std::move(callback));
                }

                MTU get_mtu() const {
                        const auto mtu = parent.transporter.get_mtu();
                        assert(mtu > 0 && "attempted to use mtu <= 0");
                        return mtu - 1;
                }

              private:
                Multiplexer &parent;
                TransporterId id;
        };

        void set_channel_receiver(TransporterId id, ReceiveCallback callback) {
                const std::scoped_lock lock(mutex);
                channel_receivers[id] = std::move(callback);
        }

        result::Result<bool> send_with_id(TransporterId id, Data &&data) {
                const std::scoped_lock lock(mutex);
                auto it = send_fns.find(id);
                if (it == send_fns.end()) {
                        return result::err("transporter not registered");
                }
                data.insert(data.begin(), id);
                return it->second(std::move(data));
        }

        result::Result<Channel> get_channel(TransporterId id) {
                if (!send_fns.contains(id)) {
                        return result::err("transporter not registered");
                }
                return result::ok(Channel(*this, id));
        }

      private:
        static constexpr auto TAG = "Multiplexer";
        friend class Channel;

        T &transporter;
        std::unordered_map<TransporterId, SendFn> send_fns;
        std::unordered_map<TransporterId, ReceiveCallback> channel_receivers;
        std::mutex mutex;

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
                if (auto it = channel_receivers.find(id);
                    it != channel_receivers.end()) {
                        it->second(result::ok(std::move(data)));
                        return;
                }
                logging::logger().println(logging::LogLevel::Error, TAG,
                                          "transporter not found");
        }
};

} // namespace transport
