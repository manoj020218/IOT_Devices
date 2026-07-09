#include "mqtt_client.h"

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

namespace {

constexpr char JENIX_COMMAND_TOPIC[] = "jenix/runtime/commands";
constexpr char JENIX_COMMAND_ACK_TOPIC[] = "jenix/runtime/commands/ack";
constexpr char JENIX_OTA_TOPIC[] = "jenix/runtime/ota";
constexpr char JENIX_OTA_ACK_TOPIC[] = "jenix/runtime/ota/ack";

WiFiClient g_wifi_client;
PubSubClient g_mqtt(g_wifi_client);
PendingMqttCommand g_pending_command = {MQTT_CMD_NONE, {0}, {0}, {0}, false, false};
uint32_t g_last_reconnect_attempt_ms = 0;
char g_device_id[33] = {};

void build_topic(char *buffer, size_t buffer_len, const DeviceConfig &config, const char *suffix) {
  snprintf(buffer, buffer_len, "jnx/tg/%s/%s", config.device_id, suffix);
}

bool equals_ignore_case(const char *lhs, const char *rhs) {
  if (lhs == nullptr || rhs == nullptr) {
    return false;
  }

  while (*lhs != '\0' && *rhs != '\0') {
    char left = *lhs;
    char right = *rhs;
    if (left >= 'a' && left <= 'z') {
      left = static_cast<char>(left - ('a' - 'A'));
    }
    if (right >= 'a' && right <= 'z') {
      right = static_cast<char>(right - ('a' - 'A'));
    }
    if (left != right) {
      return false;
    }
    ++lhs;
    ++rhs;
  }

  return *lhs == '\0' && *rhs == '\0';
}

bool matches_current_device(const char *device_id) {
  return device_id != nullptr && g_device_id[0] != '\0' && equals_ignore_case(device_id, g_device_id);
}

MqttCommandType parse_legacy_command_type(const char *command) {
  if (strcmp(command, "motor_on") == 0) return MQTT_CMD_MOTOR_ON;
  if (strcmp(command, "motor_off") == 0) return MQTT_CMD_MOTOR_OFF;
  if (strcmp(command, "alarm_test") == 0) return MQTT_CMD_ALARM_TEST;
  if (strcmp(command, "set_config") == 0) return MQTT_CMD_SET_CONFIG;
  if (strcmp(command, "get_config") == 0) return MQTT_CMD_GET_CONFIG;
  if (strcmp(command, "restart") == 0) return MQTT_CMD_RESTART;
  if (strcmp(command, "factory_reset") == 0) return MQTT_CMD_FACTORY_RESET;
  if (strcmp(command, "ota_check") == 0) return MQTT_CMD_OTA_CHECK;
  if (strcmp(command, "ota_update") == 0) return MQTT_CMD_OTA_UPDATE;
  return MQTT_CMD_NONE;
}

void store_pending_command(MqttCommandType type, const uint8_t *payload, unsigned int length,
                           const char *correlation_id, const char *requested_version,
                           bool requires_ack, bool is_jenix_runtime) {
  g_pending_command.type = type;
  const unsigned int json_len =
      min(length, static_cast<unsigned int>(sizeof(g_pending_command.payload) - 1U));
  memcpy(g_pending_command.payload, payload, json_len);
  g_pending_command.payload[json_len] = '\0';
  strlcpy(g_pending_command.correlation_id,
          correlation_id == nullptr ? "" : correlation_id,
          sizeof(g_pending_command.correlation_id));
  strlcpy(g_pending_command.requested_version,
          requested_version == nullptr ? "" : requested_version,
          sizeof(g_pending_command.requested_version));
  g_pending_command.requires_ack = requires_ack;
  g_pending_command.is_jenix_runtime = is_jenix_runtime;
}

MqttCommandType map_jenix_command(JsonDocument &doc) {
  const char *command = doc["command"] | "";
  if (strcmp(command, "set_relay") == 0) {
    const bool value = doc["payload"]["value"] | false;
    return value ? MQTT_CMD_MOTOR_ON : MQTT_CMD_MOTOR_OFF;
  }
  if (strcmp(command, "refresh") == 0) return MQTT_CMD_REFRESH;
  if (strcmp(command, "sync") == 0) return MQTT_CMD_SYNC;
  if (strcmp(command, "factory_reset") == 0) return MQTT_CMD_FACTORY_RESET;
  return MQTT_CMD_UNSUPPORTED;
}

void mqtt_callback(char *topic, uint8_t *payload, unsigned int length) {
  StaticJsonDocument<768> doc;
  const DeserializationError error = deserializeJson(doc, payload, length);

  if (strcmp(topic, JENIX_COMMAND_TOPIC) == 0) {
    if (error) {
      return;
    }

    const char *device_id = doc["deviceId"] | "";
    if (!matches_current_device(device_id)) {
      return;
    }

    store_pending_command(map_jenix_command(doc), payload, length, doc["deliveryId"] | "", "",
                          true, true);
    return;
  }

  if (strcmp(topic, JENIX_OTA_TOPIC) == 0) {
    if (error) {
      return;
    }

    const char *device_id = doc["deviceId"] | "";
    if (!matches_current_device(device_id)) {
      return;
    }

    store_pending_command(MQTT_CMD_OTA_UPDATE, payload, length, doc["requestId"] | "",
                          doc["targetVersion"] | "", true, true);
    return;
  }

  const char *command = nullptr;
  if (!error) {
    command = doc["command"] | doc["cmd"] | nullptr;
  }
  if (command == nullptr) {
    static char command_buf[32];
    const unsigned int copy_len = min(length, static_cast<unsigned int>(sizeof(command_buf) - 1U));
    memcpy(command_buf, payload, copy_len);
    command_buf[copy_len] = '\0';
    command = command_buf;
  }

  store_pending_command(parse_legacy_command_type(command), payload, length, "", "", false, false);
}

bool publish_document(const char *topic, JsonDocument &doc, bool retained) {
  char buffer[1024];
  const size_t len = serializeJson(doc, buffer, sizeof(buffer));
  return g_mqtt.publish(topic, reinterpret_cast<const uint8_t *>(buffer), len, retained);
}

uint8_t tank_level_pct(const DeviceConfig &config, const TankRuntime &tank) {
  if (config.top_motor_off_level_mm == 0) {
    return 0;
  }

  uint32_t pct = (static_cast<uint32_t>(tank.water_level_mm) * 100UL) /
                 static_cast<uint32_t>(config.top_motor_off_level_mm);
  if (pct > 100UL) {
    pct = 100UL;
  }
  return static_cast<uint8_t>(pct);
}

void build_ack_timestamp(char *buffer, size_t buffer_len) {
  const uint32_t total_seconds = millis() / 1000UL;
  const uint32_t hours = (total_seconds / 3600UL) % 24UL;
  const uint32_t minutes = (total_seconds / 60UL) % 60UL;
  const uint32_t seconds = total_seconds % 60UL;
  snprintf(buffer, buffer_len, "1970-01-01T%02lu:%02lu:%02lu.000Z",
           static_cast<unsigned long>(hours), static_cast<unsigned long>(minutes),
           static_cast<unsigned long>(seconds));
}

}  // namespace

