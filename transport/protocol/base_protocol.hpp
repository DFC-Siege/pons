#pragma once

#include "protocol.hpp"
#include "result.hpp"

namespace transport {
using ReceiveCallback = std::function<void(result::Result<DataView>)>;

class BaseProtocol {
      public:
        explicit BaseProtocol(ReceiveCallback callback) : callback(callback) {
        }

        virtual result::Result<DataView> send(DataView data) = 0;

      protected:
        ReceiveCallback callback;
};
} // namespace transport
