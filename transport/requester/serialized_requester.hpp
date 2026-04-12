#pragma once

#include <memory>

#include "platform_mutex.hpp"
#include "requester.hpp"
#include "serializer.hpp"

namespace transport {

template <locking::Mutex M = DefaultMutex,
          locking::Semaphore S = DefaultSemaphore>
class SerializedRequestHandle {
      public:
        SerializedRequestHandle(
            std::shared_ptr<RequestHandle<M, S>> request_handle)
            : request_handle(request_handle) {
        }

        template <serializer::Serializable T>
        result::Result<T> await(std::chrono::milliseconds timeout) {
                auto result = request_handle->await(timeout);
                if (result.failed()) {
                        return result::err(result.error());
                }
                return T::deserialize(result.value());
        }

        bool has_response() {
                return request_handle->has_response();
        }

        template <serializer::Serializable T>
        result::Result<T> take_response() {
                auto response = request_handle->take_response();
                if (response.failed()) {
                        return result::err(response.error());
                }
                return T::deserialize(response.value());
        }

      private:
        std::shared_ptr<RequestHandle<M, S>> request_handle;
};

template <Transporter T, locking::Mutex M = DefaultMutex,
          locking::Semaphore S = DefaultSemaphore>
class SerializedRequester {
      public:
        SerializedRequester(std::unique_ptr<Requester<T, M, S>> requester)
            : requester(std::move(requester)) {
        }

        template <serializer::Serializable Q>
        result::Result<SerializedRequestHandle<M, S>>
        send_request(TransporterId transporter_id, CommandId command_id,
                     CommandId response_command_id, Q &&data) {
                auto handle_result = requester->send_request(
                    transporter_id, command_id, response_command_id,
                    std::move(data.serialize()));
                if (handle_result.failed()) {
                        return result::err(handle_result.error());
                }
                return result::ok(SerializedRequestHandle<M, S>(
                    std::move(handle_result).value()));
        }

        template <serializer::Serializable R, serializer::Serializable Q>
        void register_requestable(CommandId request_command_id,
                                  CommandId response_command_id,
                                  TransporterId transporter_id,
                                  std::function<result::Result<R>(Q)> handler) {
                requester->register_requestable(
                    request_command_id, response_command_id, transporter_id,
                    [handler](Data data) -> result::Result<Data> {
                            auto deserialize_result = Q::deserialize(data);
                            if (deserialize_result.failed()) {
                                    return result::err(
                                        deserialize_result.error());
                            }

                            auto handler_result =
                                handler(std::move(deserialize_result).value());
                            if (handler_result.failed()) {
                                    return result::err(handler_result.error());
                            }
                            return result::ok(
                                handler_result.value().serialize());
                    });
        }

      private:
        std::unique_ptr<Requester<T, M, S>> requester;
};
} // namespace transport
