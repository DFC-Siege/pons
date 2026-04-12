#pragma once

#include "dispatcher.hpp"
#include "result.hpp"
#include "serializer.hpp"

namespace transport {
template <serializer::Serializable S>
using SerializedHandler = std::function<void(result::Result<S>)>;

template <Transporter T> class SerializedDispatcher {
      public:
        SerializedDispatcher(std::unique_ptr<Dispatcher<T>> dispatcher)
            : dispatcher(std::move(dispatcher)) {
        }

        template <serializer::Serializable S>
        result::Try send(TransporterId transporter_id, CommandId command_id,
                         S &&data) {
                return dispatcher->send(transporter_id, command_id,
                                        std::move(data.serialize()));
        }

        Dispatcher<T> &get_dispatcher() {
                return *dispatcher;
        }

        template <serializer::Serializable S>
        void register_handler(CommandId id, SerializedHandler<S> &&handler) {
                dispatcher->register_handler(
                    id, [handler](result::Result<Data> result) {
                            if (result.failed()) {
                                    handler(static_cast<result::Result<S>>(
                                        result::err(result.error())));
                                    return;
                            }
                            auto data = result.value();
                            handler(S::deserialize(data));
                    });
        }

      private:
        std::unique_ptr<Dispatcher<T>> dispatcher;
};
} // namespace transport
