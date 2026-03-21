#pragma once

#include <esp_http_client.h>
#include <span>
#include <string_view>

#include "i_http_client.hpp"
#include "result.hpp"

namespace http {

class HttpClient : public IHttpClient {
      public:
        result::Result<Response> get(std::string_view url,
                                     const Headers &headers) override;
        result::Result<Response> post(std::string_view url,
                                      const Headers &headers,
                                      std::span<const uint8_t> body) override;
        result::Result<Response> put(std::string_view url,
                                     const Headers &headers,
                                     std::span<const uint8_t> body) override;
        result::Result<Response> del(std::string_view url,
                                     const Headers &headers) override;
        result::Result<Response> patch(std::string_view url,
                                       const Headers &headers,
                                       std::span<const uint8_t> body) override;

      private:
        result::Result<Response> request(esp_http_client_method_t method,
                                         std::string_view url,
                                         const Headers &headers,
                                         std::span<const uint8_t> body);
};

} // namespace http
