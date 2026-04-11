#pragma once
#include <cassert>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

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
        explicit Multiplexer(std::unique_ptr<T> transporter)
            : transporter(std::move(transporter)) {
                this->transporter->set_receiver(
                    [this](result::Result<Data> result) {
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

                ~InnerChannel() override {
                        parent.unregister_receiver(id);
                }

                result::Try send(Data &&data) override {
                        return parent.send_with_id(id, std::move(data));
                }

                MTU get_mtu() const override {
                        const auto mtu = parent.transporter->get_mtu();
                        assert(mtu >= sizeof(TransporterId));
                        return mtu - sizeof(TransporterId);
                }

                void set_receiver(ReceiveCallback callback) override {
                        BaseTransporter::set_receiver(std::move(callback));
                        parent.register_receiver(
                            id, [this](result::Result<Data> data) {
                                    const auto result = try_callback(data);
                                    if (result.failed()) {
                                            logging::logger().println(
                                                logging::LogLevel::Error, TAG,
                                                result.error());
                                    }
                            });
                }

              private:
                static constexpr auto TAG = "InnerChannel";
                Multiplexer &parent;
                TransporterId id;
        };

        static_assert(Transporter<InnerChannel>);

        std::unique_ptr<InnerChannel> create_inner_channel(TransporterId id) {
                const std::lock_guard<M> lock(mutex);
                assert(!registered_ids.contains(id) && "duplicate channel id");
                registered_ids.insert(id);
                return std::make_unique<InnerChannel>(*this, id);
        }

      private:
        static constexpr auto TAG = "Multiplexer";
        friend class InnerChannel;

        std::unique_ptr<T> transporter;
        std::unordered_map<TransporterId, ReceiveCallback> receivers;
        std::unordered_set<TransporterId> registered_ids;
        M mutex;

        void register_receiver(TransporterId id, ReceiveCallback callback) {
                const std::lock_guard<M> lock(mutex);
                receivers[id] = std::move(callback);
        }

        void unregister_receiver(TransporterId id) {
                const std::lock_guard<M> lock(mutex);
                receivers.erase(id);
                registered_ids.erase(id);
        }

        result::Try send_with_id(TransporterId id, Data &&data) {
                {
                        const std::lock_guard<M> lock(mutex);
                        if (!registered_ids.contains(id)) {
                                return result::err("channel not registered");
                        }
                }

                data.insert(data.begin(), id);
                return transporter->send(std::move(data));
        }

        void handle_receive(Data &&data) {
                if (data.size() < sizeof(TransporterId)) {
                        return;
                }

                TransporterId id;
                std::memcpy(&id, data.data(), sizeof(TransporterId));
                data.erase(data.begin(), data.begin() + sizeof(TransporterId));

                ReceiveCallback callback;
                {
                        const std::lock_guard<M> lock(mutex);
                        auto it = receivers.find(id);
                        if (it == receivers.end()) {
                                logging::logger().println(
                                    logging::LogLevel::Error, TAG,
                                    "channel not found: " + std::to_string(id));
                                return;
                        }
                        callback = it->second;
                }

                callback(result::ok(std::move(data)));
        }
};

} // namespace transport
