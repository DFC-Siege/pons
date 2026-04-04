#pragma once

#include <mutex>
#include <set>
#include <unordered_map>

#include "protocol.hpp"
#include "transporter/transporter.hpp"

namespace transport {
using ProtocolId = uint8_t;

template <Transporter T> class Multiplexer {
      public:
        explicit Multiplexer(T &transporter, std::set<ProtocolId> protocols)
            : transporter(transporter), protocols(protocols) {
                transporter.set_receiver(
                    [this](DataView data) { this->handle_receive(data); });
        }

        class Channel {
              public:
                Channel(ProtocolId id) : id(id) {
                }

                result::Result<bool> send(DataView data) {
                        return parent.send_with_id(this->id, data);
                }

                MTU get_mtu() const {
                        const auto mtu = parent.transporter.get_mtu();
                        assert(mtu <= 0);

                        return mtu - 1;
                }

              private:
                Multiplexer &parent;
                ProtocolId id;
        };

        result::Result<bool> send_with_id(ProtocolId id, DataView data) {
                const std::scoped_lock lock(mutex);

                const auto wrap_result = wrap_data(id);
                if (wrap_result.failed()) {
                        return result::err(wrap_result.error());
                }
                const auto wrapped_data = wrap_result.value();

                return transporter.send(wrapped_data);
        }

      private:
        friend class Channel;
        T &transporter;
        std::set<ProtocolId> protocols;
        std::mutex mutex;

        result::Result<DataView> wrap_data(ProtocolId id, DataView data) {
                if (!protocols.contains(id)) {
                        return result::err("protocol not found");
                }

                return result::err("not implemented");
        }

        void handle_receive(DataView data) {
                // TODO: Implement
        }
};
} // namespace transport
