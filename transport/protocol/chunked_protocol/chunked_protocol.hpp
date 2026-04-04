#pragma once

#include "protocol/base_protocol.hpp"
#include "result.hpp"

namespace transport {
class ChunkedProtocol : public BaseProtocol {
      public:
        result::Result<Data> send(Data data) override;

      protected:
        result::Result<Data> handle_data(Data data) override;
};
} // namespace transport
