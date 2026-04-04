#pragma once

#include "protocol/base_protocol.hpp"
#include "result.hpp"

namespace transport {
class ChunkedProtocol : public BaseProtocol {
      public:
        result::Result<DataView> send(DataView data) override;

      protected:
        result::Result<DataView> handle_data(DataView data) override;
};
} // namespace transport
