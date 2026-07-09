#include "tank_logic.h"

#include <cstring>

#include "logger.h"
#include "rf_output.h"

namespace {

TankRuntime g_runtime = {};
TankEvent g_pending_event = {};
bool g_has_pending_event = false;

uint32_t g_state_started_ms = 0;
uint32_t g_command_started_ms = 0;
uint32_t g_last_alarm_pulse_ms = 0;
uint16_t g_confirm_baseline_level_mm = 0;
uint32_t g_last_trend_sample_ms = 0;
uint16_t g_last_trend_sample_level_mm = 0;
bool g_restore_from_motor_on = false;

void queue_event(const char *topic_suffix, const char *code, const char *message) {
  strlcpy(g_pending_event.topic_suffix, topic_suffix, sizeof(g_pending_event.topic_suffix));
  strlcpy(g_pending_event.code, code, sizeof(g_pending_event.code));
  strlcpy(g_pending_event.message, message, sizeof(g_pending_event.message));
  g_has_pending_event = true;
}

void set_state(TankState state) {
  if (g_runtime.state == state) {
    return;
  }
  g_runtime.state = state;
  g_state_started_ms = millis();
  char message[96];
  snprintf(message, sizeof(message), "State changed to %s", tank_state_to_string(state));
  logger_log("INFO", "state", "STATE_CHANGE", message);
  queue_event("event", "STATE_CHANGE", message);
}

void set_alarm(AlarmCode code, const char *message) {
  g_runtime.alarm_active = (code != ALARM_NONE);
  g_runtime.alarm_code = code;
  strlcpy(g_runtime.last_alarm_message, message, sizeof(g_runtime.last_alarm_message));
  logger_log(code == ALARM_NONE ? "INFO" : "WARN", "alarm", alarm_code_to_string(code), message);
  queue_event("alarm", alarm_code_to_string(code), message);
  if (code != ALARM_NONE) {
    set_state(TANK_ALARM);
  }
}

uint16_t safe_water_level(const DeviceConfig &config, const SensorSnapshot &sensor) {
  if (sensor.status != SENSOR_OK || config.zero_level_mm == 0) {
    return 0;
  }
  if (sensor.filtered_distance_mm >= config.zero_level_mm) {
    return 0;
  }
  return static_cast<uint16_t>(config.zero_level_mm - sensor.filtered_distance_mm);
}

bool send_rf_command(const DeviceConfig &config, LastCommand command) {
  bool result = false;
  if (command == CMD_MOTOR_ON) {
    result = rf_output_pulse(command, config.rf_on_pulse_ms);
  } else if (command == CMD_MOTOR_OFF) {
    result = rf_output_pulse(command, config.rf_off_pulse_ms);
  } else if (command == CMD_ALARM) {
    result = rf_output_pulse(command, config.rf_alarm_pulse_ms);
  }

  if (result) {
    g_runtime.last_command = command;
    g_runtime.last_command_ms = millis();
    logger_log("INFO", "rf", last_command_to_string(command), "RF pulse sent");
  }
  return result;
}

void update_trend(uint32_t now_ms, uint16_t water_level_mm) {
  if (g_last_trend_sample_ms == 0) {
    g_last_trend_sample_ms = now_ms;
    g_last_trend_sample_level_mm = water_level_mm;
    g_runtime.water_trend = WATER_TREND_UNKNOWN;
    g_runtime.rise_rate_mm_per_min = 0;
    return;
  }

  const uint32_t elapsed = now_ms - g_last_trend_sample_ms;
  if (elapsed < 30000UL) {
    return;
  }

  const int16_t delta = static_cast<int16_t>(water_level_mm) -
                        static_cast<int16_t>(g_last_trend_sample_level_mm);
  g_runtime.rise_rate_mm_per_min = static_cast<int16_t>((static_cast<int32_t>(delta) * 60000L) /
                                                         static_cast<int32_t>(elapsed));
  if (delta >= 5) {
    g_runtime.water_trend = WATER_TREND_RISING;
  } else if (delta <= -5) {
    g_runtime.water_trend = WATER_TREND_FALLING;
  } else {
    g_runtime.water_trend = WATER_TREND_STABLE;
  }

  g_last_trend_sample_ms = now_ms;
  g_last_trend_sample_level_mm = water_level_mm;
}

void handle_overflow_alarm(const DeviceConfig &config, uint32_t now_ms) {
  if (config.alarm_repeat_enable == 0) {
    return;
  }

  const uint32_t repeat_ms = static_cast<uint32_t>(config.alarm_repeat_minutes) * 60000UL;
  if (g_last_alarm_pulse_ms == 0 || (now_ms - g_last_alarm_pulse_ms) >= repeat_ms) {
    if (send_rf_command(config, CMD_ALARM)) {
      g_last_alarm_pulse_ms = now_ms;
    }
  }
}

void maybe_start_fill_sequence(const DeviceConfig &config, uint16_t water_level_mm) {
  if (water_level_mm > config.bottom_motor_start_level_mm) {
    return;
  }
  set_state(TANK_LOW_LEVEL_DETECTED);
  if (send_rf_command(config, CMD_MOTOR_ON)) {
    g_runtime.motor_status = MOTOR_STATUS_ASSUMED_ON;
    g_runtime.on_retry_count = 0;
    g_command_started_ms = millis();
    g_confirm_baseline_level_mm = water_level_mm;
    set_state(TANK_MOTOR_ON_SENT);
    set_state(TANK_WAIT_RISE_CONFIRM);
  }
}

}  // namespace

