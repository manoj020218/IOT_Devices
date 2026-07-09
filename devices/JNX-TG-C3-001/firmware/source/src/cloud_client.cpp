#include "cloud_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "logger.h"

namespace {

struct CloudState {
  bool registered;
  uint32_t last_register_attempt_ms;
  uint32_t last_telemetry_attempt_ms;
  char phase[24];
  char message[128];
  char bound_device_id[33];
  char bound_base_url[160];
  char bound_home_id[65];
  char bound_owner_user_id[65];
  char bound_ingest_key[65];
} g_cloud = {};

void set_phase(const char *phase, const char *message) {
  strlcpy(g_cloud.phase, phase, sizeof(g_cloud.phase));
  strlcpy(g_cloud.message, message, sizeof(g_cloud.message));
}

bool has_cloud_config(const DeviceConfig &config) {
  return config.cloud_base_url[0] != '\0' && config.device_ingest_key[0] != '\0' &&
         config.cloud_home_id[0] != '\0' && config.cloud_owner_user_id[0] != '\0';
}

void sync_binding(const DeviceConfig &config) {
  if (strcmp(g_cloud.bound_device_id, config.device_id) == 0 &&
      strcmp(g_cloud.bound_base_url, config.cloud_base_url) == 0 &&
      strcmp(g_cloud.bound_home_id, config.cloud_home_id) == 0 &&
      strcmp(g_cloud.bound_owner_user_id, config.cloud_owner_user_id) == 0 &&
      strcmp(g_cloud.bound_ingest_key, config.device_ingest_key) == 0) {
    return;
  }

  strlcpy(g_cloud.bound_device_id, config.device_id, sizeof(g_cloud.bound_device_id));
  strlcpy(g_cloud.bound_base_url, config.cloud_base_url, sizeof(g_cloud.bound_base_url));
  strlcpy(g_cloud.bound_home_id, config.cloud_home_id, sizeof(g_cloud.bound_home_id));
  strlcpy(g_cloud.bound_owner_user_id, config.cloud_owner_user_id, sizeof(g_cloud.bound_owner_user_id));
  strlcpy(g_cloud.bound_ingest_key, config.device_ingest_key, sizeof(g_cloud.bound_ingest_key));
  g_cloud.registered = false;
  g_cloud.last_register_attempt_ms = 0;
  g_cloud.last_telemetry_attempt_ms = 0;
}

String normalize_base_url(const char *base_url) {
  String base = String(base_url);
  base.trim();
  while (base.endsWith("/")) {
    base.remove(base.length() - 1);
  }
  return base;
}

String build_api_url(const char *base_url, const char *path) {
  String base = normalize_base_url(base_url);
  if (base.endsWith("/api/v1")) {
    return base + path;
  }
  return base + "/api/v1" + path;
}

int post_json(const String &url, const char *device_key, const String &body, String *response_body) {
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(8000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-device-key", device_key);

  int status = -1;
  if (url.startsWith("https://")) {
    WiFiClientSecure client;
    client.setInsecure();
    if (!http.begin(client, url)) {
      return -1;
    }
    status = http.POST(body);
  } else {
    WiFiClient client;
    if (!http.begin(client, url)) {
      return -1;
    }
    status = http.POST(body);
  }

  if (response_body != nullptr) {
    *response_body = http.getString();
  }
  http.end();
  return status;
}

const char *connectivity_status(bool enabled, bool connected) {
  if (!enabled) {
    return "unknown";
  }
  return connected ? "online" : "offline";
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

bool register_device(const DeviceConfig &config) {
  StaticJsonDocument<512> doc;
  doc["deviceId"] = config.device_id;
  doc["pid"] = PRODUCT_PID;
  doc["homeId"] = config.cloud_home_id;
  doc["ownerUserId"] = config.cloud_owner_user_id;
  doc["displayName"] = config.tank_name;
  doc["firmwareVersion"] = PRODUCT_FW_SEMVER;
  doc["hardwareRevision"] = PRODUCT_HW_REVISION;
  doc["matterEnabled"] = false;

  String body;
  serializeJson(doc, body);

  const String url = build_api_url(config.cloud_base_url, "/devices/register");
  String response;
  const int status = post_json(url, config.device_ingest_key, body, &response);
  if (status == 200 || status == 201 || status == 409) {
    g_cloud.registered = true;
    set_phase("registered", "Cloud device registration ready");
    return true;
  }

  g_cloud.registered = false;
  if (status == 404) {
    set_phase("error", "Cloud register failed: PID or route not found");
  } else if (status == 401 || status == 403) {
    set_phase("error", "Cloud register failed: device key rejected");
  } else {
    set_phase("error", "Cloud register failed");
  }
  logger_log("WARN", "cloud", "REGISTER_FAIL", g_cloud.message);
  return false;
}

bool publish_telemetry(const DeviceConfig &config, const SystemStatus &system,
                       const SensorSnapshot &sensor, const TankRuntime &tank, const RfRuntime &rf) {
  StaticJsonDocument<1536> telemetry;
  telemetry["tankLevelPct"] = tank_level_pct(config, tank);
  telemetry["tankLevelMm"] = tank.water_level_mm;
  telemetry["rawDistanceMm"] = sensor.raw_distance_mm;
  telemetry["filteredDistanceMm"] = sensor.filtered_distance_mm;
  telemetry["zeroLevelMm"] = config.zero_level_mm;
  telemetry["bottomLevelMm"] = config.bottom_motor_start_level_mm;
  telemetry["topLevelMm"] = config.top_motor_off_level_mm;
  telemetry["overflowMarginMm"] = config.overflow_margin_mm;
  telemetry["signalStrength"] = system.wifi_rssi_dbm;
  telemetry["wifiRssi"] = system.wifi_rssi_dbm;
  telemetry["wifiSsidName"] = system.active_wifi_ssid;
  telemetry["localIp"] = system.local_ip;
  telemetry["localUrl"] = system.local_url;
  telemetry["wifiTxPowerDbmTenths"] = system.wifi_tx_power_dbm_tenths;
  telemetry["wifiTxPowerDbm"] = wifi_tx_power_dbm_value(system.wifi_tx_power_dbm_tenths);
  telemetry["wifiTxPowerLabel"] =
      wifi_tx_power_option_for_dbm_tenths(system.wifi_tx_power_dbm_tenths).label;
  telemetry["pumpRunning"] = tank.motor_status == MOTOR_STATUS_ASSUMED_ON;
  telemetry["alarmState"] = tank.alarm_active
                                ? (tank.alarm_code == ALARM_NONE ? "active" : alarm_code_to_string(tank.alarm_code))
                                : "normal";
  telemetry["waterTrend"] = water_trend_to_string(tank.water_trend);
  telemetry["currentState"] = tank_state_to_string(tank.state);
  telemetry["motorStatus"] = motor_status_to_string(tank.motor_status);
  telemetry["sensorStatus"] = sensor_status_to_string(sensor.status);
  telemetry["rfOnCount"] = rf.rf_on_count;
  telemetry["rfOffCount"] = rf.rf_off_count;
  telemetry["rfAlarmCount"] = rf.rf_alarm_count;
  telemetry["firmwareVersion"] = PRODUCT_FW_SEMVER;
  telemetry["hardwareRevision"] = PRODUCT_HW_REVISION;

  StaticJsonDocument<2048> doc;
  doc["telemetry"] = telemetry;
  doc["mqttStatus"] = connectivity_status(config.mqtt_enabled != 0, system.mqtt_connected);
  doc["cloudStatus"] = "online";
  doc["localStatus"] = "available";

  String body;
  serializeJson(doc, body);

  String path = "/devices/";
  path += config.device_id;
  path += "/telemetry";
  const String url = build_api_url(config.cloud_base_url, path.c_str());

  String response;
  const int status = post_json(url, config.device_ingest_key, body, &response);
  if (status == 200 || status == 201) {
    set_phase("online", "Cloud telemetry synced");
    return true;
  }

  if (status == 404) {
    g_cloud.registered = false;
    set_phase("error", "Cloud telemetry rejected: device not registered");
  } else if (status == 401 || status == 403) {
    set_phase("error", "Cloud telemetry rejected: device key rejected");
  } else {
    set_phase("error", "Cloud telemetry post failed");
  }
  logger_log("WARN", "cloud", "TELEMETRY_FAIL", g_cloud.message);
  return false;
}

}  // namespace

bool cloud_client_init() {
  memset(&g_cloud, 0, sizeof(g_cloud));
  set_phase("disabled", "Cloud integration not configured");
  return true;
}

void cloud_client_update(uint32_t now_ms, const DeviceConfig &config, const SystemStatus &system,
                         const SensorSnapshot &sensor, const TankRuntime &tank, const RfRuntime &rf) {
  sync_binding(config);

  if (!has_cloud_config(config)) {
    set_phase("disabled", "Cloud integration not configured");
    return;
  }

  if (!system.sta_connected) {
    set_phase("waiting_wifi", "Waiting for station WiFi before cloud sync");
    return;
  }

  if (!g_cloud.registered &&
      (g_cloud.last_register_attempt_ms == 0 ||
       (now_ms - g_cloud.last_register_attempt_ms) >= CLOUD_REGISTER_RETRY_MS)) {
    g_cloud.last_register_attempt_ms = now_ms;
    set_phase("registering", "Registering device with Jenix cloud");
    if (!register_device(config)) {
      return;
    }
  }

  const uint32_t telemetry_interval_ms =
      static_cast<uint32_t>(config.telemetry_interval_seconds == 0
                                ? TELEMETRY_FALLBACK_INTERVAL_SEC
                                : config.telemetry_interval_seconds) *
      1000UL;
  if (g_cloud.last_telemetry_attempt_ms != 0 &&
      (now_ms - g_cloud.last_telemetry_attempt_ms) < telemetry_interval_ms) {
    return;
  }

  g_cloud.last_telemetry_attempt_ms = now_ms;
  publish_telemetry(config, system, sensor, tank, rf);
}

bool cloud_client_is_registered() {
  return g_cloud.registered;
}

const char *cloud_client_phase() {
  return g_cloud.phase;
}

const char *cloud_client_message() {
  return g_cloud.message;
}
