#pragma once

#include <Arduino.h>

constexpr char PRODUCT_NAME[] = "Smart Tank Guard by Jenix";
constexpr char PRODUCT_PID[] = "JNX-TG-C3-001";
constexpr char PRODUCT_SKU[] = "JNX-TG-A02YYUW-C3";
constexpr char PRODUCT_FW_NAME[] = "JNX-TG-C3";
constexpr char PRODUCT_FW_SEMVER[] = "1.0.3";
constexpr char PRODUCT_FW_VERSION[] = "JNX-TG-C3 v1.0.3";
constexpr char PRODUCT_BUILD_DATE[] = "2026-07-09";
constexpr char PRODUCT_HW_REVISION[] = "HW1.0";
constexpr char PRODUCT_PROVISION_SERVICE_UUID[] = "FF00";

constexpr uint32_t CONFIG_MAGIC = 0x4A4E5854UL;
constexpr uint16_t CONFIG_SCHEMA_VERSION = 4;
constexpr uint32_t RUNTIME_MAGIC = 0x54475254UL;
constexpr uint16_t RUNTIME_SCHEMA_VERSION = 1;

constexpr char NVS_NAMESPACE[] = "jnx_tg";
constexpr char NVS_KEY_CONFIG[] = "cfg";
constexpr char NVS_KEY_RUNTIME[] = "rt";

constexpr size_t LOG_MAX_LINE_BYTES = 240;
constexpr size_t LOG_MAX_DOWNLOAD_ITEMS = 100;
constexpr char LOG_ACTIVE_FILE_PATH[] = "/events.log";
constexpr char LOG_ARCHIVE_FILE_PATH[] = "/events.prev.log";
constexpr size_t LOG_ROTATE_MAX_BYTES = 128UL * 1024UL;

constexpr uint8_t PIN_SENSOR_RX = 20;
constexpr uint8_t PIN_SENSOR_TX = 21;
constexpr uint8_t PIN_RF_MOTOR_ON = 1;
constexpr uint8_t PIN_RF_MOTOR_OFF = 2;
constexpr uint8_t PIN_RF_ALARM = 3;
constexpr uint8_t PIN_STATUS_LED = 4;

constexpr uint32_t SENSOR_BAUD_RATE = 9600;
constexpr uint16_t SENSOR_MIN_MM = 30;
constexpr uint16_t SENSOR_MAX_MM = 4500;
constexpr uint8_t SENSOR_FILTER_SAMPLES = 7;
constexpr uint16_t SENSOR_MAX_STEP_MM = 400;
constexpr uint32_t SENSOR_FAULT_TIMEOUT_MS = 15000;
constexpr uint32_t SENSOR_STABILIZE_MS = 5000;

constexpr uint8_t TG_WIFI_MODE_AP_FALLBACK = 0;
constexpr uint8_t TG_WIFI_MODE_STA = 1;
constexpr uint8_t AP_CHANNEL = 6;
constexpr uint8_t AP_MAX_CONNECTIONS = 4;
constexpr uint32_t WIFI_CONNECT_RETRY_MS = 15000;
constexpr uint32_t WIFI_AP_RECOVERY_WINDOW_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t WIFI_AP_ONLY_RETRY_MS = 60000;

constexpr uint32_t MQTT_RECONNECT_MS = 10000;
constexpr uint32_t TELEMETRY_FALLBACK_INTERVAL_SEC = 30;
constexpr uint32_t CLOUD_REGISTER_RETRY_MS = 30000;
constexpr uint32_t CLOUD_TELEMETRY_RETRY_MS = 10000;

constexpr uint16_t RF_MIN_PULSE_MS = 50;
constexpr uint16_t RF_MAX_PULSE_MS = 5000;
constexpr uint16_t RF_HARD_TIMEOUT_MS = 6000;

