#pragma once

#include <mqtt_client.h>
#include <string>

#include "mqtt_hal.hpp"
#include "result.hpp"

namespace mqtt {

class MqttClient {
      public:
        struct Config {
                std::string uri;
                std::string client_id;
                std::string username;
                std::string password;
        };

        explicit MqttClient(Config config);
        ~MqttClient();

        MqttClient(const MqttClient &) = delete;
        MqttClient &operator=(const MqttClient &) = delete;
        MqttClient(MqttClient &&) = delete;
        MqttClient &operator=(MqttClient &&) = delete;

        result::Try start();
        result::Try subscribe(std::string_view topic, int qos = 0);

        result::Try send(std::string_view topic, Data &&data);
        void on_receive(ReceiveCallback callback);

      private:
        static void event_handler(void *arg, esp_event_base_t base,
                                  int32_t event_id, void *event_data);

        Config config;
        esp_mqtt_client_handle_t handle = nullptr;
        ReceiveCallback callback;

        std::string pending_topic;
        Data pending_payload;
};

} // namespace mqtt