bool mqtt_client_init() {
  g_mqtt.setCallback(mqtt_callback);
  g_mqtt.setBufferSize(1024);
  return true;
}

void mqtt_client_update(uint32_t now_ms, const DeviceConfig &config, const SystemStatus &system) {
  strlcpy(g_device_id, config.device_id, sizeof(g_device_id));

  if (!config.mqtt_enabled || config.mqtt_host[0] == '\0' || !system.sta_connected) {
    if (g_mqtt.connected()) {
      g_mqtt.disconnect();
    }
    return;
  }

  if (!g_mqtt.connected() && (now_ms - g_last_reconnect_attempt_ms) >= MQTT_RECONNECT_MS) {
    g_last_reconnect_attempt_ms = now_ms;
    g_mqtt.setServer(config.mqtt_host, config.mqtt_port);

    char client_id[48];
    snprintf(client_id, sizeof(client_id), "jnx-tg-%s", config.device_id);

    bool connected = false;
    if (config.mqtt_username[0] != '\0') {
      connected = g_mqtt.connect(client_id, config.mqtt_username, config.mqtt_password);
    } else {
      connected = g_mqtt.connect(client_id);
    }

    if (connected) {
      char cmd_topic[96];
      build_topic(cmd_topic, sizeof(cmd_topic), config, "cmd");
      g_mqtt.subscribe(cmd_topic);
      g_mqtt.subscribe(JENIX_COMMAND_TOPIC);
      g_mqtt.subscribe(JENIX_OTA_TOPIC);
    }
  }

  if (g_mqtt.connected()) {
    g_mqtt.loop();
  }
}

bool mqtt_client_is_connected() {
  return g_mqtt.connected();
}