constexpr uint16_t DEFAULT_ZERO_LEVEL_MM = 0;
constexpr uint16_t DEFAULT_BOTTOM_START_LEVEL_MM = 300;
constexpr uint16_t DEFAULT_TOP_OFF_LEVEL_MM = 900;
constexpr uint16_t DEFAULT_OVERFLOW_MARGIN_MM = 30;
constexpr uint16_t DEFAULT_POWER_RESTORE_WAIT_MIN = 3;
constexpr uint16_t DEFAULT_MOTOR_START_CONFIRM_MIN = 3;
constexpr uint16_t DEFAULT_MOTOR_OFF_CONFIRM_MIN = 3;
constexpr uint16_t DEFAULT_WATER_RISE_CONFIRM_MM = 10;
constexpr uint16_t DEFAULT_RF_ON_PULSE_MS = 500;
constexpr uint16_t DEFAULT_RF_OFF_PULSE_MS = 500;
constexpr uint16_t DEFAULT_RF_ALARM_PULSE_MS = 500;
constexpr uint8_t DEFAULT_RF_ON_MAX_RETRIES = 2;
constexpr uint8_t DEFAULT_RF_OFF_MAX_RETRIES = 3;
constexpr uint8_t DEFAULT_RF_RETRY_GAP_MIN = 1;
constexpr bool DEFAULT_ALARM_REPEAT_ENABLE = true;
constexpr uint16_t DEFAULT_ALARM_REPEAT_MIN = 2;
constexpr uint16_t DEFAULT_TELEMETRY_INTERVAL_SEC = 30;
constexpr uint16_t DEFAULT_WIFI_TX_POWER_DBM_TENTHS = 85;

constexpr uint8_t DEFAULT_RF_ON_ACTIVE_HIGH = 1;
constexpr uint8_t DEFAULT_RF_OFF_ACTIVE_HIGH = 1;
constexpr uint8_t DEFAULT_RF_ALARM_ACTIVE_HIGH = 1;
constexpr uint8_t DEFAULT_STATUS_LED_ENABLED = 1;
constexpr uint8_t DEFAULT_STATUS_LED_ACTIVE_HIGH = 1;

constexpr char DEFAULT_DEVICE_ID[] = "tankguard-c3";
constexpr char DEFAULT_SITE_NAME[] = "Default Site";
constexpr char DEFAULT_TANK_NAME[] = "Main Tank";
constexpr char DEFAULT_UI_PASSWORD[] = "Hanuman@2026";

constexpr uint16_t DEFAULT_MQTT_PORT = 1883;

struct WifiTxPowerOption {
  uint16_t dbm_tenths;
  int8_t driver_value;
  const char *label;
};

constexpr WifiTxPowerOption WIFI_TX_POWER_OPTIONS[] = {
    {195, 78, "19.5 dBm"},
    {190, 76, "19 dBm"},
    {185, 74, "18.5 dBm"},
    {170, 68, "17 dBm"},
    {150, 60, "15 dBm"},
    {130, 52, "13 dBm"},
    {110, 44, "11 dBm"},
    {85, 34, "8.5 dBm"},
};

constexpr size_t WIFI_TX_POWER_OPTION_COUNT =
    sizeof(WIFI_TX_POWER_OPTIONS) / sizeof(WIFI_TX_POWER_OPTIONS[0]);

inline uint16_t sanitize_wifi_tx_power_dbm_tenths(int32_t requested) {
  const WifiTxPowerOption *closest = &WIFI_TX_POWER_OPTIONS[0];
  uint32_t closest_diff =
      requested >= static_cast<int32_t>(closest->dbm_tenths)
          ? static_cast<uint32_t>(requested - static_cast<int32_t>(closest->dbm_tenths))
          : static_cast<uint32_t>(static_cast<int32_t>(closest->dbm_tenths) - requested);
  for (size_t i = 1; i < WIFI_TX_POWER_OPTION_COUNT; ++i) {
    const WifiTxPowerOption *candidate = &WIFI_TX_POWER_OPTIONS[i];
    const uint32_t diff =
        requested >= static_cast<int32_t>(candidate->dbm_tenths)
            ? static_cast<uint32_t>(requested - static_cast<int32_t>(candidate->dbm_tenths))
            : static_cast<uint32_t>(static_cast<int32_t>(candidate->dbm_tenths) - requested);
    if (diff < closest_diff) {
      closest = candidate;
      closest_diff = diff;
    }
  }
  return closest->dbm_tenths;
}

inline const WifiTxPowerOption &wifi_tx_power_option_for_dbm_tenths(uint16_t requested) {
  const uint16_t sanitized = sanitize_wifi_tx_power_dbm_tenths(requested);
  for (size_t i = 0; i < WIFI_TX_POWER_OPTION_COUNT; ++i) {
    if (WIFI_TX_POWER_OPTIONS[i].dbm_tenths == sanitized) {
      return WIFI_TX_POWER_OPTIONS[i];
    }
  }
  return WIFI_TX_POWER_OPTIONS[0];
}

