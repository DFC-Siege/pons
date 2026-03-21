#include "http_client.hpp"

namespace http {

result::Result<Response> HttpClient::request(esp_http_client_method_t method,
                                             std::string_view url,
                                             const Headers &headers,
                                             std::span<const uint8_t> body) {
        esp_http_client_config_t config{};
        config.url = url.data();

        auto client = esp_http_client_init(&config);
        if (!client)
                return result::err("failed to init http client");

        esp_http_client_set_method(client, method);

        for (const auto &[key, value] : headers)
                esp_http_client_set_header(client, key.data(), value.data());

        if (!body.empty())
                esp_http_client_set_post_field(
                    client, reinterpret_cast<const char *>(body.data()),
                    body.size());

        if (esp_http_client_open(client, body.size()) != ESP_OK) {
                esp_http_client_cleanup(client);
                return result::err("failed to open http connection");
        }

        auto content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
                esp_http_client_cleanup(client);
                return result::err("failed to fetch headers");
        }

        std::vector<uint8_t> response_body(content_length);
        auto read = esp_http_client_read_response(
            client, reinterpret_cast<char *>(response_body.data()),
            content_length);
        if (read < 0) {
                esp_http_client_cleanup(client);
                return result::err("failed to read response");
        }
        response_body.resize(read);

        Response response{
            .status_code = esp_http_client_get_status_code(client),
            .headers = {},
            .body = std::move(response_body),
        };

        esp_http_client_cleanup(client);
        return result::ok(std::move(response));
}

result::Result<Response> HttpClient::get(std::string_view url,
                                         const Headers &headers) {
        return request(HTTP_METHOD_GET, url, headers, {});
}

result::Result<Response> HttpClient::post(std::string_view url,
                                          const Headers &headers,
                                          std::span<const uint8_t> body) {
        return request(HTTP_METHOD_POST, url, headers, body);
}

result::Result<Response> HttpClient::put(std::string_view url,
                                         const Headers &headers,
                                         std::span<const uint8_t> body) {
        return request(HTTP_METHOD_PUT, url, headers, body);
}

result::Result<Response> HttpClient::del(std::string_view url,
                                         const Headers &headers) {
        return request(HTTP_METHOD_DELETE, url, headers, {});
}

result::Result<Response> HttpClient::patch(std::string_view url,
                                           const Headers &headers,
                                           std::span<const uint8_t> body) {
        return request(HTTP_METHOD_PATCH, url, headers, body);
}

} // namespace http
