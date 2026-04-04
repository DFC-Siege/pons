#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "logger.hpp"
#include "result.hpp"
#include "transport_data.hpp"
#include "transporter.hpp"

namespace transport {
using CommandId = uint16_t;
using Handler = std::function<void(result::Result<Data>)>;

struct WrappedData {
        CommandId command_id;
        Data data;

        static result::Result<WrappedData> unwrap_data(Data &&data) {
                return result::err("not implemented");
        }

        static result::Result<Data> wrap_data(CommandId command_id,
                                              Data &&data) {
                return result::err("not implemented");
        }
};

template <Transporter T> class Dispatcher {
      public:
        Dispatcher(T &transporter) : transporter(transporter) {
                transporter.set_receiver([this](result::Result<Data> result) {
                        if (result.failed()) {
                                logging::logger().println(
                                    logging::LogLevel::Error, TAG,
                                    result.error());
                                return;
                        }

                        handle_data(std::move(result.value()));
                });
        }

        result::Result<bool> send(CommandId command_id, Data &&data) {
                const auto wrap_result =
                    WrappedData::wrap_data(command_id, std::move(data));
                if (wrap_result.failed()) {
                        return result::err(wrap_result.error());
                }

                return transporter.send(std::move(wrap_result.value()));
        }

        void register_handler(CommandId id, Handler &&handler) {
                assert(handler && "attempted to register an empty handler");
                std::lock_guard<std::mutex> lock(mutex);
                handlers[id] = std::move(handler);
        }

      private:
        static constexpr auto TAG = "Dispatcher";
        std::unordered_map<CommandId, Handler> handlers;
        T &transporter;
        std::mutex mutex;

        result::Result<Handler> try_get_handler(CommandId id) {
                auto it = handlers.find(id);
                if (it == handlers.end()) {
                        return result::err("couldn't find handler");
                }

                return result::ok(it->second);
        }

        void handle_data(Data &&data) {
                const auto unwrap_result =
                    WrappedData::unwrap_data(std::move(data));
                if (unwrap_result.failed()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  unwrap_result.error());
                        return;
                }

                auto wrapped_data = unwrap_result.value();
                Handler target_handler;
                {
                        std::lock_guard<std::mutex> lock(mutex);
                        auto it = handlers.find(wrapped_data.command_id);
                        if (it == handlers.end()) {
                                logging::logger().println(
                                    logging::LogLevel::Error, TAG,
                                    "no handler found");
                                return;
                        }
                        target_handler = it->second;
                }

                target_handler(result::ok(std::move(wrapped_data.data)));
        }
};
} // namespace transport
