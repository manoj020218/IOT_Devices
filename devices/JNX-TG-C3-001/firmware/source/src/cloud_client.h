#pragma once

#include "project_config.h"

bool cloud_client_init();
void cloud_client_update(uint32_t now_ms, const DeviceConfig &config, const SystemStatus &system,
                         const SensorSnapshot &sensor, const TankRuntime &tank, const RfRuntime &rf);
bool cloud_client_is_registered();
const char *cloud_client_phase();
const char *cloud_client_message();
