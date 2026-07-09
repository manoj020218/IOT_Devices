#include "config_manager.h"

#include <Preferences.h>
#include <cstring>

namespace {

Preferences g_preferences;
bool g_is_open = false;

struct StoredConfigBlob {
  uint32_t magic;
  uint16_t version;
  uint16_t payload_size;
  DeviceConfig payload;
  uint32_t crc32;
};

struct StoredRuntimeBlob {
  uint32_t magic;
  uint16_t version;
  uint16_t payload_size;
  RuntimePersist payload;
  uint32_t crc32;
};

struct LegacyDeviceConfigV1 {
  uint16_t zero_level_mm;
  uint16_t bottom_motor_start_level_mm;
  uint16_t top_motor_off_level_mm;
  uint16_t overflow_margin_mm;
  uint16_t power_restore_wait_minutes;
  uint16_t motor_start_confirm_minutes;
  uint16_t motor_off_confirm_minutes;
  uint16_t water_rise_confirm_mm;
  uint16_t rf_on_pulse_ms;
  uint16_t rf_off_pulse_ms;
  uint16_t rf_alarm_pulse_ms;
  uint8_t rf_on_max_retries;
  uint8_t rf_off_max_retries;
  uint8_t rf_retry_gap_minutes;
  uint8_t alarm_repeat_enable;
  uint16_t alarm_repeat_minutes;
  uint16_t telemetry_interval_seconds;
  uint8_t wifi_mode;
  char wifi_ssid[33];
  char wifi_password[65];
  uint8_t mqtt_enabled;
  char mqtt_host[65];
  uint16_t mqtt_port;
  char mqtt_username[33];
  char mqtt_password[65];
  char device_id[33];
  char site_name[49];
  char tank_name[49];
  uint8_t rf_on_active_high;
  uint8_t rf_off_active_high;
  uint8_t rf_alarm_active_high;
  uint8_t status_led_enabled;
  uint8_t status_led_active_high;
  char ota_url[192];
  char ota_channel[24];
};

struct LegacyDeviceConfigV2 {
  uint16_t zero_level_mm;
  uint16_t bottom_motor_start_level_mm;
  uint16_t top_motor_off_level_mm;
  uint16_t overflow_margin_mm;
  uint16_t power_restore_wait_minutes;
  uint16_t motor_start_confirm_minutes;
  uint16_t motor_off_confirm_minutes;
  uint16_t water_rise_confirm_mm;
  uint16_t rf_on_pulse_ms;
  uint16_t rf_off_pulse_ms;
  uint16_t rf_alarm_pulse_ms;
  uint8_t rf_on_max_retries;
  uint8_t rf_off_max_retries;
  uint8_t rf_retry_gap_minutes;
  uint8_t alarm_repeat_enable;
  uint16_t alarm_repeat_minutes;
  uint16_t telemetry_interval_seconds;
  uint8_t wifi_mode;
  char wifi_ssid[33];
  char wifi_password[65];
  uint8_t mqtt_enabled;
  char mqtt_host[65];
  uint16_t mqtt_port;
  char mqtt_username[33];
  char mqtt_password[65];
  char device_id[33];
  char site_name[49];
  char tank_name[49];
  uint8_t rf_on_active_high;
  uint8_t rf_off_active_high;
  uint8_t rf_alarm_active_high;
  uint8_t status_led_enabled;
  uint8_t status_led_active_high;
  char ui_password[32];
  char ota_url[192];
  char ota_channel[24];
};

struct LegacyDeviceConfigV3 {
  uint16_t zero_level_mm;
  uint16_t bottom_motor_start_level_mm;
  uint16_t top_motor_off_level_mm;
  uint16_t overflow_margin_mm;
  uint16_t power_restore_wait_minutes;
  uint16_t motor_start_confirm_minutes;
  uint16_t motor_off_confirm_minutes;
  uint16_t water_rise_confirm_mm;
  uint16_t rf_on_pulse_ms;
  uint16_t rf_off_pulse_ms;
  uint16_t rf_alarm_pulse_ms;
  uint8_t rf_on_max_retries;
  uint8_t rf_off_max_retries;
  uint8_t rf_retry_gap_minutes;
  uint8_t alarm_repeat_enable;
  uint16_t alarm_repeat_minutes;
  uint16_t telemetry_interval_seconds;
  uint8_t wifi_mode;
  char wifi_ssid[33];
  char wifi_password[65];
  uint8_t mqtt_enabled;
  char mqtt_host[65];
  uint16_t mqtt_port;
  char mqtt_username[33];
  char mqtt_password[65];
  char device_id[33];
  char site_name[49];
  char tank_name[49];
  char cloud_base_url[160];
  char device_ingest_key[65];
  char cloud_home_id[65];
  char cloud_owner_user_id[65];
  uint8_t rf_on_active_high;
  uint8_t rf_off_active_high;
  uint8_t rf_alarm_active_high;
  uint8_t status_led_enabled;
  uint8_t status_led_active_high;
  char ui_password[32];
  char ota_url[192];
  char ota_channel[24];
};

struct LegacyStoredConfigBlobV1 {
  uint32_t magic;
  uint16_t version;
  uint16_t payload_size;
  LegacyDeviceConfigV1 payload;
  uint32_t crc32;
};

struct LegacyStoredConfigBlobV2 {
  uint32_t magic;
  uint16_t version;
  uint16_t payload_size;
  LegacyDeviceConfigV2 payload;
  uint32_t crc32;
};

struct LegacyStoredConfigBlobV3 {
  uint32_t magic;
  uint16_t version;
  uint16_t payload_size;
  LegacyDeviceConfigV3 payload;
  uint32_t crc32;
};

uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len) {
  crc = ~crc;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 1U) ? (crc >> 1U) ^ 0xEDB88320UL : (crc >> 1U);
    }
  }
  return ~crc;
}

