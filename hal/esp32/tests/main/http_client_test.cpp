#include <cstring>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <string>
#include <unity.h>

#include "http_client.hpp"
#include "http_client_test.hpp"
#include "sdkconfig.h"

static constexpr auto WIFI_SSID = CONFIG_EXAMPLE_WIFI_SSID;
static constexpr auto WIFI_PASS = CONFIG_EXAMPLE_WIFI_PASSWORD;
static constexpr auto TEST_URL = "http://httpbin.org/get";
static constexpr auto POST_URL = "http://httpbin.org/post";

static EventGroupHandle_t wifi_event_group;
static constexpr auto WIFI_CONNECTED_BIT = BIT0;

static void wifi_event_handler(void *, esp_event_base_t base, int32_t id,
                               void *) {
        if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
                esp_wifi_connect();
        else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
                xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
}

static void wifi_connect() {
        wifi_event_group = xEventGroupCreate();
        esp_netif_init();
        esp_event_loop_create_default();
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);

        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                   wifi_event_handler, nullptr);
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                   wifi_event_handler, nullptr);

        esp_wifi_set_mode(WIFI_MODE_STA);
        wifi_config_t wifi_config{};
        memcpy(wifi_config.sta.ssid, WIFI_SSID, strlen(WIFI_SSID));
        memcpy(wifi_config.sta.password, WIFI_PASS, strlen(WIFI_PASS));
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_start();
        esp_wifi_connect();

        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true,
                            pdMS_TO_TICKS(10000));
}

static void test_get_returns_200() {
        http::HttpClient client;
        const auto result = client.get(TEST_URL, {});
        TEST_ASSERT_FALSE(result.failed());
        TEST_ASSERT_EQUAL(200, result.value().status_code);
}

static void test_get_returns_error_on_invalid_url() {
        http::HttpClient client;
        const auto result = client.get("http://invalid.invalid", {});
        TEST_ASSERT_TRUE(result.failed());
}

static void test_post_returns_200() {
        http::HttpClient client;
        const std::vector<uint8_t> body = {0x01, 0x02, 0x03};
        const auto result = client.post(POST_URL, {}, body);

        if (result.failed()) {
                ESP_LOGE("TEST", "POST request failed: %s",
                         std::string(result.error()).c_str());
        }

        TEST_ASSERT_FALSE(result.failed());
        TEST_ASSERT_EQUAL(200, result.value().status_code);
}

static void test_get_with_headers() {
        http::HttpClient client;
        const Headers headers = {{"Accept", "application/json"}};
        const auto result = client.get(TEST_URL, headers);
        TEST_ASSERT_FALSE(result.failed());
        TEST_ASSERT_EQUAL(200, result.value().status_code);
}

static void test_response_body_not_empty() {
        http::HttpClient client;
        const auto result = client.get(TEST_URL, {});
        TEST_ASSERT_FALSE(result.failed());
        TEST_ASSERT_FALSE(result.value().body.empty());
}

void run_http_client_tests() {
        wifi_connect();
        RUN_TEST(test_get_returns_200);
        RUN_TEST(test_get_returns_error_on_invalid_url);
        RUN_TEST(test_post_returns_200);
        RUN_TEST(test_get_with_headers);
        RUN_TEST(test_response_body_not_empty);
}
