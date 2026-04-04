#pragma once

#include <cstdint>
#include <memory>
#include <queue>
#include <vector>

#include "result.hpp"
#include "transporter.hpp"

namespace transport {
class BaseTransporter {
      public:
        virtual result::Result<bool> send(DataView data) = 0;
        void set_receiver(ReceiveCallback callback);
        virtual MTU get_mtu() const = 0;

      protected:
        result::Result<bool> handle_receive(DataView data);

      private:
        std::optional<ReceiveCallback> callback;
};
} // namespace transport