template <typename T>
bool blob_matches(const T &lhs, const T &rhs) {
  return memcmp(&lhs, &rhs, sizeof(T)) == 0;
}

template <typename TBlob, typename TPayload>
bool save_blob(const char *key, uint32_t magic, uint16_t version, const TPayload &payload) {
  if (!g_is_open) {
    return false;
  }

  TBlob blob = {};
  blob.magic = magic;
  blob.version = version;
  blob.payload_size = sizeof(TPayload);
  blob.payload = payload;
  blob.crc32 = crc32_update(0, reinterpret_cast<const uint8_t *>(&blob.payload), sizeof(TPayload));

  return g_preferences.putBytes(key, &blob, sizeof(blob)) == sizeof(blob);
}

template <typename TBlob, typename TPayload>
bool load_blob(const char *key, uint32_t magic, uint16_t version, TPayload &payload) {
  if (!g_is_open || !g_preferences.isKey(key)) {
    return false;
  }

  TBlob blob = {};
  const size_t read = g_preferences.getBytes(key, &blob, sizeof(blob));
  if (read != sizeof(blob)) {
    return false;
  }
  if (blob.magic != magic || blob.version != version || blob.payload_size != sizeof(TPayload)) {
    return false;
  }

  const uint32_t expected_crc =
      crc32_update(0, reinterpret_cast<const uint8_t *>(&blob.payload), sizeof(TPayload));
  if (expected_crc != blob.crc32) {
    return false;
  }

  payload = blob.payload;
  return true;
}