inline const WifiTxPowerOption &wifi_tx_power_option_for_driver_value(int32_t driver_value) {
  const WifiTxPowerOption *closest = &WIFI_TX_POWER_OPTIONS[0];
  uint32_t closest_diff =
      driver_value >= static_cast<int32_t>(closest->driver_value)
          ? static_cast<uint32_t>(driver_value - static_cast<int32_t>(closest->driver_value))
          : static_cast<uint32_t>(static_cast<int32_t>(closest->driver_value) - driver_value);
  for (size_t i = 1; i < WIFI_TX_POWER_OPTION_COUNT; ++i) {
    const WifiTxPowerOption *candidate = &WIFI_TX_POWER_OPTIONS[i];
    const uint32_t diff =
        driver_value >= static_cast<int32_t>(candidate->driver_value)
            ? static_cast<uint32_t>(driver_value - static_cast<int32_t>(candidate->driver_value))
            : static_cast<uint32_t>(static_cast<int32_t>(candidate->driver_value) - driver_value);
    if (diff < closest_diff) {
      closest = candidate;
      closest_diff = diff;
    }
  }
  return *closest;
}

inline float wifi_tx_power_dbm_value(uint16_t dbm_tenths) {
  return static_cast<float>(dbm_tenths) / 10.0f;
}

enum SensorStatus : uint8_t {
  SENSOR_OK = 0,
  SENSOR_NO_DATA = 1,
  SENSOR_INVALID = 2,
  SENSOR_FAULT = 3,
};

enum TankState : uint8_t {
  TANK_BOOT = 0,
  TANK_SENSOR_STABILIZE = 1,
  TANK_IDLE = 2,
  TANK_LOW_LEVEL_DETECTED = 3,
  TANK_MOTOR_ON_SENT = 4,
  TANK_WAIT_RISE_CONFIRM = 5,
  TANK_FILLING = 6,
  TANK_TOP_REACHED = 7,
  TANK_MOTOR_OFF_SENT = 8,
  TANK_OFF_CONFIRM = 9,
  TANK_ALARM = 10,
  TANK_SENSOR_FAULT = 11,
};

enum MotorStatus : uint8_t {
  MOTOR_STATUS_UNKNOWN = 0,
  MOTOR_STATUS_ASSUMED_OFF = 1,
  MOTOR_STATUS_ASSUMED_ON = 2,
};

enum LastCommand : uint8_t {
  CMD_NONE = 0,
  CMD_MOTOR_ON = 1,
  CMD_MOTOR_OFF = 2,
  CMD_ALARM = 3,
};

enum WaterTrend : uint8_t {
  WATER_TREND_UNKNOWN = 0,
  WATER_TREND_STABLE = 1,
  WATER_TREND_RISING = 2,
  WATER_TREND_FALLING = 3,
};

enum AlarmCode : uint8_t {
  ALARM_NONE = 0,
  ALARM_DRY_RUN_OR_MOTOR_MALFUNCTION = 1,
  ALARM_COMMUNITY_SUPPLY_OR_MANUAL_OVERRIDE_OR_OFF_FAILED = 2,
  ALARM_OVERFLOW_ABOVE_TOP_LEVEL = 3,
  ALARM_SENSOR_FAULT = 4,
  ALARM_SENSOR_INVALID = 5,
};

struct DeviceConfig {
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
  uint16_t wifi_tx_power_dbm_tenths;
  uint8_t rf_on_active_high;
  uint8_t rf_off_active_high;
  uint8_t rf_alarm_active_high;
  uint8_t status_led_enabled;
  uint8_t status_led_active_high;
  char ui_password[32];
  char ota_url[192];
  char ota_channel[24];
};

struct RuntimePersist {
  TankState last_state;
  MotorStatus last_motor_status;
  LastCommand last_command;
  AlarmCode last_alarm_code;
  uint16_t last_water_level_mm;
  uint16_t last_distance_mm;
};

struct SensorSnapshot {
  SensorStatus status;
  uint16_t raw_distance_mm;
  uint16_t filtered_distance_mm;
  uint32_t last_valid_ms;
  bool valid_frame_seen;
};

struct RfRuntime {
  bool motor_on_active;
  bool motor_off_active;
  bool alarm_active;
  LastCommand last_command;
  uint32_t last_command_ms;
  uint32_t rf_on_count;
  uint32_t rf_off_count;
  uint32_t rf_alarm_count;
};

