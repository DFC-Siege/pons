#pragma once

#include <cstdint>
#include <memory>
#include <queue>
#include <vector>

#include "result.hpp"
#include "transport_data.hpp"
#include "transporter.hpp"

namespace transport {
class BaseTransporter {
      public:
        virtual result::Try send(Data &&data) = 0;
        virtual void set_receiver(ReceiveCallback callback);
        virtual MTU get_mtu() const = 0;
        virtual ~BaseTransporter() = default;
        BaseTransporter() = default;
        BaseTransporter(const BaseTransporter &) = delete;
        BaseTransporter &operator=(const BaseTransporter &) = delete;
        BaseTransporter(BaseTransporter &&) = delete;
        BaseTransporter &operator=(BaseTransporter &&) = delete;

      protected:
        result::Try try_callback(result::Result<Data> data);

      private:
        std::optional<ReceiveCallback> callback;
};

static_assert(Transporter<BaseTransporter>);
} // namespace transport