template <typename T>
T clamp_value(T value, T min_value, T max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

void copy_default_string(char *dest, size_t size, const char *value) {
  strlcpy(dest, value, size);
}

void copy_legacy_into_current(const LegacyDeviceConfigV1 &legacy, DeviceConfig &current) {
  current.zero_level_mm = legacy.zero_level_mm;
  current.bottom_motor_start_level_mm = legacy.bottom_motor_start_level_mm;
  current.top_motor_off_level_mm = legacy.top_motor_off_level_mm;
  current.overflow_margin_mm = legacy.overflow_margin_mm;
  current.power_restore_wait_minutes = legacy.power_restore_wait_minutes;
  current.motor_start_confirm_minutes = legacy.motor_start_confirm_minutes;
  current.motor_off_confirm_minutes = legacy.motor_off_confirm_minutes;
  current.water_rise_confirm_mm = legacy.water_rise_confirm_mm;
  current.rf_on_pulse_ms = legacy.rf_on_pulse_ms;
  current.rf_off_pulse_ms = legacy.rf_off_pulse_ms;
  current.rf_alarm_pulse_ms = legacy.rf_alarm_pulse_ms;
  current.rf_on_max_retries = legacy.rf_on_max_retries;
  current.rf_off_max_retries = legacy.rf_off_max_retries;
  current.rf_retry_gap_minutes = legacy.rf_retry_gap_minutes;
  current.alarm_repeat_enable = legacy.alarm_repeat_enable;
  current.alarm_repeat_minutes = legacy.alarm_repeat_minutes;
  current.telemetry_interval_seconds = legacy.telemetry_interval_seconds;
  current.wifi_mode = legacy.wifi_mode;
  strlcpy(current.wifi_ssid, legacy.wifi_ssid, sizeof(current.wifi_ssid));
  strlcpy(current.wifi_password, legacy.wifi_password, sizeof(current.wifi_password));
  current.mqtt_enabled = legacy.mqtt_enabled;
  strlcpy(current.mqtt_host, legacy.mqtt_host, sizeof(current.mqtt_host));
  current.mqtt_port = legacy.mqtt_port;
  strlcpy(current.mqtt_username, legacy.mqtt_username, sizeof(current.mqtt_username));
  strlcpy(current.mqtt_password, legacy.mqtt_password, sizeof(current.mqtt_password));
  strlcpy(current.device_id, legacy.device_id, sizeof(current.device_id));
  strlcpy(current.site_name, legacy.site_name, sizeof(current.site_name));
  strlcpy(current.tank_name, legacy.tank_name, sizeof(current.tank_name));
  current.rf_on_active_high = legacy.rf_on_active_high;
  current.rf_off_active_high = legacy.rf_off_active_high;
  current.rf_alarm_active_high = legacy.rf_alarm_active_high;
  current.status_led_enabled = legacy.status_led_enabled;
  current.status_led_active_high = legacy.status_led_active_high;
  strlcpy(current.ota_url, legacy.ota_url, sizeof(current.ota_url));
  strlcpy(current.ota_channel, legacy.ota_channel, sizeof(current.ota_channel));
}

void copy_legacy_v2_into_current(const LegacyDeviceConfigV2 &legacy, DeviceConfig &current) {
  current.zero_level_mm = legacy.zero_level_mm;
  current.bottom_motor_start_level_mm = legacy.bottom_motor_start_level_mm;
  current.top_motor_off_level_mm = legacy.top_motor_off_level_mm;
  current.overflow_margin_mm = legacy.overflow_margin_mm;
  current.power_restore_wait_minutes = legacy.power_restore_wait_minutes;
  current.motor_start_confirm_minutes = legacy.motor_start_confirm_minutes;
  current.motor_off_confirm_minutes = legacy.motor_off_confirm_minutes;
  current.water_rise_confirm_mm = legacy.water_rise_confirm_mm;
  current.rf_on_pulse_ms = legacy.rf_on_pulse_ms;
  current.rf_off_pulse_ms = legacy.rf_off_pulse_ms;
  current.rf_alarm_pulse_ms = legacy.rf_alarm_pulse_ms;
  current.rf_on_max_retries = legacy.rf_on_max_retries;
  current.rf_off_max_retries = legacy.rf_off_max_retries;
  current.rf_retry_gap_minutes = legacy.rf_retry_gap_minutes;
  current.alarm_repeat_enable = legacy.alarm_repeat_enable;
  current.alarm_repeat_minutes = legacy.alarm_repeat_minutes;
  current.telemetry_interval_seconds = legacy.telemetry_interval_seconds;
  current.wifi_mode = legacy.wifi_mode;
  strlcpy(current.wifi_ssid, legacy.wifi_ssid, sizeof(current.wifi_ssid));
  strlcpy(current.wifi_password, legacy.wifi_password, sizeof(current.wifi_password));
  current.mqtt_enabled = legacy.mqtt_enabled;
  strlcpy(current.mqtt_host, legacy.mqtt_host, sizeof(current.mqtt_host));
  current.mqtt_port = legacy.mqtt_port;
  strlcpy(current.mqtt_username, legacy.mqtt_username, sizeof(current.mqtt_username));
  strlcpy(current.mqtt_password, legacy.mqtt_password, sizeof(current.mqtt_password));
  strlcpy(current.device_id, legacy.device_id, sizeof(current.device_id));
  strlcpy(current.site_name, legacy.site_name, sizeof(current.site_name));
  strlcpy(current.tank_name, legacy.tank_name, sizeof(current.tank_name));
  current.rf_on_active_high = legacy.rf_on_active_high;
  current.rf_off_active_high = legacy.rf_off_active_high;
  current.rf_alarm_active_high = legacy.rf_alarm_active_high;
  current.status_led_enabled = legacy.status_led_enabled;
  current.status_led_active_high = legacy.status_led_active_high;
  strlcpy(current.ui_password, legacy.ui_password, sizeof(current.ui_password));
  strlcpy(current.ota_url, legacy.ota_url, sizeof(current.ota_url));
  strlcpy(current.ota_channel, legacy.ota_channel, sizeof(current.ota_channel));
}

void copy_legacy_v3_into_current(const LegacyDeviceConfigV3 &legacy, DeviceConfig &current) {
  current.zero_level_mm = legacy.zero_level_mm;
  current.bottom_motor_start_level_mm = legacy.bottom_motor_start_level_mm;
  current.top_motor_off_level_mm = legacy.top_motor_off_level_mm;
  current.overflow_margin_mm = legacy.overflow_margin_mm;
  current.power_restore_wait_minutes = legacy.power_restore_wait_minutes;
  current.motor_start_confirm_minutes = legacy.motor_start_confirm_minutes;
  current.motor_off_confirm_minutes = legacy.motor_off_confirm_minutes;
  current.water_rise_confirm_mm = legacy.water_rise_confirm_mm;
  current.rf_on_pulse_ms = legacy.rf_on_pulse_ms;
  current.rf_off_pulse_ms = legacy.rf_off_pulse_ms;
  current.rf_alarm_pulse_ms = legacy.rf_alarm_pulse_ms;
  current.rf_on_max_retries = legacy.rf_on_max_retries;
  current.rf_off_max_retries = legacy.rf_off_max_retries;
  current.rf_retry_gap_minutes = legacy.rf_retry_gap_minutes;
  current.alarm_repeat_enable = legacy.alarm_repeat_enable;
  current.alarm_repeat_minutes = legacy.alarm_repeat_minutes;
  current.telemetry_interval_seconds = legacy.telemetry_interval_seconds;
  current.wifi_mode = legacy.wifi_mode;
  strlcpy(current.wifi_ssid, legacy.wifi_ssid, sizeof(current.wifi_ssid));
  strlcpy(current.wifi_password, legacy.wifi_password, sizeof(current.wifi_password));
  current.mqtt_enabled = legacy.mqtt_enabled;
  strlcpy(current.mqtt_host, legacy.mqtt_host, sizeof(current.mqtt_host));
  current.mqtt_port = legacy.mqtt_port;
  strlcpy(current.mqtt_username, legacy.mqtt_username, sizeof(current.mqtt_username));
  strlcpy(current.mqtt_password, legacy.mqtt_password, sizeof(current.mqtt_password));
  strlcpy(current.device_id, legacy.device_id, sizeof(current.device_id));
  strlcpy(current.site_name, legacy.site_name, sizeof(current.site_name));
  strlcpy(current.tank_name, legacy.tank_name, sizeof(current.tank_name));
  strlcpy(current.cloud_base_url, legacy.cloud_base_url, sizeof(current.cloud_base_url));
  strlcpy(current.device_ingest_key, legacy.device_ingest_key, sizeof(current.device_ingest_key));
  strlcpy(current.cloud_home_id, legacy.cloud_home_id, sizeof(current.cloud_home_id));
  strlcpy(current.cloud_owner_user_id, legacy.cloud_owner_user_id,
          sizeof(current.cloud_owner_user_id));
  current.wifi_tx_power_dbm_tenths = DEFAULT_WIFI_TX_POWER_DBM_TENTHS;
  current.rf_on_active_high = legacy.rf_on_active_high;
  current.rf_off_active_high = legacy.rf_off_active_high;
  current.rf_alarm_active_high = legacy.rf_alarm_active_high;
  current.status_led_enabled = legacy.status_led_enabled;
  current.status_led_active_high = legacy.status_led_active_high;
  strlcpy(current.ui_password, legacy.ui_password, sizeof(current.ui_password));
  strlcpy(current.ota_url, legacy.ota_url, sizeof(current.ota_url));
  strlcpy(current.ota_channel, legacy.ota_channel, sizeof(current.ota_channel));
}

bool load_legacy_v1(DeviceConfig &config) {
  if (!g_is_open || !g_preferences.isKey(NVS_KEY_CONFIG)) {
    return false;
  }

  LegacyStoredConfigBlobV1 blob = {};
  const size_t read = g_preferences.getBytes(NVS_KEY_CONFIG, &blob, sizeof(blob));
  if (read != sizeof(blob)) {
    return false;
  }
  if (blob.magic != CONFIG_MAGIC || blob.version != 1 ||
      blob.payload_size != sizeof(LegacyDeviceConfigV1)) {
    return false;
  }

  const uint32_t expected_crc =
      crc32_update(0, reinterpret_cast<const uint8_t *>(&blob.payload), sizeof(blob.payload));
  if (expected_crc != blob.crc32) {
    return false;
  }

  copy_legacy_into_current(blob.payload, config);
  return true;
}

bool load_legacy_v2(DeviceConfig &config) {
  if (!g_is_open || !g_preferences.isKey(NVS_KEY_CONFIG)) {
    return false;
  }

  LegacyStoredConfigBlobV2 blob = {};
  const size_t read = g_preferences.getBytes(NVS_KEY_CONFIG, &blob, sizeof(blob));
  if (read != sizeof(blob)) {
    return false;
  }
  if (blob.magic != CONFIG_MAGIC || blob.version != 2 ||
      blob.payload_size != sizeof(LegacyDeviceConfigV2)) {
    return false;
  }

  const uint32_t expected_crc =
      crc32_update(0, reinterpret_cast<const uint8_t *>(&blob.payload), sizeof(blob.payload));
  if (expected_crc != blob.crc32) {
    return false;
  }

  copy_legacy_v2_into_current(blob.payload, config);
  return true;
}

bool load_legacy_v3(DeviceConfig &config) {
  if (!g_is_open || !g_preferences.isKey(NVS_KEY_CONFIG)) {
    return false;
  }

  LegacyStoredConfigBlobV3 blob = {};
  const size_t read = g_preferences.getBytes(NVS_KEY_CONFIG, &blob, sizeof(blob));
  if (read != sizeof(blob)) {
    return false;
  }
  if (blob.magic != CONFIG_MAGIC || blob.version != 3 ||
      blob.payload_size != sizeof(LegacyDeviceConfigV3)) {
    return false;
  }

  const uint32_t expected_crc =
      crc32_update(0, reinterpret_cast<const uint8_t *>(&blob.payload), sizeof(blob.payload));
  if (expected_crc != blob.crc32) {
    return false;
  }

  copy_legacy_v3_into_current(blob.payload, config);
  return true;
}

}  // namespace

