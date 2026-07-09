#pragma once

#include "project_config.h"

enum MqttCommandType : uint8_t {
  MQTT_CMD_NONE = 0,
  MQTT_CMD_MOTOR_ON = 1,
  MQTT_CMD_MOTOR_OFF = 2,
  MQTT_CMD_ALARM_TEST = 3,
  MQTT_CMD_SET_CONFIG = 4,
  MQTT_CMD_GET_CONFIG = 5,
  MQTT_CMD_RESTART = 6,
  MQTT_CMD_FACTORY_RESET = 7,
  MQTT_CMD_OTA_CHECK = 8,
  MQTT_CMD_OTA_UPDATE = 9,
  MQTT_CMD_SYNC = 10,
  MQTT_CMD_REFRESH = 11,
  MQTT_CMD_UNSUPPORTED = 255,
};

struct PendingMqttCommand {
  MqttCommandType type;
  char payload[512];
  char correlation_id[64];
  char requested_version[32];
  bool requires_ack;
  bool is_jenix_runtime;
};

bool mqtt_client_init();
void mqtt_client_update(uint32_t now_ms, const DeviceConfig &config, const SystemStatus &system);
bool mqtt_client_is_connected();
bool mqtt_client_pop_command(PendingMqttCommand &command);
bool mqtt_client_publish_telemetry(const DeviceConfig &config, const SystemStatus &system,
                                   const SensorSnapshot &sensor, const TankRuntime &tank,
                                   const RfRuntime &rf);
bool mqtt_client_publish_status(const DeviceConfig &config, const SystemStatus &system,
                                const TankRuntime &tank);
bool mqtt_client_publish_config(const DeviceConfig &config);
bool mqtt_client_publish_event(const DeviceConfig &config, const char *topic_suffix,
                               const char *code, const char *message);
bool mqtt_client_publish_command_ack(const DeviceConfig &config, const char *delivery_id,
                                     bool success, const char *error_message = nullptr);
bool mqtt_client_publish_ota_ack(const DeviceConfig &config, const char *request_id, bool success,
                                 const char *applied_version = nullptr,
                                 const char *error_message = nullptr);
