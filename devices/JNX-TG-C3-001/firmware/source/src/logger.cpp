#include "logger.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace {

bool g_logger_ready = false;
bool g_littlefs_ready = false;
char g_log_lines[LOG_MAX_DOWNLOAD_ITEMS][LOG_MAX_LINE_BYTES] = {};
size_t g_log_count = 0;
size_t g_log_next = 0;

void append_line_to_memory(const char *line) {
  if (!g_logger_ready) {
    return;
  }

  strlcpy(g_log_lines[g_log_next], line, sizeof(g_log_lines[g_log_next]));
  g_log_next = (g_log_next + 1U) % LOG_MAX_DOWNLOAD_ITEMS;
  if (g_log_count < LOG_MAX_DOWNLOAD_ITEMS) {
    ++g_log_count;
  }
}

void clear_memory_cache() {
  memset(g_log_lines, 0, sizeof(g_log_lines));
  g_log_count = 0;
  g_log_next = 0;
}

bool filesystem_has_file(const char *path) {
  if (!g_littlefs_ready) {
    return false;
  }

  File root = LittleFS.open("/");
  if (!root) {
    return false;
  }

  bool found = false;
  File entry = root.openNextFile();
  while (entry) {
    if (!entry.isDirectory() && strcmp(entry.path(), path) == 0) {
      found = true;
      entry.close();
      break;
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
  return found;
}

void append_file_lines_to_memory(const char *path) {
  if (!g_littlefs_ready || !filesystem_has_file(path)) {
    return;
  }

  File file = LittleFS.open(path, FILE_READ);
  if (!file) {
    return;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      continue;
    }
    append_line_to_memory(line.c_str());
  }
  file.close();
}

void rebuild_memory_cache_from_files() {
  clear_memory_cache();
  append_file_lines_to_memory(LOG_ARCHIVE_FILE_PATH);
  append_file_lines_to_memory(LOG_ACTIVE_FILE_PATH);
}

void append_line_to_file(const char *line) {
  if (!g_littlefs_ready) {
    return;
  }

  File file = LittleFS.open(LOG_ACTIVE_FILE_PATH, FILE_APPEND);
  if (!file) {
    Serial.println("[LOG] Failed to open active log file");
    return;
  }
  file.println(line);
  file.close();
}

void read_recent_lines(String *out_lines, size_t &line_count, size_t max_items) {
  const size_t available = g_log_count;
  line_count = available < max_items ? available : max_items;
  if (line_count == 0) {
    return;
  }

  size_t start = (available == LOG_MAX_DOWNLOAD_ITEMS) ? g_log_next : 0;
  if (available > line_count) {
    start = (start + (available - line_count)) % LOG_MAX_DOWNLOAD_ITEMS;
  }

  for (size_t i = 0; i < line_count; ++i) {
    const size_t index = (start + i) % LOG_MAX_DOWNLOAD_ITEMS;
    out_lines[i] = g_log_lines[index];
  }
}

}  // namespace

bool logger_init() {
  clear_memory_cache();
  g_littlefs_ready = LittleFS.begin(false);
  g_logger_ready = true;
  if (g_littlefs_ready) {
    logger_rotate_if_needed();
    rebuild_memory_cache_from_files();
  } else {
    Serial.println("[LOG] LittleFS mount failed, using memory-only logs until filesystem image is present");
  }
  return g_logger_ready;
}

void logger_log(const char *level, const char *category, const char *code, const char *message) {
  StaticJsonDocument<256> doc;
  doc["ts_ms"] = static_cast<unsigned long>(millis());
  doc["level"] = level;
  doc["category"] = category;
  doc["code"] = code;
  doc["message"] = message;

  char line[LOG_MAX_LINE_BYTES] = {};
  serializeJson(doc, line, sizeof(line));

  append_line_to_memory(line);
  append_line_to_file(line);
  logger_rotate_if_needed();
}

String logger_get_recent_json(size_t max_items) {
  if (max_items == 0) {
    max_items = LOG_MAX_DOWNLOAD_ITEMS;
  }
  if (max_items > LOG_MAX_DOWNLOAD_ITEMS) {
    max_items = LOG_MAX_DOWNLOAD_ITEMS;
  }

  String lines[LOG_MAX_DOWNLOAD_ITEMS];
  size_t line_count = 0;
  read_recent_lines(lines, line_count, max_items);

  String json = "[";
  for (size_t i = 0; i < line_count; ++i) {
    if (i > 0) {
      json += ',';
    }
    json += lines[i];
  }
  json += "]";
  return json;
}

String logger_get_recent_csv(size_t max_items) {
  if (max_items == 0) {
    max_items = LOG_MAX_DOWNLOAD_ITEMS;
  }
  if (max_items > LOG_MAX_DOWNLOAD_ITEMS) {
    max_items = LOG_MAX_DOWNLOAD_ITEMS;
  }

  String lines[LOG_MAX_DOWNLOAD_ITEMS];
  size_t line_count = 0;
  read_recent_lines(lines, line_count, max_items);

  String csv = "ts_ms,level,category,code,message\n";
  for (size_t i = 0; i < line_count; ++i) {
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, lines[i]) != DeserializationError::Ok) {
      continue;
    }
    csv += String(static_cast<unsigned long>(doc["ts_ms"] | 0));
    csv += ',';
    csv += String(static_cast<const char *>(doc["level"] | ""));
    csv += ',';
    csv += String(static_cast<const char *>(doc["category"] | ""));
    csv += ',';
    csv += String(static_cast<const char *>(doc["code"] | ""));
    csv += ',';
    String message = String(static_cast<const char *>(doc["message"] | ""));
    message.replace(",", " ");
    csv += message;
    csv += '\n';
  }
  return csv;
}

void logger_rotate_if_needed() {
  if (!g_littlefs_ready || !filesystem_has_file(LOG_ACTIVE_FILE_PATH)) {
    return;
  }

  File active = LittleFS.open(LOG_ACTIVE_FILE_PATH, FILE_READ);
  if (!active) {
    return;
  }
  const size_t active_size = active.size();
  active.close();

  if (active_size <= LOG_ROTATE_MAX_BYTES) {
    return;
  }

  if (filesystem_has_file(LOG_ARCHIVE_FILE_PATH)) {
    LittleFS.remove(LOG_ARCHIVE_FILE_PATH);
  }
  LittleFS.rename(LOG_ACTIVE_FILE_PATH, LOG_ARCHIVE_FILE_PATH);
}
