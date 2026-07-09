#include <Arduino.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "cloud_client.h"
#include "config_manager.h"
#include "logger.h"
#include "mqtt_client.h"
#include "ota_manager.h"
#include "project_config.h"
#include "rf_output.h"
#include "sensor_dyp.h"
#include "tank_logic.h"
#include "web_server.h"

namespace {

DeviceConfig g_config = {};
RuntimePersist g_persisted_runtime = {};
SystemStatus g_system = {};
bool g_sta_connecting = false;
bool g_sta_had_connection = false;
uint32_t g_sta_connect_started_ms = 0;
uint32_t g_last_sta_retry_ms = 0;
bool g_ap_recovery_active = false;
uint32_t g_ap_recovery_deadline_ms = 0;
bool g_ble_started = false;
NimBLEServer *g_ble_server = nullptr;
NimBLEService *g_ble_device_info_service = nullptr;
uint32_t g_last_telemetry_ms = 0;
uint32_t g_last_runtime_save_ms = 0;
RuntimePersist g_last_saved_runtime = {};
bool g_mqtt_was_connected = false;
char g_pending_ota_request_id[64] = {};
char g_pending_ota_target_version[32] = {};
uint16_t g_last_applied_wifi_tx_power_dbm_tenths = 0;

void build_identity() {
  const uint64_t mac = ESP.getEfuseMac();
  const uint16_t suffix = static_cast<uint16_t>(mac & 0xFFFFU);
  snprintf(g_system.device_name, sizeof(g_system.device_name), "JNX-TG-%04X", suffix);
  snprintf(g_system.mdns_name, sizeof(g_system.mdns_name), "jnx-tg-%04x", suffix);
  snprintf(g_system.mac_address, sizeof(g_system.mac_address), "%02X:%02X:%02X:%02X:%02X:%02X",
           static_cast<unsigned int>((mac >> 40) & 0xFFU),
           static_cast<unsigned int>((mac >> 32) & 0xFFU),
           static_cast<unsigned int>((mac >> 24) & 0xFFU),
           static_cast<unsigned int>((mac >> 16) & 0xFFU),
           static_cast<unsigned int>((mac >> 8) & 0xFFU),
           static_cast<unsigned int>(mac & 0xFFU));
}

bool wifi_is_configured() {
  return g_config.wifi_mode == TG_WIFI_MODE_STA && g_config.wifi_ssid[0] != '\0';
}

void sync_ap_ip() {
  const IPAddress ip = WiFi.softAPIP();
  snprintf(g_system.ap_ip, sizeof(g_system.ap_ip), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

void sync_sta_ip() {
  if (!WiFi.isConnected()) {
    strlcpy(g_system.sta_ip, "", sizeof(g_system.sta_ip));
    return;
  }
  const IPAddress ip = WiFi.localIP();
  snprintf(g_system.sta_ip, sizeof(g_system.sta_ip), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

uint16_t configured_wifi_tx_power_dbm_tenths() {
  return g_config.wifi_tx_power_dbm_tenths == 0
             ? DEFAULT_WIFI_TX_POWER_DBM_TENTHS
             : sanitize_wifi_tx_power_dbm_tenths(g_config.wifi_tx_power_dbm_tenths);
}

uint16_t current_wifi_tx_power_dbm_tenths() {
  const wifi_mode_t mode = WiFi.getMode();
  if ((mode & (WIFI_AP | WIFI_STA)) == 0) {
    return configured_wifi_tx_power_dbm_tenths();
  }
  const WifiTxPowerOption &applied =
      wifi_tx_power_option_for_driver_value(static_cast<int32_t>(WiFi.getTxPower()));
  return applied.dbm_tenths;
}

bool apply_wifi_tx_power(uint16_t requested_dbm_tenths, bool force = false) {
  const WifiTxPowerOption &requested =
      wifi_tx_power_option_for_dbm_tenths(requested_dbm_tenths == 0
                                              ? DEFAULT_WIFI_TX_POWER_DBM_TENTHS
                                              : requested_dbm_tenths);
  g_system.wifi_tx_power_dbm_tenths = requested.dbm_tenths;
  if (!force && g_last_applied_wifi_tx_power_dbm_tenths == requested.dbm_tenths) {
    return true;
  }
  if (!WiFi.setTxPower(static_cast<wifi_power_t>(requested.driver_value))) {
    Serial.printf("[WiFi] Failed to set TX power to %s\n", requested.label);
    return false;
  }
  const WifiTxPowerOption &applied =
      wifi_tx_power_option_for_driver_value(static_cast<int32_t>(WiFi.getTxPower()));
  g_last_applied_wifi_tx_power_dbm_tenths = applied.dbm_tenths;
  g_system.wifi_tx_power_dbm_tenths = applied.dbm_tenths;
  Serial.printf("[WiFi] TX power requested=%s applied=%s\n", requested.label, applied.label);
  return true;
}

void sync_network_snapshot() {
  if (WiFi.isConnected()) {
    const String ssid = WiFi.SSID();
    strlcpy(g_system.active_wifi_ssid, ssid.c_str(), sizeof(g_system.active_wifi_ssid));
    g_system.wifi_rssi_dbm = WiFi.RSSI();
    strlcpy(g_system.local_ip, g_system.sta_ip, sizeof(g_system.local_ip));
  } else if (g_system.ap_active) {
    strlcpy(g_system.active_wifi_ssid, g_system.device_name, sizeof(g_system.active_wifi_ssid));
    g_system.wifi_rssi_dbm = -127;
    strlcpy(g_system.local_ip, g_system.ap_ip, sizeof(g_system.local_ip));
  } else {
    g_system.active_wifi_ssid[0] = '\0';
    g_system.wifi_rssi_dbm = -127;
    g_system.local_ip[0] = '\0';
  }

  snprintf(g_system.local_url, sizeof(g_system.local_url), "http://%s.local/", g_system.mdns_name);
  g_system.wifi_tx_power_dbm_tenths = current_wifi_tx_power_dbm_tenths();
}

void start_access_point() {
  WiFi.softAP(g_system.device_name);
  apply_wifi_tx_power(configured_wifi_tx_power_dbm_tenths(), true);
  g_system.ap_active = true;
  sync_ap_ip();
  Serial.printf("[WiFi] AP '%s' active, IP=%s, TX power=%s\n",
                g_system.device_name,
                g_system.ap_ip,
                wifi_tx_power_option_for_dbm_tenths(configured_wifi_tx_power_dbm_tenths()).label);
}

void begin_sta_connect() {
  WiFi.mode(WIFI_AP_STA);
  if (!g_system.ap_active) {
    start_access_point();
  }
  apply_wifi_tx_power(configured_wifi_tx_power_dbm_tenths(), true);
  WiFi.setHostname(g_system.mdns_name);
  WiFi.setAutoReconnect(true);
  WiFi.begin(g_config.wifi_ssid, g_config.wifi_password);
  g_sta_connecting = true;
  g_sta_connect_started_ms = millis();
  g_last_sta_retry_ms = g_sta_connect_started_ms;
}

void ensure_ble_identity() {
  if (g_ble_started) {
    return;
  }
  NimBLEDevice::init(std::string(g_system.device_name));
  g_ble_server = NimBLEDevice::createServer();
  g_ble_device_info_service = g_ble_server->createService("180A");
  NimBLEService *provisioning_service = g_ble_server->createService(PRODUCT_PROVISION_SERVICE_UUID);

  NimBLECharacteristic *manufacturer =
      g_ble_device_info_service->createCharacteristic("2A29", NIMBLE_PROPERTY::READ);
  manufacturer->setValue("Jenix");

  NimBLECharacteristic *model =
      g_ble_device_info_service->createCharacteristic("2A24", NIMBLE_PROPERTY::READ);
  model->setValue(PRODUCT_SKU);

  NimBLECharacteristic *serial =
      g_ble_device_info_service->createCharacteristic("2A25", NIMBLE_PROPERTY::READ);
  serial->setValue(g_system.mac_address);

  NimBLECharacteristic *firmware =
      g_ble_device_info_service->createCharacteristic("2A26", NIMBLE_PROPERTY::READ);
  firmware->setValue(PRODUCT_FW_VERSION);

  g_ble_device_info_service->start();
  provisioning_service->start();

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->stop();
  advertising->reset();
  advertising->setName(g_system.device_name);
  advertising->addServiceUUID(g_ble_device_info_service->getUUID());
  advertising->addServiceUUID(provisioning_service->getUUID());
  advertising->addTxPower();
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);
  g_ble_started = advertising->start();
  g_system.ble_active = g_ble_started;
  Serial.printf("[BLE] Advertising '%s' %s\n",
                g_system.device_name,
                g_ble_started ? "started" : "failed");
}

void refresh_wifi(uint32_t now_ms) {
  if (!wifi_is_configured()) {
    if (WiFi.getMode() != WIFI_AP) {
      WiFi.mode(WIFI_AP);
      start_access_point();
    } else if (!g_system.ap_active) {
      start_access_point();
    }
    g_system.sta_connected = false;
    g_sta_connecting = false;
    g_ap_recovery_active = false;
    sync_sta_ip();
    return;
  }

  if (!g_sta_connecting && !WiFi.isConnected() &&
      (g_last_sta_retry_ms == 0 || (now_ms - g_last_sta_retry_ms) >= WIFI_CONNECT_RETRY_MS)) {
    begin_sta_connect();
  }

  if (WiFi.isConnected()) {
    g_system.sta_connected = true;
    g_sta_connecting = false;
    sync_sta_ip();
    if (!g_sta_had_connection) {
      logger_log("INFO", "wifi", "WIFI_CONNECTED", "WiFi station connected");
      g_sta_had_connection = true;
    }
    if (!g_ap_recovery_active) {
      g_ap_recovery_active = true;
      g_ap_recovery_deadline_ms = now_ms + WIFI_AP_RECOVERY_WINDOW_MS;
    }
    if (g_ap_recovery_active && now_ms >= g_ap_recovery_deadline_ms) {
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      g_system.ap_active = false;
      g_ap_recovery_active = false;
      g_system.ap_recovery_remaining_s = 0;
    } else if (g_ap_recovery_active) {
      g_system.ap_active = true;
      g_system.ap_recovery_remaining_s = (g_ap_recovery_deadline_ms - now_ms) / 1000UL;
      sync_ap_ip();
    }
    return;
  }

  g_system.sta_connected = false;
  sync_sta_ip();
  if (g_sta_connecting && (now_ms - g_sta_connect_started_ms) >= WIFI_CONNECT_RETRY_MS) {
    g_sta_connecting = false;
  }
  if (g_sta_had_connection) {
    logger_log("WARN", "wifi", "WIFI_DISCONNECTED", "WiFi station disconnected");
    g_sta_had_connection = false;
  }
  if (!g_system.ap_active) {
    WiFi.mode(WIFI_AP_STA);
    start_access_point();
  }
  g_ap_recovery_active = false;
  g_system.ap_recovery_remaining_s = 0;
}

void update_status_led(uint32_t now_ms) {
  if (!g_config.status_led_enabled) {
    digitalWrite(PIN_STATUS_LED, g_config.status_led_active_high ? LOW : HIGH);
    return;
  }

  const TankRuntime &tank = tank_logic_get_runtime();
  bool on = false;
  if (tank.alarm_active || tank.state == TANK_SENSOR_FAULT) {
    on = ((now_ms / 200UL) % 2U) == 0U;
  } else if (tank.state == TANK_FILLING || tank.state == TANK_WAIT_RISE_CONFIRM) {
    on = ((now_ms / 600UL) % 2U) == 0U;
  } else if (g_system.ap_active && !g_system.sta_connected) {
    on = ((now_ms / 1000UL) % 2U) == 0U;
  } else if (g_system.sta_connected) {
    on = true;
  }

  const bool level = g_config.status_led_active_high ? on : !on;
  digitalWrite(PIN_STATUS_LED, level ? HIGH : LOW);
}

void persist_runtime_if_needed(uint32_t now_ms) {
  RuntimePersist snapshot = {};
  tank_logic_build_runtime_persist(snapshot);
  if (memcmp(&snapshot, &g_last_saved_runtime, sizeof(snapshot)) == 0) {
    return;
  }
  if (g_last_runtime_save_ms != 0 && (now_ms - g_last_runtime_save_ms) < 5000UL) {
    return;
  }
  if (config_manager_save_runtime(snapshot)) {
    g_last_saved_runtime = snapshot;
    g_last_runtime_save_ms = now_ms;
  }
}

void ensure_unique_device_id() {
  if (g_config.device_id[0] != '\0' && strcmp(g_config.device_id, DEFAULT_DEVICE_ID) != 0) {
    return;
  }

  const uint64_t mac = ESP.getEfuseMac();
  const uint16_t suffix = static_cast<uint16_t>(mac & 0xFFFFU);
  char generated[sizeof(g_config.device_id)];
  snprintf(generated, sizeof(generated), "JNX-TG-C3-%04X", suffix);
  strlcpy(g_config.device_id, generated, sizeof(g_config.device_id));
  config_manager_save_config(g_config);
}

void apply_config_patch_from_json(JsonVariantConst doc, DeviceConfig &candidate) {
  if (doc.containsKey("zero_level_mm")) candidate.zero_level_mm = doc["zero_level_mm"];
  if (doc.containsKey("bottom_motor_start_level_mm")) {
    candidate.bottom_motor_start_level_mm = doc["bottom_motor_start_level_mm"];
  }
  if (doc.containsKey("top_motor_off_level_mm")) {
    candidate.top_motor_off_level_mm = doc["top_motor_off_level_mm"];
  }
  if (doc.containsKey("overflow_margin_mm")) candidate.overflow_margin_mm = doc["overflow_margin_mm"];
  if (doc.containsKey("power_restore_wait_minutes")) {
    candidate.power_restore_wait_minutes = doc["power_restore_wait_minutes"];
  }
  if (doc.containsKey("motor_start_confirm_minutes")) {
    candidate.motor_start_confirm_minutes = doc["motor_start_confirm_minutes"];
  }
  if (doc.containsKey("motor_off_confirm_minutes")) {
    candidate.motor_off_confirm_minutes = doc["motor_off_confirm_minutes"];
  }
  if (doc.containsKey("water_rise_confirm_mm")) candidate.water_rise_confirm_mm = doc["water_rise_confirm_mm"];
  if (doc.containsKey("rf_on_pulse_ms")) candidate.rf_on_pulse_ms = doc["rf_on_pulse_ms"];
  if (doc.containsKey("rf_off_pulse_ms")) candidate.rf_off_pulse_ms = doc["rf_off_pulse_ms"];
  if (doc.containsKey("rf_alarm_pulse_ms")) candidate.rf_alarm_pulse_ms = doc["rf_alarm_pulse_ms"];
  if (doc.containsKey("rf_on_max_retries")) candidate.rf_on_max_retries = doc["rf_on_max_retries"];
  if (doc.containsKey("rf_off_max_retries")) candidate.rf_off_max_retries = doc["rf_off_max_retries"];
  if (doc.containsKey("rf_retry_gap_minutes")) candidate.rf_retry_gap_minutes = doc["rf_retry_gap_minutes"];
  if (doc.containsKey("alarm_repeat_enable")) candidate.alarm_repeat_enable = doc["alarm_repeat_enable"];
  if (doc.containsKey("alarm_repeat_minutes")) candidate.alarm_repeat_minutes = doc["alarm_repeat_minutes"];
  if (doc.containsKey("telemetry_interval_seconds")) {
    candidate.telemetry_interval_seconds = doc["telemetry_interval_seconds"];
  }
  if (doc.containsKey("wifi_tx_power_dbm_tenths")) {
    candidate.wifi_tx_power_dbm_tenths = doc["wifi_tx_power_dbm_tenths"];
  }
  if (doc.containsKey("wifi_mode")) candidate.wifi_mode = doc["wifi_mode"];
  if (doc.containsKey("mqtt_enabled")) candidate.mqtt_enabled = doc["mqtt_enabled"];
  if (doc.containsKey("mqtt_port")) candidate.mqtt_port = doc["mqtt_port"];

  if (doc.containsKey("device_id")) {
    strlcpy(candidate.device_id, doc["device_id"] | candidate.device_id, sizeof(candidate.device_id));
  }
  if (doc.containsKey("site_name")) {
    strlcpy(candidate.site_name, doc["site_name"] | candidate.site_name, sizeof(candidate.site_name));
  }
  if (doc.containsKey("tank_name")) {
    strlcpy(candidate.tank_name, doc["tank_name"] | candidate.tank_name, sizeof(candidate.tank_name));
  }
  if (doc.containsKey("wifi_ssid")) {
    strlcpy(candidate.wifi_ssid, doc["wifi_ssid"] | candidate.wifi_ssid, sizeof(candidate.wifi_ssid));
  }
  if (doc.containsKey("wifi_password")) {
    const char *wifi_password = doc["wifi_password"] | "";
    if (wifi_password[0] != '\0') {
      strlcpy(candidate.wifi_password, wifi_password, sizeof(candidate.wifi_password));
    }
  }
  if (doc.containsKey("mqtt_host")) {
    strlcpy(candidate.mqtt_host, doc["mqtt_host"] | candidate.mqtt_host, sizeof(candidate.mqtt_host));
  }
  if (doc.containsKey("mqtt_username")) {
    strlcpy(candidate.mqtt_username, doc["mqtt_username"] | candidate.mqtt_username,
            sizeof(candidate.mqtt_username));
  }
  if (doc.containsKey("mqtt_password")) {
    const char *mqtt_password = doc["mqtt_password"] | "";
    if (mqtt_password[0] != '\0') {
      strlcpy(candidate.mqtt_password, mqtt_password, sizeof(candidate.mqtt_password));
    }
  }
  if (doc.containsKey("cloud_base_url")) {
    strlcpy(candidate.cloud_base_url, doc["cloud_base_url"] | candidate.cloud_base_url,
            sizeof(candidate.cloud_base_url));
  }
  if (doc.containsKey("device_ingest_key")) {
    const char *device_ingest_key = doc["device_ingest_key"] | "";
    if (device_ingest_key[0] != '\0') {
      strlcpy(candidate.device_ingest_key, device_ingest_key, sizeof(candidate.device_ingest_key));
    }
  }
  if (doc.containsKey("cloud_home_id")) {
    strlcpy(candidate.cloud_home_id, doc["cloud_home_id"] | candidate.cloud_home_id,
            sizeof(candidate.cloud_home_id));
  }
  if (doc.containsKey("cloud_owner_user_id")) {
    strlcpy(candidate.cloud_owner_user_id,
            doc["cloud_owner_user_id"] | candidate.cloud_owner_user_id,
            sizeof(candidate.cloud_owner_user_id));
  }
  if (doc.containsKey("ota_url")) {
    strlcpy(candidate.ota_url, doc["ota_url"] | candidate.ota_url, sizeof(candidate.ota_url));
  }
  if (doc.containsKey("ota_channel")) {
    strlcpy(candidate.ota_channel, doc["ota_channel"] | candidate.ota_channel, sizeof(candidate.ota_channel));
  }
}

void handle_mqtt_command(const PendingMqttCommand &command) {
  StaticJsonDocument<768> doc;
  deserializeJson(doc, command.payload);
  bool success = true;
  const char *error_message = "";

  if (command.type == MQTT_CMD_MOTOR_ON) {
    const bool override_top =
        command.is_jenix_runtime ? (doc["payload"]["override"] | false) : (doc["override"] | false);
    success = tank_logic_manual_motor_on(g_config, override_top);
    if (!success) error_message = "Motor ON command rejected";
  } else if (command.type == MQTT_CMD_MOTOR_OFF) {
    success = tank_logic_manual_motor_off(g_config);
    if (!success) error_message = "Motor OFF command rejected";
  } else if (command.type == MQTT_CMD_ALARM_TEST) {
    success = tank_logic_manual_alarm_test(g_config);
    if (!success) error_message = "Alarm test command rejected";
  } else if (command.type == MQTT_CMD_GET_CONFIG || command.type == MQTT_CMD_SYNC ||
             command.type == MQTT_CMD_REFRESH) {
    mqtt_client_publish_config(g_config);
    mqtt_client_publish_status(g_config, g_system, tank_logic_get_runtime());
  } else if (command.type == MQTT_CMD_SET_CONFIG) {
    DeviceConfig candidate = g_config;
    apply_config_patch_from_json(doc.as<JsonVariantConst>(), candidate);
    config_manager_validate(candidate);
    if (config_manager_save_config(candidate)) {
      g_config = candidate;
      rf_output_apply_config(g_config);
      mqtt_client_publish_config(g_config);
      mqtt_client_publish_event(g_config, "config", "CONFIG_UPDATED", "Config updated from MQTT");
    } else {
      success = false;
      error_message = "Config save failed";
    }
  } else if (command.type == MQTT_CMD_RESTART) {
    if (command.requires_ack) {
      mqtt_client_publish_command_ack(g_config, command.correlation_id, true);
    }
    mqtt_client_publish_event(g_config, "event", "RESTART", "Restart requested from MQTT");
    delay(200);
    ESP.restart();
  } else if (command.type == MQTT_CMD_FACTORY_RESET) {
    if (command.requires_ack) {
      mqtt_client_publish_command_ack(g_config, command.correlation_id, true);
    }
    mqtt_client_publish_event(g_config, "event", "FACTORY_RESET", "Factory reset requested from MQTT");
    config_manager_factory_reset(g_config, g_persisted_runtime);
    delay(200);
    ESP.restart();
  } else if (command.type == MQTT_CMD_OTA_CHECK || command.type == MQTT_CMD_OTA_UPDATE) {
    OtaRequest request = {};
    if (command.is_jenix_runtime) {
      strlcpy(request.url, doc["artifactUrl"] | g_config.ota_url, sizeof(request.url));
      strlcpy(request.version, doc["targetVersion"] | "", sizeof(request.version));
      strlcpy(request.checksum, doc["checksum"] | "", sizeof(request.checksum));
    } else {
      strlcpy(request.url, doc["url"] | g_config.ota_url, sizeof(request.url));
      strlcpy(request.version, doc["version"] | "", sizeof(request.version));
      strlcpy(request.checksum, doc["checksum"] | "", sizeof(request.checksum));
    }
    char error[128] = {};
    success = ota_manager_request_cloud_update(request, error, sizeof(error));
    if (!success) {
      error_message = error;
      if (command.requires_ack) {
        mqtt_client_publish_ota_ack(g_config, command.correlation_id, false, nullptr, error_message);
      }
    } else if (command.requires_ack) {
      strlcpy(g_pending_ota_request_id, command.correlation_id, sizeof(g_pending_ota_request_id));
      strlcpy(g_pending_ota_target_version,
              command.requested_version[0] != '\0' ? command.requested_version : request.version,
              sizeof(g_pending_ota_target_version));
    }
    return;
  } else if (command.type == MQTT_CMD_UNSUPPORTED) {
    success = false;
    error_message = "Unsupported runtime command";
  } else {
    return;
  }

  if (command.requires_ack) {
    mqtt_client_publish_command_ack(g_config, command.correlation_id, success,
                                    success ? nullptr : error_message);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(250);

  pinMode(PIN_STATUS_LED, OUTPUT);
  build_identity();

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  start_access_point();
  ensure_ble_identity();

  config_manager_init(g_config, g_persisted_runtime);
  config_manager_validate(g_config);
  ensure_unique_device_id();
  apply_wifi_tx_power(configured_wifi_tx_power_dbm_tenths(), true);
  sync_network_snapshot();
  logger_init();
  logger_log("INFO", "boot", "SYSTEM_BOOT", "Firmware boot sequence started");

  sensor_dyp_init();
  rf_output_init(g_config);
  tank_logic_init(g_config, g_persisted_runtime);
  cloud_client_init();
  mqtt_client_init();
  ota_manager_init();
  web_server_init(&g_config, &g_system);

  esp_task_wdt_init(30, true);
  esp_task_wdt_add(nullptr);

  ota_manager_mark_app_valid();
  g_last_saved_runtime = g_persisted_runtime;
}

void loop() {
  const uint32_t now_ms = millis();
  esp_task_wdt_reset();

  refresh_wifi(now_ms);
  apply_wifi_tx_power(configured_wifi_tx_power_dbm_tenths());
  sync_network_snapshot();
  sensor_dyp_update(now_ms);
  rf_output_update(now_ms);
  tank_logic_update(now_ms, g_config, sensor_dyp_get_snapshot());

  g_system.mqtt_connected = mqtt_client_is_connected();
  g_system.cloud_registered = cloud_client_is_registered();
  g_system.uptime_s = now_ms / 1000UL;

  mqtt_client_update(now_ms, g_config, g_system);
  g_system.mqtt_connected = mqtt_client_is_connected();
  cloud_client_update(now_ms, g_config, g_system, sensor_dyp_get_snapshot(), tank_logic_get_runtime(),
                      rf_output_get_runtime());
  g_system.cloud_registered = cloud_client_is_registered();
  if (g_system.mqtt_connected && !g_mqtt_was_connected) {
    mqtt_client_publish_config(g_config);
    mqtt_client_publish_status(g_config, g_system, tank_logic_get_runtime());
  }
  g_mqtt_was_connected = g_system.mqtt_connected;

  PendingMqttCommand pending = {};
  while (mqtt_client_pop_command(pending)) {
    handle_mqtt_command(pending);
  }

  TankEvent tank_event = {};
  while (tank_logic_pop_event(tank_event)) {
    mqtt_client_publish_event(g_config, tank_event.topic_suffix, tank_event.code, tank_event.message);
  }

  if (g_last_telemetry_ms == 0 ||
      (now_ms - g_last_telemetry_ms) >=
          static_cast<uint32_t>(g_config.telemetry_interval_seconds) * 1000UL) {
    mqtt_client_publish_telemetry(g_config, g_system, sensor_dyp_get_snapshot(), tank_logic_get_runtime(),
                                  rf_output_get_runtime());
    mqtt_client_publish_status(g_config, g_system, tank_logic_get_runtime());
    g_last_telemetry_ms = now_ms;
  }

  ota_manager_update(now_ms);
  if (g_pending_ota_request_id[0] != '\0') {
    if (strcmp(ota_manager_phase(), "success") == 0) {
      mqtt_client_publish_ota_ack(g_config, g_pending_ota_request_id, true,
                                  g_pending_ota_target_version[0] != '\0' ? g_pending_ota_target_version
                                                                           : PRODUCT_FW_SEMVER,
                                  nullptr);
      g_pending_ota_request_id[0] = '\0';
      g_pending_ota_target_version[0] = '\0';
    } else if (strcmp(ota_manager_phase(), "error") == 0) {
      mqtt_client_publish_ota_ack(g_config, g_pending_ota_request_id, false, nullptr,
                                  ota_manager_message());
      g_pending_ota_request_id[0] = '\0';
      g_pending_ota_target_version[0] = '\0';
    }
  }
  if (ota_manager_should_reboot()) {
    ota_manager_clear_reboot_flag();
    delay(200);
    ESP.restart();
  }

  persist_runtime_if_needed(now_ms);
  update_status_led(now_ms);
  web_server_handle();
  delay(20);
}
