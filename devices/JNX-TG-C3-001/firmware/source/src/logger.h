#pragma once

#include "project_config.h"

bool logger_init();
void logger_log(const char *level, const char *category, const char *code, const char *message);
String logger_get_recent_json(size_t max_items);
String logger_get_recent_csv(size_t max_items);
void logger_rotate_if_needed();