bool mqtt_client_pop_command(PendingMqttCommand &command) {
  if (g_pending_command.type == MQTT_CMD_NONE) {
    return false;
  }
  command = g_pending_command;
  g_pending_command.type = MQTT_CMD_NONE;
  g_pending_command.payload[0] = '\0';
  g_pending_command.correlation_id[0] = '\0';
  g_pending_command.requested_version[0] = '\0';
  g_pending_command.requires_ack = false;
  g_pending_command.is_jenix_runtime = false;
  return true;
}

bool mqtt_client_publish_telemetry(const DeviceConfig &config, const SystemStatus &system,
                                   const SensorSnapshot &sensor, const TankRuntime &tank,
                                   const RfRuntime &rf) {
  if (!g_mqtt.connected()) {
    return false;
  }

  StaticJsonDocument<1024> doc;
  doc["project"] = PRODUCT_NAME;
  doc["pid"] = PRODUCT_PID;
  doc["sku"] = PRODUCT_SKU;
  doc["device_id"] = config.device_id;
  doc["mac"] = system.mac_address;
  doc["ssid_name"] = system.device_name;
  doc["firmware_version"] = PRODUCT_FW_VERSION;
  doc["hardware_revision"] = PRODUCT_HW_REVISION;
  doc["build_date"] = PRODUCT_BUILD_DATE;
  doc["uptime_sec"] = system.uptime_s;
  doc["wifi_status"] = system.sta_connected ? "connected" : (system.ap_active ? "ap_mode" : "disconnected");
  doc["mqtt_status"] = g_mqtt.connected() ? "connected" : "disconnected";
  doc["water_level_mm"] = tank.water_level_mm;
  doc["tankLevelPct"] = tank_level_pct(config, tank);
  doc["raw_distance_mm"] = sensor.raw_distance_mm;
  doc["filtered_distance_mm"] = sensor.filtered_distance_mm;
  doc["zero_level_mm"] = config.zero_level_mm;
  doc["bottom_level_mm"] = config.bottom_motor_start_level_mm;
  doc["top_level_mm"] = config.top_motor_off_level_mm;
  doc["overflow_margin_mm"] = config.overflow_margin_mm;
  doc["water_trend"] = water_trend_to_string(tank.water_trend);
  doc["rise_rate_mm_per_min"] = tank.rise_rate_mm_per_min;
  doc["current_state"] = tank_state_to_string(tank.state);
  doc["motor_status"] = motor_status_to_string(tank.motor_status);
  doc["last_command"] = last_command_to_string(tank.last_command);
  doc["last_command_time"] = tank.last_command_ms;
  doc["rf_on_count"] = rf.rf_on_count;
  doc["rf_off_count"] = rf.rf_off_count;
  doc["rf_alarm_count"] = rf.rf_alarm_count;
  doc["sensor_status"] = sensor_status_to_string(sensor.status);
  doc["alarm_active"] = tank.alarm_active;
  doc["alarm_code"] = alarm_code_to_string(tank.alarm_code);
  doc["alarm_message"] = tank.last_alarm_message;

  char topic[96];
  build_topic(topic, sizeof(topic), config, "telemetry");
  return publish_document(topic, doc, false);
}

bool mqtt_client_publish_status(const DeviceConfig &config, const SystemStatus &system,
                                const TankRuntime &tank) {
  if (!g_mqtt.connected()) {
    return false;
  }

  StaticJsonDocument<384> doc;
  doc["project"] = PRODUCT_NAME;
  doc["pid"] = PRODUCT_PID;
  doc["device_id"] = config.device_id;
  doc["state"] = tank_state_to_string(tank.state);
  doc["motor_status"] = motor_status_to_string(tank.motor_status);
  doc["alarm_active"] = tank.alarm_active;
  doc["alarm_code"] = alarm_code_to_string(tank.alarm_code);
  doc["wifi_status"] = system.sta_connected ? "connected" : (system.ap_active ? "ap_mode" : "disconnected");
  doc["mqtt_status"] = g_mqtt.connected() ? "connected" : "disconnected";
  doc["cloud_registered"] = system.cloud_registered;

  char topic[96];
  build_topic(topic, sizeof(topic), config, "status");
  return publish_document(topic, doc, true);
}

