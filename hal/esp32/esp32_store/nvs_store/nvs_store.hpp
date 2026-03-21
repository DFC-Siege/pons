#pragma once

#include <nvs.h>
#include <string_view>

#include "i_store.hpp"
#include "result.hpp"

namespace store {
namespace kv {
class NvsStore : public IStore {
      public:
        static result::Result<NvsStore> init(std::string_view ns);

        result::Result<bool> store(std::string_view key,
                                   std::string_view value) override;

        result::Result<std::string> get(std::string_view key) override;

      private:
        static constexpr auto MAX_INIT_TRIES = 3;
        static bool has_initialized;
        std::string_view ns;
        nvs_handle_t handle;

        NvsStore(std::string_view ns);
        static void assert_initialized();

        result::Result<bool> try_open();
        static result::Result<bool> try_init(int count = 0);
};
} // namespace kv
} // namespace store
