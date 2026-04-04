#pragma once

#include <cstring>
#include <mutex>
#include <unordered_map>

#include "protocol.hpp"
#include "transporter/transporter.hpp"

namespace transport {
using ProtocolId = uint8_t;

template <Transporter T, Protocol P> class Multiplexer {
      public:
        explicit Multiplexer(T &transporter,
                             std::unordered_map<ProtocolId, P &> protocols)
            : transporter(transporter), protocols(protocols) {
                transporter.set_receiver([this](Data &&data) {
                        this->handle_receive(std::move(data));
                });
        }

        class Channel {
              public:
                Channel(Multiplexer &parent, ProtocolId id)
                    : parent(parent), id(id) {
                }

                result::Result<bool> send(Data &&data) {
                        return parent.send_with_id(this->id, std::move(data));
                }

                MTU get_mtu() const {
                        const auto mtu = parent.transporter.get_mtu();
                        assert(mtu > 0);

                        return mtu - 1;
                }

              private:
                Multiplexer &parent;
                ProtocolId id;
        };

        result::Result<bool> send_with_id(ProtocolId id, Data &&data) {
                const std::scoped_lock lock(mutex);

                const auto wrap_result = wrap_data(id, std::move(data));
                if (wrap_result.failed()) {
                        return result::err(wrap_result.error());
                }

                auto wrapped_data = std::move(wrap_result.value());
                return transporter.send(std::move(wrapped_data));
        }

        result::Result<Channel> get_channel(ProtocolId id) {
                if (!protocols.contains(id)) {
                        return result::err(
                            "Protocol ID not registered in Multiplexer");
                }
                return result::ok(Channel(*this, id));
        }

      private:
        friend class Channel;
        T &transporter;
        std::unordered_map<ProtocolId, P &> protocols;
        std::mutex mutex;

        result::Result<P &> try_get_protocol(ProtocolId id) {
                auto it = protocols.find(id);
                if (it == protocols.end()) {
                        return result::err("protocol not found");
                }
                return result::ok(it->second);
        }

        result::Result<Data> wrap_data(ProtocolId id, Data &&data) {
                if (!protocols.contains(id)) {
                        return result::err("protocol not found");
                }
                data.insert(data.begin(), id);

                return result::ok(std::move(data));
        }

        void handle_receive(Data &&data) {
                size_t n = sizeof(ProtocolId);
                if (data.size() < n) {
                        // TODO: Add logs
                        return;
                }

                ProtocolId id;
                std::memcpy(&id, data.data(), sizeof(ProtocolId));

                const auto protocol_result = try_get_protocol(id);
                if (protocol_result.failed()) {
                        // TODO: Add logs
                        return;
                }

                data.erase(data.begin(), data.begin() + n);
                protocols[id].on_receive(std::move(data));
        }
};
} // namespace transport