void config_manager_apply_defaults(DeviceConfig &config) {
  memset(&config, 0, sizeof(config));
  config.zero_level_mm = DEFAULT_ZERO_LEVEL_MM;
  config.bottom_motor_start_level_mm = DEFAULT_BOTTOM_START_LEVEL_MM;
  config.top_motor_off_level_mm = DEFAULT_TOP_OFF_LEVEL_MM;
  config.overflow_margin_mm = DEFAULT_OVERFLOW_MARGIN_MM;
  config.power_restore_wait_minutes = DEFAULT_POWER_RESTORE_WAIT_MIN;
  config.motor_start_confirm_minutes = DEFAULT_MOTOR_START_CONFIRM_MIN;
  config.motor_off_confirm_minutes = DEFAULT_MOTOR_OFF_CONFIRM_MIN;
  config.water_rise_confirm_mm = DEFAULT_WATER_RISE_CONFIRM_MM;
  config.rf_on_pulse_ms = DEFAULT_RF_ON_PULSE_MS;
  config.rf_off_pulse_ms = DEFAULT_RF_OFF_PULSE_MS;
  config.rf_alarm_pulse_ms = DEFAULT_RF_ALARM_PULSE_MS;
  config.rf_on_max_retries = DEFAULT_RF_ON_MAX_RETRIES;
  config.rf_off_max_retries = DEFAULT_RF_OFF_MAX_RETRIES;
  config.rf_retry_gap_minutes = DEFAULT_RF_RETRY_GAP_MIN;
  config.alarm_repeat_enable = DEFAULT_ALARM_REPEAT_ENABLE ? 1 : 0;
  config.alarm_repeat_minutes = DEFAULT_ALARM_REPEAT_MIN;
  config.telemetry_interval_seconds = DEFAULT_TELEMETRY_INTERVAL_SEC;
  config.wifi_tx_power_dbm_tenths = DEFAULT_WIFI_TX_POWER_DBM_TENTHS;
  config.wifi_mode = TG_WIFI_MODE_AP_FALLBACK;
  config.mqtt_enabled = 0;
  config.mqtt_port = DEFAULT_MQTT_PORT;
  config.rf_on_active_high = DEFAULT_RF_ON_ACTIVE_HIGH;
  config.rf_off_active_high = DEFAULT_RF_OFF_ACTIVE_HIGH;
  config.rf_alarm_active_high = DEFAULT_RF_ALARM_ACTIVE_HIGH;
  config.status_led_enabled = DEFAULT_STATUS_LED_ENABLED;
  config.status_led_active_high = DEFAULT_STATUS_LED_ACTIVE_HIGH;
  copy_default_string(config.device_id, sizeof(config.device_id), DEFAULT_DEVICE_ID);
  copy_default_string(config.site_name, sizeof(config.site_name), DEFAULT_SITE_NAME);
  copy_default_string(config.tank_name, sizeof(config.tank_name), DEFAULT_TANK_NAME);
  config.cloud_base_url[0] = '\0';
  config.device_ingest_key[0] = '\0';
  config.cloud_home_id[0] = '\0';
  config.cloud_owner_user_id[0] = '\0';
  copy_default_string(config.ui_password, sizeof(config.ui_password), DEFAULT_UI_PASSWORD);
  config.ota_url[0] = '\0';
  copy_default_string(config.ota_channel, sizeof(config.ota_channel), "stable");
}

