#include "rf_output.h"

namespace {

struct PulseChannel {
  uint8_t pin;
  bool active_high;
  bool active;
  uint32_t started_ms;
  uint16_t duration_ms;
};

PulseChannel g_motor_on = {PIN_RF_MOTOR_ON, true, false, 0, 0};
PulseChannel g_motor_off = {PIN_RF_MOTOR_OFF, true, false, 0, 0};
PulseChannel g_alarm = {PIN_RF_ALARM, true, false, 0, 0};
RfRuntime g_runtime = {};

void write_channel(const PulseChannel &channel, bool active) {
  const bool output_level = channel.active_high ? active : !active;
  digitalWrite(channel.pin, output_level ? HIGH : LOW);
}

void begin_channel(PulseChannel &channel, uint32_t now_ms, uint16_t duration_ms) {
  channel.active = true;
  channel.started_ms = now_ms;
  channel.duration_ms = duration_ms;
  write_channel(channel, true);
}

void stop_channel(PulseChannel &channel) {
  channel.active = false;
  channel.duration_ms = 0;
  write_channel(channel, false);
}

PulseChannel *channel_for_command(LastCommand command) {
  switch (command) {
    case CMD_MOTOR_ON: return &g_motor_on;
    case CMD_MOTOR_OFF: return &g_motor_off;
    case CMD_ALARM: return &g_alarm;
    default: return nullptr;
  }
}

}  // namespace

bool rf_output_init(const DeviceConfig &config) {
  pinMode(PIN_RF_MOTOR_ON, OUTPUT);
  pinMode(PIN_RF_MOTOR_OFF, OUTPUT);
  pinMode(PIN_RF_ALARM, OUTPUT);
  rf_output_apply_config(config);
  rf_output_force_all_off();
  memset(&g_runtime, 0, sizeof(g_runtime));
  g_runtime.last_command = CMD_NONE;
  return true;
}

void rf_output_apply_config(const DeviceConfig &config) {
  g_motor_on.active_high = config.rf_on_active_high != 0;
  g_motor_off.active_high = config.rf_off_active_high != 0;
  g_alarm.active_high = config.rf_alarm_active_high != 0;
  if (!g_motor_on.active) {
    write_channel(g_motor_on, false);
  }
  if (!g_motor_off.active) {
    write_channel(g_motor_off, false);
  }
  if (!g_alarm.active) {
    write_channel(g_alarm, false);
  }
}

bool rf_output_pulse(LastCommand command, uint16_t pulse_ms) {
  PulseChannel *channel = channel_for_command(command);
  if (channel == nullptr) {
    return false;
  }

  const uint16_t bounded_pulse = constrain(pulse_ms, RF_MIN_PULSE_MS, RF_MAX_PULSE_MS);
  const uint32_t now_ms = millis();
  begin_channel(*channel, now_ms, bounded_pulse);

  g_runtime.last_command = command;
  g_runtime.last_command_ms = now_ms;
  if (command == CMD_MOTOR_ON) {
    ++g_runtime.rf_on_count;
  } else if (command == CMD_MOTOR_OFF) {
    ++g_runtime.rf_off_count;
  } else if (command == CMD_ALARM) {
    ++g_runtime.rf_alarm_count;
  }
  return true;
}

void rf_output_update(uint32_t now_ms) {
  PulseChannel *channels[] = {&g_motor_on, &g_motor_off, &g_alarm};
  for (size_t i = 0; i < 3; ++i) {
    PulseChannel *channel = channels[i];
    if (!channel->active) {
      continue;
    }
    const uint32_t elapsed = now_ms - channel->started_ms;
    if (elapsed >= channel->duration_ms || elapsed >= RF_HARD_TIMEOUT_MS) {
      stop_channel(*channel);
    }
  }

  g_runtime.motor_on_active = g_motor_on.active;
  g_runtime.motor_off_active = g_motor_off.active;
  g_runtime.alarm_active = g_alarm.active;
}

void rf_output_force_all_off() {
  stop_channel(g_motor_on);
  stop_channel(g_motor_off);
  stop_channel(g_alarm);
  g_runtime.motor_on_active = false;
  g_runtime.motor_off_active = false;
  g_runtime.alarm_active = false;
}

const RfRuntime &rf_output_get_runtime() {
  return g_runtime;
}

