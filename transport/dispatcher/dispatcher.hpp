#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "logger.hpp"
#include "mutex.hpp"
#include "platform_mutex.hpp"
#include "result.hpp"
#include "transport_data.hpp"
#include "transporter.hpp"

namespace transport {
using TransporterId = uint8_t;
using CommandId = uint16_t;
using Handler = std::function<void(result::Result<Data>)>;

struct WrappedData {
        CommandId command_id;
        Data data;

        static result::Result<WrappedData> unwrap_data(Data &&data) {
                if (data.size() < sizeof(CommandId)) {
                        return result::err("data too small to unwrap");
                }

                const auto command_id = detail::pull_le<CommandId>(data, 0);
                auto payload =
                    Data(data.begin() + sizeof(CommandId), data.end());

                return result::ok(WrappedData{command_id, std::move(payload)});
        }

        static result::Result<Data> wrap_data(CommandId command_id,
                                              Data &&data) {
                Data buf;
                buf.reserve(sizeof(CommandId) + data.size());
                detail::push_le(buf, command_id);
                buf.insert(buf.end(), data.begin(), data.end());
                return result::ok(std::move(buf));
        }
};

template <Transporter T, locking::Mutex M = DefaultMutex> class Dispatcher {
      public:
        void register_transporter(TransporterId id, T &&transporter) {
                std::lock_guard<M> lock(mutex);
                auto [it, inserted] = transporters.emplace(
                    id, std::move(transporter));
                assert(inserted && "duplicate transporter id");
                it->second.set_receiver(
                    [this](result::Result<Data> result) {
                            if (result.failed()) {
                                    logging::logger().println(
                                        logging::LogLevel::Error, TAG,
                                        result.error());
                                    return;
                            }

                            handle_data(std::move(result).value());
                    });
        }

        result::Try send(TransporterId transporter_id, CommandId command_id,
                         Data &&data) {
                T *transporter = nullptr;
                {
                        std::lock_guard<M> lock(mutex);
                        auto it = transporters.find(transporter_id);
                        if (it == transporters.end()) {
                                return result::err(
                                    "no transporter found with id");
                        }
                        transporter = &it->second;
                }

                auto wrap_result =
                    WrappedData::wrap_data(command_id, std::move(data));
                if (wrap_result.failed()) {
                        return result::err(wrap_result.error());
                }
                return transporter->send(std::move(wrap_result).value());
        }

        void register_handler(CommandId id, Handler &&handler) {
                assert(handler && "attempted to register an empty handler");
                std::lock_guard<M> lock(mutex);
                handlers[id] = std::move(handler);
        }

      private:
        static constexpr auto TAG = "Dispatcher";
        std::unordered_map<CommandId, Handler> handlers;
        std::unordered_map<TransporterId, T> transporters;
        M mutex;

        void handle_data(Data &&data) {
                auto unwrap_result = WrappedData::unwrap_data(std::move(data));
                if (unwrap_result.failed()) {
                        logging::logger().println(logging::LogLevel::Error, TAG,
                                                  unwrap_result.error());
                        return;
                }
                auto wrapped_data = std::move(unwrap_result).value();
                Handler target_handler;
                {
                        std::lock_guard<M> lock(mutex);
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
}; // namespace transport
} // namespace transport
