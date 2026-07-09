#pragma once

#include "project_config.h"

struct TankEvent {
  char topic_suffix[16];
  char code[48];
  char message[128];
};

void tank_logic_init(const DeviceConfig &config, const RuntimePersist &persisted);
void tank_logic_update(uint32_t now_ms, const DeviceConfig &config, const SensorSnapshot &sensor);
const TankRuntime &tank_logic_get_runtime();
void tank_logic_build_runtime_persist(RuntimePersist &persist);
bool tank_logic_manual_motor_on(const DeviceConfig &config, bool override_top_level);
bool tank_logic_manual_motor_off(const DeviceConfig &config);
bool tank_logic_manual_alarm_test(const DeviceConfig &config);
bool tank_logic_pop_event(TankEvent &event_out);
bool tank_logic_set_zero_reference(uint16_t zero_mm);

