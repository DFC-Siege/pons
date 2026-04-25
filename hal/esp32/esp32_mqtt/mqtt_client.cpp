#include "mqtt_client.hpp"

namespace mqtt {

MqttClient::MqttClient(Config cfg) : config(std::move(cfg)) {
        esp_mqtt_client_config_t mqtt_cfg{};
        mqtt_cfg.broker.address.uri = config.uri.c_str();
        if (!config.client_id.empty())
                mqtt_cfg.credentials.client_id = config.client_id.c_str();
        if (!config.username.empty())
                mqtt_cfg.credentials.username = config.username.c_str();
        if (!config.password.empty())
                mqtt_cfg.credentials.authentication.password =
                    config.password.c_str();

        handle = esp_mqtt_client_init(&mqtt_cfg);
        if (!handle)
                return;

        esp_mqtt_client_register_event(
            handle, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
            &MqttClient::event_handler, this);
}

MqttClient::~MqttClient() {
        if (handle) {
                esp_mqtt_client_stop(handle);
                esp_mqtt_client_destroy(handle);
        }
}

result::Try MqttClient::start() {
        if (!handle)
                return result::err("mqtt client not initialized");
        if (esp_mqtt_client_start(handle) != ESP_OK)
                return result::err("failed to start mqtt client");
        return result::ok();
}

result::Try MqttClient::subscribe(std::string_view topic, int qos) {
        if (!handle)
                return result::err("mqtt client not initialized");
        std::string t(topic);
        if (esp_mqtt_client_subscribe(handle, t.c_str(), qos) < 0)
                return result::err("failed to subscribe");
        return result::ok();
}

result::Try MqttClient::send(std::string_view topic, Data &&data) {
        if (!handle)
                return result::err("mqtt client not initialized");
        std::string t(topic);
        int msg_id = esp_mqtt_client_publish(
            handle, t.c_str(), reinterpret_cast<const char *>(data.data()),
            static_cast<int>(data.size()), 0, 0);
        if (msg_id < 0)
                return result::err("failed to publish");
        return result::ok();
}

void MqttClient::on_receive(ReceiveCallback cb) {
        callback = std::move(cb);
}

void MqttClient::event_handler(void *arg, esp_event_base_t /*base*/,
                               int32_t event_id, void *event_data) {
        auto *self = static_cast<MqttClient *>(arg);
        auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);

        if (event_id != MQTT_EVENT_DATA)
                return;

        if (event->current_data_offset == 0) {
                self->pending_topic.assign(event->topic, event->topic_len);
                self->pending_payload.clear();
                self->pending_payload.reserve(event->total_data_len);
        }

        const auto *bytes = reinterpret_cast<const uint8_t *>(event->data);
        self->pending_payload.insert(self->pending_payload.end(), bytes,
                                     bytes + event->data_len);

        const bool last = event->current_data_offset + event->data_len >=
                          event->total_data_len;
        if (last && self->callback) {
                self->callback(self->pending_topic,
                               std::move(self->pending_payload));
                self->pending_payload.clear();
                self->pending_topic.clear();
        }
}

} // namespace mqtt