bool mqtt_client_publish_config(const DeviceConfig &config) {
  if (!g_mqtt.connected()) {
    return false;
  }

  StaticJsonDocument<1024> doc;
  doc["device_id"] = config.device_id;
  doc["site_name"] = config.site_name;
  doc["tank_name"] = config.tank_name;
  doc["wifi_mode"] = config.wifi_mode;
  doc["wifi_ssid"] = config.wifi_ssid;
  doc["mqtt_enabled"] = config.mqtt_enabled;
  doc["mqtt_host"] = config.mqtt_host;
  doc["mqtt_port"] = config.mqtt_port;
  doc["mqtt_username"] = config.mqtt_username;
  doc["cloud_base_url"] = config.cloud_base_url;
  doc["cloud_home_id"] = config.cloud_home_id;
  doc["cloud_owner_user_id"] = config.cloud_owner_user_id;
  doc["zero_level_mm"] = config.zero_level_mm;
  doc["bottom_motor_start_level_mm"] = config.bottom_motor_start_level_mm;
  doc["top_motor_off_level_mm"] = config.top_motor_off_level_mm;
  doc["overflow_margin_mm"] = config.overflow_margin_mm;
  doc["power_restore_wait_minutes"] = config.power_restore_wait_minutes;
  doc["motor_start_confirm_minutes"] = config.motor_start_confirm_minutes;
  doc["motor_off_confirm_minutes"] = config.motor_off_confirm_minutes;
  doc["water_rise_confirm_mm"] = config.water_rise_confirm_mm;
  doc["rf_on_pulse_ms"] = config.rf_on_pulse_ms;
  doc["rf_off_pulse_ms"] = config.rf_off_pulse_ms;
  doc["rf_alarm_pulse_ms"] = config.rf_alarm_pulse_ms;
  doc["rf_on_max_retries"] = config.rf_on_max_retries;
  doc["rf_off_max_retries"] = config.rf_off_max_retries;
  doc["rf_retry_gap_minutes"] = config.rf_retry_gap_minutes;
  doc["alarm_repeat_enable"] = config.alarm_repeat_enable;
  doc["alarm_repeat_minutes"] = config.alarm_repeat_minutes;
  doc["telemetry_interval_seconds"] = config.telemetry_interval_seconds;
  doc["rf_on_active_high"] = config.rf_on_active_high;
  doc["rf_off_active_high"] = config.rf_off_active_high;
  doc["rf_alarm_active_high"] = config.rf_alarm_active_high;
  doc["ota_url"] = config.ota_url;
  doc["ota_channel"] = config.ota_channel;

  char topic[96];
  build_topic(topic, sizeof(topic), config, "config");
  return publish_document(topic, doc, false);
}

bool mqtt_client_publish_event(const DeviceConfig &config, const char *topic_suffix,
                               const char *code, const char *message) {
  if (!g_mqtt.connected()) {
    return false;
  }

  StaticJsonDocument<384> doc;
  doc["project"] = PRODUCT_NAME;
  doc["device_id"] = config.device_id;
  doc["code"] = code;
  doc["message"] = message;
  doc["uptime_ms"] = millis();

  char topic[96];
  build_topic(topic, sizeof(topic), config, topic_suffix);
  return publish_document(topic, doc, false);
}

bool mqtt_client_publish_command_ack(const DeviceConfig &config, const char *delivery_id,
                                     bool success, const char *error_message) {
  if (!g_mqtt.connected() || delivery_id == nullptr || delivery_id[0] == '\0') {
    return false;
  }

  char acknowledged_at[32];
  build_ack_timestamp(acknowledged_at, sizeof(acknowledged_at));

  StaticJsonDocument<384> doc;
  doc["deliveryId"] = delivery_id;
  doc["deviceId"] = config.device_id;
  doc["acknowledgedAt"] = acknowledged_at;
  doc["status"] = success ? "completed" : "failed";
  if (!success && error_message != nullptr && error_message[0] != '\0') {
    doc["errorMessage"] = error_message;
  }

  return publish_document(JENIX_COMMAND_ACK_TOPIC, doc, false);
}

bool mqtt_client_publish_ota_ack(const DeviceConfig &config, const char *request_id, bool success,
                                 const char *applied_version, const char *error_message) {
  if (!g_mqtt.connected() || request_id == nullptr || request_id[0] == '\0') {
    return false;
  }

  char acknowledged_at[32];
  build_ack_timestamp(acknowledged_at, sizeof(acknowledged_at));

  StaticJsonDocument<384> doc;
  doc["requestId"] = request_id;
  doc["deviceId"] = config.device_id;
  doc["acknowledgedAt"] = acknowledged_at;
  doc["status"] = success ? "completed" : "failed";
  if (success && applied_version != nullptr && applied_version[0] != '\0') {
    doc["appliedVersion"] = applied_version;
  }
  if (!success && error_message != nullptr && error_message[0] != '\0') {
    doc["errorMessage"] = error_message;
  }

  return publish_document(JENIX_OTA_ACK_TOPIC, doc, false);
}
