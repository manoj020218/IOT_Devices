#pragma once

#include "project_config.h"

bool config_manager_init(DeviceConfig &config, RuntimePersist &runtime);
bool config_manager_save_config(const DeviceConfig &config);
bool config_manager_save_runtime(const RuntimePersist &runtime);
void config_manager_factory_reset(DeviceConfig &config, RuntimePersist &runtime);
void config_manager_apply_defaults(DeviceConfig &config);
void config_manager_apply_runtime_defaults(RuntimePersist &runtime);
bool config_manager_validate(DeviceConfig &config);
bool config_manager_set_zero(DeviceConfig &config, uint16_t zero_level_mm);
void config_manager_close();

