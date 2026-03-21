#include "http_client.hpp"

namespace http {

struct ResponseCollector {
        std::vector<uint8_t> *body;
};

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
        if (evt->event_id == HTTP_EVENT_ON_DATA) {
                auto *collector =
                    static_cast<ResponseCollector *>(evt->user_data);
                const auto *data = static_cast<const uint8_t *>(evt->data);
                collector->body->insert(collector->body->end(), data,
                                        data + evt->data_len);
        }
        return ESP_OK;
}

result::Result<Response> HttpClient::request(esp_http_client_method_t method,
                                             std::string_view url,
                                             const Headers &headers,
                                             std::span<const uint8_t> body) {
        std::vector<uint8_t> response_body;
        ResponseCollector collector{&response_body};

        esp_http_client_config_t config{};
        config.url = url.data();
        config.timeout_ms = 5000;
        config.event_handler = http_event_handler;
        config.user_data = &collector;

        auto client = esp_http_client_init(&config);
        if (!client)
                return result::err("failed to init http client");

        esp_http_client_set_method(client, method);

        for (const auto &[key, value] : headers)
                esp_http_client_set_header(client, key.data(), value.data());

        if (!body.empty()) {
                esp_http_client_set_post_field(
                    client, reinterpret_cast<const char *>(body.data()),
                    static_cast<int>(body.size()));
        }

        esp_err_t err = esp_http_client_perform(client);
        if (err != ESP_OK) {
                esp_http_client_cleanup(client);
                return result::err("http request failed");
        }

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
