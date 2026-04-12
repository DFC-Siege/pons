#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "dispatcher.hpp"
#include "i_logger.hpp"
#include "logger.hpp"
#include "platform_mutex.hpp"
#include "platform_semaphore.hpp"
#include "result.hpp"
#include "semaphore.hpp"
#include "transporter.hpp"

namespace transport {
using SessionId = uint8_t;

struct RequestWrapper {
        SessionId session_id;
        bool success;
        Data data;

        static Data to_data(RequestWrapper &&request_wrapper) {
                Data buf;
                buf.reserve(sizeof(SessionId) + 1 +
                            request_wrapper.data.size());
                detail::push_le(buf, request_wrapper.session_id);
                buf.push_back(request_wrapper.success ? 1 : 0);
                buf.insert(buf.end(), request_wrapper.data.begin(),
                           request_wrapper.data.end());
                return buf;
        }

        static result::Result<RequestWrapper> from_data(Data &&data) {
                if (data.size() < sizeof(SessionId) + 1) {
                        return result::err(
                            "data too small to unwrap request wrapper");
                }
                const auto session_id = detail::pull_le<SessionId>(data, 0);
                const auto success = data[sizeof(SessionId)] != 0;
                auto payload =
                    Data(data.begin() + sizeof(SessionId) + 1, data.end());
                return result::ok(
                    RequestWrapper{session_id, success, std::move(payload)});
        }
};

template <locking::Mutex M = DefaultMutex,
          locking::Semaphore S = DefaultSemaphore>
class RequestHandle {
      public:
        result::Result<Data> await(std::chrono::milliseconds timeout) {
                if (!sem.acquire(timeout)) {
                        if (on_cleanup) {
                                on_cleanup();
                        }
                        return result::err("request timed out");
                }

                std::lock_guard<M> lock(mutex);
                return *response;
        }

        bool has_response() {
                std::lock_guard<M> lock(mutex);
                return response.has_value();
        }

        result::Result<Data> take_response() {
                std::lock_guard<M> lock(mutex);
                if (!response.has_value()) {
                        return result::err("no response available");
                }
                return *response;
        }

      private:
        template <Transporter T, locking::Mutex M2, locking::Semaphore S2>
        friend class Requester;

        S sem;
        M mutex;
        std::optional<result::Result<Data>> response;
        std::function<void()> on_cleanup;
};

template <Transporter T, locking::Mutex M = DefaultMutex,
          locking::Semaphore S = DefaultSemaphore>
class Requester {
      public:
        Requester(Dispatcher<T, M> &dispatcher) : dispatcher(dispatcher) {
        }

        ~Requester() {
                std::lock_guard<M> lock(pending_mutex);
                for (auto &[_, handle] : pending_requests) {
                        {
                                std::lock_guard<M> hlock(handle->mutex);
                                handle->response =
                                    result::err("requester destroyed");
                        }
                        handle->sem.release();
                }
                pending_requests.clear();
        }

        result::Result<std::shared_ptr<RequestHandle<M, S>>>
        send_request(TransporterId transporter_id, CommandId command_id,
                     CommandId response_command_id, Data &&data) {
                const auto session_id = allocate_session();
                if (session_id.failed()) {
                        return result::err(session_id.error());
                }

                auto handle = std::make_shared<RequestHandle<M, S>>();
                handle->on_cleanup = [this, sid = session_id.value()] {
                        cleanup(sid);
                };
                {
                        std::lock_guard<M> lock(pending_mutex);
                        pending_requests[session_id.value()] = handle;
                }

                ensure_response_handler(response_command_id);

                RequestWrapper wrapper{session_id.value(), true, data};
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

        void register_requestable(
            CommandId request_command_id, CommandId response_command_id,
            TransporterId transporter_id,
            std::function<result::Result<Data>(Data)> handler) {
                dispatcher.register_handler(
                    request_command_id,
                    [this, response_command_id, transporter_id,
                     handler =
                         std::move(handler)](result::Result<Data> result) {
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
                            auto wrapped = std::move(unwrap_result).value();

                            auto response = handler(std::move(wrapped.data));

                            RequestWrapper response_wrapper{
                                wrapped.session_id, !response.failed(),
                                response.failed()
                                    ? Data{}
                                    : std::move(response).value()};
                            auto send_result = dispatcher.send(
                                transporter_id, response_command_id,
                                RequestWrapper::to_data(
                                    std::move(response_wrapper)));
                            if (send_result.failed()) {
                                    logging::logger().println(
                                        logging::LogLevel::Error, TAG,
                                        send_result.error());
                            }
                    });
        }

      private:
        static constexpr auto TAG = "Requester";
        Dispatcher<T, M> &dispatcher;
        SessionId next_session{0};
        std::unordered_map<SessionId, std::shared_ptr<RequestHandle<M, S>>>
            pending_requests;
        M pending_mutex;
        std::unordered_set<CommandId> registered_response_commands;

        void cleanup(SessionId session_id) {
                std::lock_guard<M> lock(pending_mutex);
                pending_requests.erase(session_id);
        }

        result::Result<SessionId> allocate_session() {
                std::lock_guard<M> lock(pending_mutex);
                const auto start = next_session;
                do {
                        const auto id = next_session++;
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

                            std::shared_ptr<RequestHandle<M, S>> handle;
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
                                    if (wrapped_data.success) {
                                            handle->response = result::ok(
                                                std::move(wrapped_data.data));
                                    } else {
                                            handle->response = result::err(
                                                "remote handler returned "
                                                "error");
                                    }
                            }
                            cleanup(wrapped_data.session_id);
                            handle->sem.release();
                    });

                registered_response_commands.insert(response_command_id);
        }
};
} // namespace transport
