#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "transporter/transporter.hpp"

namespace transport {
using CommandId = uint16_t;
using SessionId = uint16_t;
using Handler = std::function<void(result::Result<Data>)>;

template <Transporter T> class Dispatcher {
      public:
        Dispatcher(T &transporter) : transporter(transporter) {
                transporter.set_receiver([this](result::Result<Data> result) {
                        if (result.failed()) {
                                // TODO: Add log
                                return;
                        }

                        handle_data(std::move(result.value()));
                });
        }

        void register_handler(CommandId id, Handler &&handler) {
                assert(handler && "attempted to register an empty handler");
                std::lock_guard<std::mutex> lock(mutex);
                handlers[id] = std::move(handler);
        }

      private:
        std::unordered_map<CommandId, Handler> handlers;
        T &transporter;
        std::mutex mutex;

        void handle_data(Data &&data) {
                // TODO: Handle data
        }
};
} // namespace transport
