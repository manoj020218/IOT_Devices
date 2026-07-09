#pragma once

#include "project_config.h"

bool rf_output_init(const DeviceConfig &config);
void rf_output_apply_config(const DeviceConfig &config);
void rf_output_update(uint32_t now_ms);
bool rf_output_pulse(LastCommand command, uint16_t pulse_ms);
void rf_output_force_all_off();
const RfRuntime &rf_output_get_runtime();