void config_manager_apply_runtime_defaults(RuntimePersist &runtime) {
  memset(&runtime, 0, sizeof(runtime));
  runtime.last_state = TANK_BOOT;
  runtime.last_motor_status = MOTOR_STATUS_UNKNOWN;
  runtime.last_command = CMD_NONE;
  runtime.last_alarm_code = ALARM_NONE;
}

bool config_manager_validate(DeviceConfig &config) {
  config.zero_level_mm = clamp_value<uint16_t>(config.zero_level_mm, 0, 5000);
  config.bottom_motor_start_level_mm =
      clamp_value<uint16_t>(config.bottom_motor_start_level_mm, 10, 4000);
  config.top_motor_off_level_mm =
      clamp_value<uint16_t>(config.top_motor_off_level_mm, 20, 4500);
  if (config.top_motor_off_level_mm <= config.bottom_motor_start_level_mm) {
    config.top_motor_off_level_mm =
        config.bottom_motor_start_level_mm + DEFAULT_WATER_RISE_CONFIRM_MM + 50;
  }
  config.overflow_margin_mm = clamp_value<uint16_t>(config.overflow_margin_mm, 5, 500);
  config.power_restore_wait_minutes =
      clamp_value<uint16_t>(config.power_restore_wait_minutes, 1, 60);
  config.motor_start_confirm_minutes =
      clamp_value<uint16_t>(config.motor_start_confirm_minutes, 1, 60);
  config.motor_off_confirm_minutes =
      clamp_value<uint16_t>(config.motor_off_confirm_minutes, 1, 60);
  config.water_rise_confirm_mm = clamp_value<uint16_t>(config.water_rise_confirm_mm, 3, 500);
  config.rf_on_pulse_ms = clamp_value<uint16_t>(config.rf_on_pulse_ms, RF_MIN_PULSE_MS, RF_MAX_PULSE_MS);
  config.rf_off_pulse_ms =
      clamp_value<uint16_t>(config.rf_off_pulse_ms, RF_MIN_PULSE_MS, RF_MAX_PULSE_MS);
  config.rf_alarm_pulse_ms =
      clamp_value<uint16_t>(config.rf_alarm_pulse_ms, RF_MIN_PULSE_MS, RF_MAX_PULSE_MS);
  config.rf_on_max_retries = clamp_value<uint8_t>(config.rf_on_max_retries, 0, 5);
  config.rf_off_max_retries = clamp_value<uint8_t>(config.rf_off_max_retries, 0, 5);
  config.rf_retry_gap_minutes = clamp_value<uint8_t>(config.rf_retry_gap_minutes, 0, 30);
  config.alarm_repeat_enable = config.alarm_repeat_enable ? 1 : 0;
  config.alarm_repeat_minutes = clamp_value<uint16_t>(config.alarm_repeat_minutes, 1, 120);
  config.telemetry_interval_seconds =
      clamp_value<uint16_t>(config.telemetry_interval_seconds, 5, 3600);
  config.wifi_mode =
      (config.wifi_mode == TG_WIFI_MODE_STA) ? TG_WIFI_MODE_STA : TG_WIFI_MODE_AP_FALLBACK;
  config.mqtt_enabled = config.mqtt_enabled ? 1 : 0;
  config.mqtt_port = clamp_value<uint16_t>(config.mqtt_port, 1, 65535);
  config.wifi_tx_power_dbm_tenths =
      config.wifi_tx_power_dbm_tenths == 0
          ? DEFAULT_WIFI_TX_POWER_DBM_TENTHS
          : sanitize_wifi_tx_power_dbm_tenths(config.wifi_tx_power_dbm_tenths);
  config.rf_on_active_high = config.rf_on_active_high ? 1 : 0;
  config.rf_off_active_high = config.rf_off_active_high ? 1 : 0;
  config.rf_alarm_active_high = config.rf_alarm_active_high ? 1 : 0;
  config.status_led_enabled = config.status_led_enabled ? 1 : 0;
  config.status_led_active_high = config.status_led_active_high ? 1 : 0;

  config.wifi_ssid[sizeof(config.wifi_ssid) - 1] = '\0';
  config.wifi_password[sizeof(config.wifi_password) - 1] = '\0';
  config.mqtt_host[sizeof(config.mqtt_host) - 1] = '\0';
  config.mqtt_username[sizeof(config.mqtt_username) - 1] = '\0';
  config.mqtt_password[sizeof(config.mqtt_password) - 1] = '\0';
  config.device_id[sizeof(config.device_id) - 1] = '\0';
  config.site_name[sizeof(config.site_name) - 1] = '\0';
  config.tank_name[sizeof(config.tank_name) - 1] = '\0';
  config.cloud_base_url[sizeof(config.cloud_base_url) - 1] = '\0';
  config.device_ingest_key[sizeof(config.device_ingest_key) - 1] = '\0';
  config.cloud_home_id[sizeof(config.cloud_home_id) - 1] = '\0';
  config.cloud_owner_user_id[sizeof(config.cloud_owner_user_id) - 1] = '\0';
  config.ui_password[sizeof(config.ui_password) - 1] = '\0';
  config.ota_url[sizeof(config.ota_url) - 1] = '\0';
  config.ota_channel[sizeof(config.ota_channel) - 1] = '\0';

  if (config.device_id[0] == '\0') {
    copy_default_string(config.device_id, sizeof(config.device_id), DEFAULT_DEVICE_ID);
  }
  if (config.site_name[0] == '\0') {
    copy_default_string(config.site_name, sizeof(config.site_name), DEFAULT_SITE_NAME);
  }
  if (config.tank_name[0] == '\0') {
    copy_default_string(config.tank_name, sizeof(config.tank_name), DEFAULT_TANK_NAME);
  }
  if (config.ota_channel[0] == '\0') {
    copy_default_string(config.ota_channel, sizeof(config.ota_channel), "stable");
  }
  if (config.ui_password[0] == '\0') {
    copy_default_string(config.ui_password, sizeof(config.ui_password), DEFAULT_UI_PASSWORD);
  }

  return true;
}

