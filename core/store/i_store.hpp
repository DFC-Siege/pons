#pragma once

#include <string>
#include <string_view>

#include "result.hpp"

namespace store {
namespace kv {
class IStore {
      public:
        virtual ~IStore() = default;
        virtual result::Result<bool> store(std::string_view key,
                                           std::string_view value) = 0;
        virtual result::Result<std::string> get(std::string_view key) = 0;
};
} // namespace kv
} // namespace store
