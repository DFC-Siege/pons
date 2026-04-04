#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <span>
#include <unordered_map>

#include "protocol.hpp"
#include "result.hpp"

namespace transport {

using Command = uint16_t;
using SessionId = uint16_t;
using Data = std::vector<uint8_t>;

struct CommandWrapper {
        const Command command;
        const SessionId session_id;
        const Data data;
};

using DataView = std::span<const uint8_t>;
using Handler = std::function<void(result::Result<DataView>)>;

template <transport::Protocol P> class Dispatcher {
      public:
        Dispatcher(P &protocol);

        result::Result<bool> register_handler(Command, Handler);

        result::Result<bool> send_command(Command, Data);

      private:
        std::unordered_map<Command, Handler> handlers;
};
} // namespace transport
