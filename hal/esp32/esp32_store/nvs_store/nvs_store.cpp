#include <nvs.h>
#include <nvs_flash.h>
#include <string_view>

#include "nvs_store.hpp"
#include "result.hpp"

namespace store {
namespace kv {
bool NvsStore::has_initialized = false;

void NvsStore::assert_initialized() {
        assert(has_initialized && "NvsStore has to be initialized before use");
}

NvsStore::NvsStore(std::string_view ns) : ns(ns) {
}

result::Result<bool> NvsStore::try_init(int count) {
        auto err = nvs_flash_init();
        switch (err) {
        case ESP_OK:
                break;
        case ESP_ERR_NVS_NEW_VERSION_FOUND:
        case ESP_ERR_NVS_NO_FREE_PAGES: {
                if (count > MAX_INIT_TRIES) {
                        return result::err("failed to init nvs after " +
                                           std::to_string(count) + " tries");
                }
                return NvsStore::try_init(count++);
        }
        case ESP_ERR_NOT_FOUND:
                return result::err("no partition with label \"nvs\" found in "
                                   "the partition table");
        case ESP_ERR_NO_MEM:
                return result::err(
                    "esp doesn't have enough memory to init nvs");
        default:
                return result::err("an unknown error has occured while "
                                   "trying to initialize the NVS");
        }

        has_initialized = true;
        return result::ok();
}

result::Result<bool> NvsStore::try_open() {
        NvsStore::assert_initialized();

        auto ret = nvs_open(std::string(ns).c_str(), NVS_READWRITE, &handle);
        switch (ret) {
        case ESP_OK:
                break;
        case ESP_FAIL:
                return result::err("nvs partition corrupted");
        case ESP_ERR_NVS_NOT_INITIALIZED:
                return result::err("nvs not initialized");
        case ESP_ERR_NVS_PART_NOT_FOUND:
                return result::err("nvs partition not found");
        case ESP_ERR_NVS_NOT_FOUND:
                return result::err("namespace not found");
        case ESP_ERR_NVS_INVALID_NAME:
                return result::err("invalid namespace name");
        case ESP_ERR_NO_MEM:
                return result::err("out of memory");
        case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
                return result::err("not enough space");
        case ESP_ERR_NOT_ALLOWED:
                return result::err("partition is read only");
        case ESP_ERR_INVALID_ARG:
                return result::err("invalid argument");
        default:
                return result::err("unknown error");
        }

        return result::ok();
}

result::Result<NvsStore> NvsStore::init(std::string_view ns) {
        auto result = try_init();
        if (result.failed()) {
                return result::err(result.error());
        }

        auto store = NvsStore(ns);
        result = store.try_open();
        if (result.failed()) {
                return result::err(result.error());
        }

        return result::ok(store);
}

result::Result<bool> NvsStore::store(std::string_view key,
                                     std::string_view value) {
        NvsStore::assert_initialized();

        auto err = nvs_set_str(handle, std::string(key).c_str(),
                               std::string(value).c_str());
        switch (err) {
        case ESP_OK:
                break;
        case ESP_ERR_NVS_INVALID_HANDLE:
                return result::err("invalid handle");
        case ESP_ERR_NVS_READ_ONLY:
                return result::err("storage is opened as read only");
        case ESP_ERR_NVS_INVALID_NAME:
                return result::err("key name doesn't satisfy constraints");
        case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
                return result::err("not enough space in the storage");
        case ESP_ERR_NVS_REMOVE_FAILED:
                return result::err(
                    "value was written but storage failed to update flash. "
                    "update will finish after reinitialization");
        case ESP_ERR_NVS_VALUE_TOO_LONG:
                return result::err("value is too long");
        default:
                return result::err(
                    "an unknown error occured while setting string");
        }

        err = nvs_commit(handle);
        switch (err) {
        case ESP_OK:
                break;
        case ESP_ERR_NVS_INVALID_HANDLE:
                return result::err("invalid handle");
        default:
                return result::err(
                    "an unknown error occured while commiting to nvs");
        }

        return result::ok();
}

result::Result<std::string> NvsStore::get(std::string_view key) {
        NvsStore::assert_initialized();

        const auto key_str = std::string(key);

        auto check = [](esp_err_t ret) -> result::Result<bool> {
                switch (ret) {
                case ESP_OK:
                        return result::ok();
                case ESP_ERR_NVS_NOT_FOUND:
                        return result::err("key not found");
                case ESP_ERR_NVS_INVALID_HANDLE:
                        return result::err("invalid handle");
                case ESP_ERR_NVS_INVALID_NAME:
                        return result::err("invalid key name");
                case ESP_ERR_NVS_INVALID_LENGTH:
                        return result::err("invalid length");
                case ESP_FAIL:
                        return result::err("nvs corrupted");
                default:
                        return result::err("unknown error");
                }
        };

        size_t len = 0;
        auto result =
            check(nvs_get_str(handle, key_str.c_str(), nullptr, &len));
        if (result.failed())
                return result::err(result.error());

        std::string str(len, '\0');
        result = check(nvs_get_str(handle, key_str.c_str(), str.data(), &len));
        if (result.failed())
                return result::err(result.error());

        return result::ok(str);
}
} // namespace kv
} // namespace store