struct TankRuntime {
  TankState state;
  MotorStatus motor_status;
  LastCommand last_command;
  AlarmCode alarm_code;
  bool alarm_active;
  uint16_t water_level_mm;
  uint16_t zero_level_mm;
  uint16_t bottom_level_mm;
  uint16_t top_level_mm;
  uint16_t overflow_margin_mm;
  WaterTrend water_trend;
  int16_t rise_rate_mm_per_min;
  uint32_t last_command_ms;
  uint8_t on_retry_count;
  uint8_t off_retry_count;
  char last_alarm_message[96];
  bool manual_override_active;
};

struct SystemStatus {
  char device_name[16];
  char mdns_name[24];
  char mac_address[18];
  char ap_ip[16];
  char sta_ip[16];
  char active_wifi_ssid[33];
  char local_ip[16];
  char local_url[48];
  bool ap_active;
  bool sta_connected;
  bool mqtt_connected;
  bool ble_active;
  bool cloud_registered;
  int16_t wifi_rssi_dbm;
  uint16_t wifi_tx_power_dbm_tenths;
  uint32_t uptime_s;
  uint32_t ap_recovery_remaining_s;
};

struct OtaRequest {
  char url[192];
  char version[32];
  char checksum[80];
};

inline const char *sensor_status_to_string(SensorStatus status) {
  switch (status) {
    case SENSOR_OK: return "ok";
    case SENSOR_NO_DATA: return "no_data";
    case SENSOR_INVALID: return "invalid";
    case SENSOR_FAULT: return "fault";
    default: return "fault";
  }
}

inline const char *tank_state_to_string(TankState state) {
  switch (state) {
    case TANK_BOOT: return "BOOT";
    case TANK_SENSOR_STABILIZE: return "SENSOR_STABILIZE";
    case TANK_IDLE: return "IDLE";
    case TANK_LOW_LEVEL_DETECTED: return "LOW_LEVEL_DETECTED";
    case TANK_MOTOR_ON_SENT: return "MOTOR_ON_SENT";
    case TANK_WAIT_RISE_CONFIRM: return "WAIT_RISE_CONFIRM";
    case TANK_FILLING: return "FILLING";
    case TANK_TOP_REACHED: return "TOP_REACHED";
    case TANK_MOTOR_OFF_SENT: return "MOTOR_OFF_SENT";
    case TANK_OFF_CONFIRM: return "OFF_CONFIRM";
    case TANK_ALARM: return "ALARM";
    case TANK_SENSOR_FAULT: return "SENSOR_FAULT";
    default: return "IDLE";
  }
}

inline const char *motor_status_to_string(MotorStatus status) {
  switch (status) {
    case MOTOR_STATUS_ASSUMED_OFF: return "Assumed OFF";
    case MOTOR_STATUS_ASSUMED_ON: return "Assumed ON";
    case MOTOR_STATUS_UNKNOWN: return "Unknown";
    default: return "Unknown";
  }
}

inline const char *last_command_to_string(LastCommand command) {
  switch (command) {
    case CMD_MOTOR_ON: return "MOTOR_ON";
    case CMD_MOTOR_OFF: return "MOTOR_OFF";
    case CMD_ALARM: return "ALARM";
    case CMD_NONE: return "NONE";
    default: return "NONE";
  }
}

inline const char *water_trend_to_string(WaterTrend trend) {
  switch (trend) {
    case WATER_TREND_RISING: return "rising";
    case WATER_TREND_FALLING: return "falling";
    case WATER_TREND_STABLE: return "stable";
    case WATER_TREND_UNKNOWN: return "unknown";
    default: return "unknown";
  }
}

inline const char *alarm_code_to_string(AlarmCode code) {
  switch (code) {
    case ALARM_DRY_RUN_OR_MOTOR_MALFUNCTION: return "DRY_RUN_OR_MOTOR_MALFUNCTION";
    case ALARM_COMMUNITY_SUPPLY_OR_MANUAL_OVERRIDE_OR_OFF_FAILED:
      return "COMMUNITY_SUPPLY_OR_MANUAL_OVERRIDE_OR_OFF_FAILED";
    case ALARM_OVERFLOW_ABOVE_TOP_LEVEL: return "OVERFLOW_ABOVE_TOP_LEVEL";
    case ALARM_SENSOR_FAULT: return "SENSOR_FAULT";
    case ALARM_SENSOR_INVALID: return "SENSOR_INVALID";
    case ALARM_NONE: return "";
    default: return "";
  }
}
