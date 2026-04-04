#pragma once
#include "i_logger.hpp"
#include "logger.hpp"
#include "transporter/transporter.hpp"
#include <cstring>
#include <functional>
#include <mutex>
#include <unordered_map>
namespace transport {
using TransporterId = uint8_t;
template <Transporter T> class Multiplexer {
      public:
        explicit Multiplexer(
            T &transporter,
            std::unordered_map<TransporterId, std::reference_wrapper<T>>
                transporters)
            : transporter(transporter), transporters(std::move(transporters)) {
                transporter.set_receiver([this](result::Result<Data> result) {
                        if (result.failed()) {
                                logging::logger().println(
                                    logging::LogLevel::Error, TAG,
                                    result.error());
                                return;
                        }
                        this->handle_receive(std::move(result).value());
                });
        }

        class Channel {
              public:
                Channel(Multiplexer &parent, TransporterId id)
                    : parent(parent), id(id) {
                }
                result::Result<bool> send(Data &&data) {
                        return parent.send_with_id(this->id, std::move(data));
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

        result::Result<bool> send_with_id(TransporterId id, Data &&data) {
                const std::scoped_lock lock(mutex);
                const auto wrap_result = wrap_data(id, std::move(data));
                if (wrap_result.failed()) {
                        return result::err(wrap_result.error());
                }
                auto wrapped_data = std::move(wrap_result).value();
                return transporter.send(std::move(wrapped_data));
        }

        result::Result<Channel> get_channel(TransporterId id) {
                if (!transporters.contains(id)) {
                        return result::err("transporter not registered");
                }
                return result::ok(Channel(*this, id));
        }

      private:
        static constexpr auto TAG = "Multiplexer";
        friend class Channel;
        T &transporter;
        std::unordered_map<TransporterId, std::reference_wrapper<T>>
            transporters;
        std::mutex mutex;

        result::Result<T &> try_get_transporter(TransporterId id) {
                auto it = transporters.find(id);
                if (it == transporters.end()) {
                        return result::err("transporter not found");
                }
                return result::ok_ref(it->second.get());
        }

        result::Result<Data> wrap_data(TransporterId id, Data &&data) {
                if (!transporters.contains(id)) {
                        return result::err("transporter not found");
                }
                data.insert(data.begin(), id);
                return result::ok(std::move(data));
        }

        void handle_receive(Data &&data) {
                const size_t n = sizeof(TransporterId);
                if (data.size() < n) {
                        logging::logger().println(
                            logging::LogLevel::Error, TAG,
                            "data is smaller than minimal size");
                        return;
                }
                TransporterId id;
                std::memcpy(&id, data.data(), sizeof(TransporterId));
                data.erase(data.begin(), data.begin() + n);

                const auto it = transporters.find(id);
                if (it == transporters.end()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  "transporter not found");
                        return;
                }
                it->second.get().send(std::move(data));
        }
};
} // namespace transport
