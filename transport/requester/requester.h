#pragma once

#include <chrono>

#include "dispatcher.hpp"
#include "result.hpp"
#include "serializer.hpp"
#include "transporter.hpp"

namespace transport {
template <Transporter T, locking::Mutex M = DefaultMutex> class Requester {
      public:
        Requester(Dispatcher<T> &dispatcher) : dispatcher(dispatcher) {
        }

        template <serializer::Serializable R, serializer::Serializable Q>
        result::Result<R> request(transport::TransporterId transporter_id,
                                  transport::CommandId command_id, Q &&request,
                                  std::chrono::milliseconds timeout) {
                // TODO: Implement
                const auto send_result = dispatcher.send(
                    transporter_id, command_id, std::move(request.serialize()));
                if (send_result.failed()) {
                        return result::err(send_result.error());
                }
                // dispatcher.register_handler(CommandId id, Handler &&handler)

                return result::err("not implemented");
        }

      private:
        Dispatcher<T> &dispatcher;
};
} // namespace transport
