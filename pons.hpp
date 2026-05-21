#pragma once

// pons umbrella header for Arduino-ESP32 / PlatformIO consumers.
// Granular includes still work; this is convenience only.

// Core
#include "result.hpp"
#include "logger.hpp"
#include "i_logger.hpp"
#include "serializer.hpp"
#include "mutex.hpp"
#include "semaphore.hpp"

// Platform abstractions
#include "platform_mutex.hpp"
#include "platform_semaphore.hpp"

// HAL interfaces
#include "serial_hal.hpp"
#include "i_http_client.hpp"
#include "mqtt_hal.hpp"
#include "i_store.hpp"

// ESP32 HAL implementations (BLE excluded by default — requires NimBLE build of Arduino-ESP32)
#include "esp32_serial_hal.hpp"
#include "http_client.hpp"         // esp32_http_client
#include "mqtt_client.hpp"         // esp32_mqtt
#include "esp32_logger.hpp"
#ifdef PONS_ENABLE_BLE
#include "i_ble_hal.hpp"
#include "ble_hal.hpp"             // esp32_ble_hal (NimBLE)
#endif

// Transport
#include "transport_data.hpp"
#include "dispatcher.hpp"
#include "serialized_dispatcher.hpp"
#include "requester.hpp"
#include "serialized_requester.hpp"
#include "base_transporter.hpp"
#include "transporter.hpp"
#include "multiplexer.hpp"
#include "chunked_transporter.hpp"
#include "direct_transporter.hpp"
#include "serial_transporter.hpp"
