#pragma once

#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "result.hpp"

using Headers = std::unordered_map<std::string_view, std::string_view>;

struct Response {
        int status_code;
        Headers headers;
        std::vector<uint8_t> body;
};

namespace http {
struct IHttpClient {
        virtual ~IHttpClient() = default;

        virtual result::Result<Response> get(std::string_view url,
                                             const Headers &headers) = 0;

        virtual result::Result<Response>
        post(std::string_view url, const Headers &headers,
             std::span<const uint8_t> body) = 0;

        virtual result::Result<Response> put(std::string_view url,
                                             const Headers &headers,
                                             std::span<const uint8_t> body) = 0;

        virtual result::Result<Response> del(std::string_view url,
                                             const Headers &headers) = 0;

        virtual result::Result<Response>
        patch(std::string_view url, const Headers &headers,
              std::span<const uint8_t> body) = 0;
};
} // namespace http
