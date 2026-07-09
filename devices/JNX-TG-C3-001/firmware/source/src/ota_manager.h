#pragma once

#include "project_config.h"

bool ota_manager_init();
void ota_manager_mark_app_valid();
bool ota_manager_begin_local(size_t size_bytes, char *error, size_t error_size);
bool ota_manager_write_local(const uint8_t *data, size_t data_len, char *error, size_t error_size);
bool ota_manager_finalize_local(char *error, size_t error_size);
void ota_manager_abort_local(const char *reason);
bool ota_manager_request_cloud_update(const OtaRequest &request, char *error, size_t error_size);
void ota_manager_update(uint32_t now_ms);
bool ota_manager_is_busy();
bool ota_manager_should_reboot();
void ota_manager_clear_reboot_flag();
const char *ota_manager_phase();
const char *ota_manager_message();