bool config_manager_set_zero(DeviceConfig &config, uint16_t zero_level_mm) {
  config.zero_level_mm = zero_level_mm;
  config_manager_validate(config);
  return config_manager_save_config(config);
}

bool config_manager_init(DeviceConfig &config, RuntimePersist &runtime) {
  config_manager_apply_defaults(config);
  config_manager_apply_runtime_defaults(runtime);

  if (!g_preferences.begin(NVS_NAMESPACE, false)) {
    return false;
  }
  g_is_open = true;

  DeviceConfig loaded_config = {};
  if (load_blob<StoredConfigBlob>(NVS_KEY_CONFIG, CONFIG_MAGIC, CONFIG_SCHEMA_VERSION, loaded_config)) {
    config = loaded_config;
    config_manager_validate(config);
  } else if (load_legacy_v3(config)) {
    config_manager_validate(config);
    config_manager_save_config(config);
  } else if (load_legacy_v2(config)) {
    config_manager_validate(config);
    config_manager_save_config(config);
  } else if (load_legacy_v1(config)) {
    config_manager_validate(config);
    config_manager_save_config(config);
  }

  RuntimePersist loaded_runtime = {};
  if (load_blob<StoredRuntimeBlob>(NVS_KEY_RUNTIME, RUNTIME_MAGIC, RUNTIME_SCHEMA_VERSION,
                                   loaded_runtime)) {
    runtime = loaded_runtime;
  }

  return true;
}