void tank_logic_init(const DeviceConfig &config, const RuntimePersist &persisted) {
  memset(&g_runtime, 0, sizeof(g_runtime));
  g_runtime.state = TANK_BOOT;
  g_runtime.motor_status = persisted.last_motor_status;
  g_runtime.last_command = persisted.last_command;
  g_runtime.alarm_code = persisted.last_alarm_code;
  g_runtime.zero_level_mm = config.zero_level_mm;
  g_runtime.bottom_level_mm = config.bottom_motor_start_level_mm;
  g_runtime.top_level_mm = config.top_motor_off_level_mm;
  g_runtime.overflow_margin_mm = config.overflow_margin_mm;
  g_runtime.water_trend = WATER_TREND_UNKNOWN;
  g_state_started_ms = millis();
  g_command_started_ms = 0;
  g_last_alarm_pulse_ms = 0;
  g_confirm_baseline_level_mm = 0;
  g_last_trend_sample_ms = 0;
  g_last_trend_sample_level_mm = 0;
  g_restore_from_motor_on = (persisted.last_command == CMD_MOTOR_ON);
  g_has_pending_event = false;
  g_runtime.last_alarm_message[0] = '\0';
  set_state(TANK_SENSOR_STABILIZE);
}

void tank_logic_update(uint32_t now_ms, const DeviceConfig &config, const SensorSnapshot &sensor) {
  g_runtime.zero_level_mm = config.zero_level_mm;
  g_runtime.bottom_level_mm = config.bottom_motor_start_level_mm;
  g_runtime.top_level_mm = config.top_motor_off_level_mm;
  g_runtime.overflow_margin_mm = config.overflow_margin_mm;
  g_runtime.water_level_mm = safe_water_level(config, sensor);

  update_trend(now_ms, g_runtime.water_level_mm);

  if (sensor.status == SENSOR_FAULT || sensor.status == SENSOR_NO_DATA) {
    if (g_runtime.state != TANK_SENSOR_FAULT) {
      set_alarm(ALARM_SENSOR_FAULT, "Sensor fault or stale data");
      set_state(TANK_SENSOR_FAULT);
    }
    return;
  }

  if (sensor.status == SENSOR_INVALID) {
    set_alarm(ALARM_SENSOR_INVALID, "Invalid sensor frames detected");
  } else if (g_runtime.alarm_code == ALARM_SENSOR_INVALID) {
    set_alarm(ALARM_NONE, "");
  }

  if (g_runtime.state == TANK_SENSOR_FAULT) {
    g_runtime.alarm_active = false;
    g_runtime.alarm_code = ALARM_NONE;
    g_runtime.last_alarm_message[0] = '\0';
    set_state(TANK_SENSOR_STABILIZE);
  }

  if (g_runtime.water_level_mm >=
      static_cast<uint16_t>(config.top_motor_off_level_mm + config.overflow_margin_mm)) {
    if (g_runtime.alarm_code != ALARM_OVERFLOW_ABOVE_TOP_LEVEL) {
      set_alarm(ALARM_OVERFLOW_ABOVE_TOP_LEVEL, "Overflow detected above top threshold");
      send_rf_command(config, CMD_ALARM);
      g_last_alarm_pulse_ms = now_ms;
    }
    handle_overflow_alarm(config, now_ms);
  } else if (g_runtime.alarm_code == ALARM_OVERFLOW_ABOVE_TOP_LEVEL) {
    set_alarm(ALARM_NONE, "");
  }

  switch (g_runtime.state) {
    case TANK_BOOT:
      set_state(TANK_SENSOR_STABILIZE);
      break;

    case TANK_SENSOR_STABILIZE:
      if (sensor.last_valid_ms != 0 && (now_ms - sensor.last_valid_ms) < SENSOR_STABILIZE_MS) {
        break;
      }
      if (g_restore_from_motor_on && g_runtime.water_level_mm < config.top_motor_off_level_mm) {
        g_runtime.motor_status = MOTOR_STATUS_ASSUMED_ON;
        g_confirm_baseline_level_mm = g_runtime.water_level_mm;
        g_command_started_ms = now_ms;
        set_state(TANK_WAIT_RISE_CONFIRM);
      } else {
        set_state(TANK_IDLE);
      }
      break;

    case TANK_IDLE:
      maybe_start_fill_sequence(config, g_runtime.water_level_mm);
      break;

    case TANK_LOW_LEVEL_DETECTED:
      maybe_start_fill_sequence(config, g_runtime.water_level_mm);
      break;

    case TANK_MOTOR_ON_SENT:
      set_state(TANK_WAIT_RISE_CONFIRM);
      break;

    case TANK_WAIT_RISE_CONFIRM: {
      const uint32_t confirm_ms = static_cast<uint32_t>(config.motor_start_confirm_minutes) * 60000UL;
      if (g_runtime.water_level_mm >=
          static_cast<uint16_t>(g_confirm_baseline_level_mm + config.water_rise_confirm_mm)) {
        set_state(TANK_FILLING);
        break;
      }
      if ((now_ms - g_command_started_ms) < confirm_ms) {
        break;
      }
      if (g_runtime.on_retry_count < config.rf_on_max_retries) {
        const uint32_t retry_gap_ms = static_cast<uint32_t>(config.rf_retry_gap_minutes) * 60000UL;
        if ((now_ms - g_runtime.last_command_ms) >= retry_gap_ms) {
          ++g_runtime.on_retry_count;
          send_rf_command(config, CMD_MOTOR_ON);
          g_command_started_ms = now_ms;
          g_confirm_baseline_level_mm = g_runtime.water_level_mm;
        }
      } else {
        set_alarm(ALARM_DRY_RUN_OR_MOTOR_MALFUNCTION,
                  "Motor assumed ON but water level did not rise");
      }
      break;
    }

    case TANK_FILLING:
      if (g_runtime.water_level_mm >= config.top_motor_off_level_mm) {
        set_state(TANK_TOP_REACHED);
      }
      break;

    case TANK_TOP_REACHED:
      if (send_rf_command(config, CMD_MOTOR_OFF)) {
        g_runtime.motor_status = MOTOR_STATUS_ASSUMED_OFF;
        g_runtime.off_retry_count = 0;
        g_command_started_ms = now_ms;
        g_confirm_baseline_level_mm = g_runtime.water_level_mm;
        set_state(TANK_MOTOR_OFF_SENT);
        set_state(TANK_OFF_CONFIRM);
      }
      break;

    case TANK_MOTOR_OFF_SENT:
      set_state(TANK_OFF_CONFIRM);
      break;

    case TANK_OFF_CONFIRM: {
      const uint32_t confirm_ms = static_cast<uint32_t>(config.motor_off_confirm_minutes) * 60000UL;
      const bool still_rising =
          g_runtime.water_level_mm >
          static_cast<uint16_t>(g_confirm_baseline_level_mm + config.water_rise_confirm_mm);
      if (still_rising && g_runtime.water_level_mm > config.top_motor_off_level_mm) {
        if (g_runtime.off_retry_count < config.rf_off_max_retries) {
          const uint32_t retry_gap_ms = static_cast<uint32_t>(config.rf_retry_gap_minutes) * 60000UL;
          if ((now_ms - g_runtime.last_command_ms) >= retry_gap_ms) {
            ++g_runtime.off_retry_count;
            send_rf_command(config, CMD_MOTOR_OFF);
          }
        } else {
          set_alarm(ALARM_COMMUNITY_SUPPLY_OR_MANUAL_OVERRIDE_OR_OFF_FAILED,
                    "Water kept rising after OFF command");
        }
      }
      if ((now_ms - g_command_started_ms) >= confirm_ms && !still_rising) {
        g_runtime.manual_override_active = false;
        set_state(TANK_IDLE);
      }
      break;
    }

    case TANK_ALARM:
      if (!g_runtime.alarm_active) {
        set_state(TANK_IDLE);
      }
      break;

    case TANK_SENSOR_FAULT:
      break;
  }
}

