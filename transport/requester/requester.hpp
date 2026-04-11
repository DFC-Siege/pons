#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "dispatcher.hpp"
#include "i_logger.hpp"
#include "logger.hpp"
#include "platform_mutex.hpp"
#include "result.hpp"
#include "serializer.hpp"
#include "transporter.hpp"

namespace transport {
using SessionId = uint32_t;

struct RequestWrapper {
        SessionId session_id;
        Data data;

        static Data to_data(RequestWrapper &&request_wrapper) {
                Data buf;
                buf.reserve(sizeof(SessionId) + request_wrapper.data.size());
                detail::push_le(buf, request_wrapper.session_id);
                buf.insert(buf.end(), request_wrapper.data.begin(),
                           request_wrapper.data.end());
                return buf;
        }

        static result::Result<RequestWrapper> from_data(Data &&data) {
                if (data.size() < sizeof(SessionId)) {
                        return result::err(
                            "data too small to unwrap request wrapper");
                }
                const auto session_id = detail::pull_le<SessionId>(data, 0);
                auto payload =
                    Data(data.begin() + sizeof(SessionId), data.end());
                return result::ok(
                    RequestWrapper{session_id, std::move(payload)});
        }
};

template <locking::Mutex M = DefaultMutex> class RequestHandle {
      public:
        template <serializer::Serializable R>
        result::Result<R> await(std::chrono::milliseconds timeout) {
                {
                        std::unique_lock<M> lock(mutex);
                        if (!cv.wait_for(lock, timeout, [&] {
                                    return response.has_value();
                            })) {
                                if (on_cleanup) {
                                        on_cleanup();
                                }
                                return result::err("request timed out");
                        }
                }

                auto result = std::move(*response);
                if (result.failed()) {
                        return result::err(result.error());
                }
                return R::deserialize(std::move(result).value());
        }

        bool has_response() {
                std::lock_guard<M> lock(mutex);
                return response.has_value();
        }

        template <serializer::Serializable R>
        result::Result<R> take_response() {
                std::lock_guard<M> lock(mutex);
                if (!response.has_value()) {
                        return result::err("no response available");
                }
                auto result = std::move(*response);
                if (result.failed()) {
                        return result::err(result.error());
                }
                return R::deserialize(std::move(result).value());
        }

      private:
        template <Transporter T, locking::Mutex M2> friend class Requester;

        std::condition_variable_any cv;
        M mutex;
        std::optional<result::Result<Data>> response;
        std::function<void()> on_cleanup;
};

template <Transporter T, locking::Mutex M = DefaultMutex> class Requester {
      public:
        Requester(Dispatcher<T, M> &dispatcher) : dispatcher(dispatcher) {
        }

        template <serializer::Serializable Q>
        result::Result<std::shared_ptr<RequestHandle<M>>>
        send_request(TransporterId transporter_id, CommandId command_id,
                     CommandId response_command_id, Q &&request) {
                const auto session_id = allocate_session();
                if (session_id.failed()) {
                        return result::err(session_id.error());
                }

                auto handle = std::make_shared<RequestHandle<M>>();
                handle->on_cleanup = [this, sid = session_id.value()] {
                        cleanup(sid);
                };
                {
                        std::lock_guard<M> lock(pending_mutex);
                        pending_requests[session_id.value()] = handle;
                }

                ensure_response_handler(response_command_id);

                RequestWrapper wrapper{session_id.value(), request.serialize()};
                const auto send_result = dispatcher.send(
                    transporter_id, command_id,
                    std::move(RequestWrapper::to_data(std::move(wrapper))));
                if (send_result.failed()) {
                        std::lock_guard<M> lock(pending_mutex);
                        pending_requests.erase(session_id.value());
                        return result::err(send_result.error());
                }

                return result::ok(handle);
        }

      private:
        static constexpr auto TAG = "Requester";
        Dispatcher<T, M> &dispatcher;
        SessionId next_session{0};
        std::unordered_map<SessionId, std::shared_ptr<RequestHandle<M>>>
            pending_requests;
        M pending_mutex;
        std::unordered_set<CommandId> registered_response_commands;

        void cleanup(SessionId session_id) {
                std::lock_guard<M> lock(pending_mutex);
                pending_requests.erase(session_id);
        }

        result::Result<SessionId> allocate_session() {
                const auto start = next_session;
                do {
                        const auto id = next_session++;
                        std::lock_guard<M> lock(pending_mutex);
                        if (!pending_requests.contains(id)) {
                                return result::ok(id);
                        }
                } while (next_session != start);
                return result::err("no available session id");
        }

        void ensure_response_handler(CommandId response_command_id) {
                std::lock_guard<M> lock(pending_mutex);
                if (registered_response_commands.contains(
                        response_command_id)) {
                        return;
                }

                dispatcher.register_handler(
                    response_command_id, [this](result::Result<Data> result) {
                            if (result.failed()) {
                                    logging::logger().println(
                                        logging::LogLevel::Error, TAG,
                                        result.error());
                                    return;
                            }

                            auto unwrap_result = RequestWrapper::from_data(
                                std::move(result).value());
                            if (unwrap_result.failed()) {
                                    logging::logger().println(
                                        logging::LogLevel::Error, TAG,
                                        unwrap_result.error());
                                    return;
                            }
                            auto wrapped_data =
                                std::move(unwrap_result).value();

                            std::shared_ptr<RequestHandle<M>> handle;
                            {
                                    std::lock_guard<M> lock(pending_mutex);
                                    auto it = pending_requests.find(
                                        wrapped_data.session_id);
                                    if (it == pending_requests.end()) {
                                            logging::logger().println(
                                                logging::LogLevel::Error, TAG,
                                                "no pending request for "
                                                "session");
                                            return;
                                    }
                                    handle = it->second;
                            }

                            {
                                    std::lock_guard<M> lock(handle->mutex);
                                    handle->response = result::ok(
                                        std::move(wrapped_data.data));
                            }
                            cleanup(wrapped_data.session_id);
                            handle->cv.notify_one();
                    });

                registered_response_commands.insert(response_command_id);
        }
};
} // namespace transport