bool config_manager_save_config(const DeviceConfig &config) {
  DeviceConfig sanitized = config;
  config_manager_validate(sanitized);

  DeviceConfig current = {};
  if (load_blob<StoredConfigBlob>(NVS_KEY_CONFIG, CONFIG_MAGIC, CONFIG_SCHEMA_VERSION, current) &&
      blob_matches(current, sanitized)) {
    return true;
  }

  return save_blob<StoredConfigBlob>(NVS_KEY_CONFIG, CONFIG_MAGIC, CONFIG_SCHEMA_VERSION, sanitized);
}

bool config_manager_save_runtime(const RuntimePersist &runtime) {
  RuntimePersist current = {};
  if (load_blob<StoredRuntimeBlob>(NVS_KEY_RUNTIME, RUNTIME_MAGIC, RUNTIME_SCHEMA_VERSION, current) &&
      blob_matches(current, runtime)) {
    return true;
  }

  return save_blob<StoredRuntimeBlob>(NVS_KEY_RUNTIME, RUNTIME_MAGIC, RUNTIME_SCHEMA_VERSION, runtime);
}

void config_manager_factory_reset(DeviceConfig &config, RuntimePersist &runtime) {
  config_manager_apply_defaults(config);
  config_manager_apply_runtime_defaults(runtime);
  if (g_is_open) {
    g_preferences.clear();
  }
  config_manager_save_config(config);
  config_manager_save_runtime(runtime);
}

void config_manager_close() {
  if (g_is_open) {
    g_preferences.end();
    g_is_open = false;
  }
}
