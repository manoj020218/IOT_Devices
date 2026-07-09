#pragma once

#include "project_config.h"

bool sensor_dyp_init();
void sensor_dyp_update(uint32_t now_ms);
const SensorSnapshot &sensor_dyp_get_snapshot();
void sensor_dyp_reset();