const TankRuntime &tank_logic_get_runtime() {
  return g_runtime;
}

void tank_logic_build_runtime_persist(RuntimePersist &persist) {
  persist.last_state = g_runtime.state;
  persist.last_motor_status = g_runtime.motor_status;
  persist.last_command = g_runtime.last_command;
  persist.last_alarm_code = g_runtime.alarm_code;
  persist.last_water_level_mm = g_runtime.water_level_mm;
  persist.last_distance_mm = g_runtime.zero_level_mm > g_runtime.water_level_mm
                                 ? static_cast<uint16_t>(g_runtime.zero_level_mm - g_runtime.water_level_mm)
                                 : 0;
}

bool tank_logic_manual_motor_on(const DeviceConfig &config, bool override_top_level) {
  if (!override_top_level && g_runtime.water_level_mm >= config.top_motor_off_level_mm) {
    return false;
  }
  if (!send_rf_command(config, CMD_MOTOR_ON)) {
    return false;
  }
  g_runtime.manual_override_active = override_top_level;
  g_runtime.motor_status = MOTOR_STATUS_ASSUMED_ON;
  g_runtime.on_retry_count = 0;
  g_confirm_baseline_level_mm = g_runtime.water_level_mm;
  g_command_started_ms = millis();
  set_state(TANK_WAIT_RISE_CONFIRM);
  return true;
}

bool tank_logic_manual_motor_off(const DeviceConfig &config) {
  if (!send_rf_command(config, CMD_MOTOR_OFF)) {
    return false;
  }
  g_runtime.manual_override_active = false;
  g_runtime.motor_status = MOTOR_STATUS_ASSUMED_OFF;
  g_runtime.off_retry_count = 0;
  g_confirm_baseline_level_mm = g_runtime.water_level_mm;
  g_command_started_ms = millis();
  set_state(TANK_OFF_CONFIRM);
  return true;
}

bool tank_logic_manual_alarm_test(const DeviceConfig &config) {
  return send_rf_command(config, CMD_ALARM);
}

bool tank_logic_pop_event(TankEvent &event_out) {
  if (!g_has_pending_event) {
    return false;
  }
  event_out = g_pending_event;
  g_has_pending_event = false;
  return true;
}

bool tank_logic_set_zero_reference(uint16_t zero_mm) {
  g_runtime.zero_level_mm = zero_mm;
  return true;
}
