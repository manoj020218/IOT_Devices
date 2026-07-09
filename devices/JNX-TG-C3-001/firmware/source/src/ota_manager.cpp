#include "ota_manager.h"

#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>

#include <cstring>
#include <strings.h>

#include "logger.h"

namespace {

struct OtaState {
  bool busy;
  bool reboot_requested;
  bool local_update_active;
  bool cloud_update_pending;
  char phase[24];
  char message[128];
  OtaRequest request;
} g_ota = {};

enum ChecksumMode : uint8_t {
  CHECKSUM_NONE = 0,
  CHECKSUM_MD5 = 1,
  CHECKSUM_SHA256 = 2,
};

bool parse_semver(const char *version, int &major, int &minor, int &patch) {
  const char *cursor = strrchr(version, 'v');
  cursor = (cursor == nullptr) ? version : cursor + 1;
  return sscanf(cursor, "%d.%d.%d", &major, &minor, &patch) == 3;
}

bool is_newer_version(const char *candidate) {
  int cur_major = 0;
  int cur_minor = 0;
  int cur_patch = 0;
  int new_major = 0;
  int new_minor = 0;
  int new_patch = 0;
  if (!parse_semver(PRODUCT_FW_VERSION, cur_major, cur_minor, cur_patch)) {
    return strcmp(candidate, PRODUCT_FW_VERSION) != 0;
  }
  if (!parse_semver(candidate, new_major, new_minor, new_patch)) {
    return strcmp(candidate, PRODUCT_FW_VERSION) != 0;
  }

  if (new_major != cur_major) {
    return new_major > cur_major;
  }
  if (new_minor != cur_minor) {
    return new_minor > cur_minor;
  }
  return new_patch > cur_patch;
}

void set_phase(const char *phase, const char *message) {
  strlcpy(g_ota.phase, phase, sizeof(g_ota.phase));
  strlcpy(g_ota.message, message, sizeof(g_ota.message));
}

void fail_ota(const char *message) {
  set_phase("error", message);
  logger_log("ERROR", "ota", "OTA_FAIL", message);
  g_ota.busy = false;
  g_ota.local_update_active = false;
  g_ota.cloud_update_pending = false;
}

bool is_hex_string(const char *value, size_t expected_len) {
  if (strlen(value) != expected_len) {
    return false;
  }

  for (size_t i = 0; i < expected_len; ++i) {
    const char ch = value[i];
    const bool is_digit = ch >= '0' && ch <= '9';
    const bool is_lower = ch >= 'a' && ch <= 'f';
    const bool is_upper = ch >= 'A' && ch <= 'F';
    if (!is_digit && !is_lower && !is_upper) {
      return false;
    }
  }
  return true;
}

ChecksumMode parse_checksum_mode(const char *checksum, const char **normalized_value) {
  *normalized_value = checksum;
  if (checksum[0] == '\0') {
    return CHECKSUM_NONE;
  }

  if (strncmp(checksum, "md5:", 4) == 0 || strncmp(checksum, "MD5:", 4) == 0) {
    *normalized_value = checksum + 4;
    return is_hex_string(*normalized_value, 32) ? CHECKSUM_MD5 : CHECKSUM_NONE;
  }

  if (strncmp(checksum, "sha256:", 7) == 0 || strncmp(checksum, "SHA256:", 7) == 0) {
    *normalized_value = checksum + 7;
    return is_hex_string(*normalized_value, 64) ? CHECKSUM_SHA256 : CHECKSUM_NONE;
  }

  return is_hex_string(checksum, 32) ? CHECKSUM_MD5 : CHECKSUM_NONE;
}

void bytes_to_hex(const uint8_t *input, size_t input_len, char *output, size_t output_len) {
  static constexpr char HEX_DIGITS[] = "0123456789abcdef";
  if (output_len == 0) {
    return;
  }

  size_t cursor = 0;
  for (size_t i = 0; i < input_len && (cursor + 2) < output_len; ++i) {
    output[cursor++] = HEX_DIGITS[(input[i] >> 4) & 0x0F];
    output[cursor++] = HEX_DIGITS[input[i] & 0x0F];
  }
  output[cursor] = '\0';
}

bool stream_firmware(HTTPClient &http, int expected_size, ChecksumMode checksum_mode,
                     const char *normalized_checksum) {
  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[1024];
  int remaining = expected_size;

  mbedtls_sha256_context sha256_ctx;
  bool sha256_ready = false;
  if (checksum_mode == CHECKSUM_SHA256) {
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts_ret(&sha256_ctx, 0);
    sha256_ready = true;
  }

  while (http.connected() && (remaining > 0 || expected_size < 0)) {
    const size_t available = stream->available();
    if (available == 0) {
      delay(1);
      continue;
    }

    const size_t chunk_len = stream->readBytes(
        reinterpret_cast<char *>(buffer),
        min(sizeof(buffer), expected_size < 0 ? available : static_cast<size_t>(remaining)));
    if (chunk_len == 0) {
      continue;
    }

    if (sha256_ready) {
      mbedtls_sha256_update_ret(&sha256_ctx, buffer, chunk_len);
    }

    const size_t written = Update.write(buffer, chunk_len);
    if (written != chunk_len) {
      if (sha256_ready) {
        mbedtls_sha256_free(&sha256_ctx);
      }
      return false;
    }

    if (expected_size > 0) {
      remaining -= static_cast<int>(chunk_len);
    }
  }

  if (sha256_ready) {
    uint8_t digest[32];
    char digest_hex[65];
    mbedtls_sha256_finish_ret(&sha256_ctx, digest);
    mbedtls_sha256_free(&sha256_ctx);
    bytes_to_hex(digest, sizeof(digest), digest_hex, sizeof(digest_hex));
    if (strcasecmp(digest_hex, normalized_checksum) != 0) {
      return false;
    }
  }

  return expected_size < 0 || remaining == 0;
}

bool run_cloud_update() {
  if (!WiFi.isConnected()) {
    fail_ota("WiFi not connected for cloud OTA");
    return false;
  }
  if (!is_newer_version(g_ota.request.version)) {
    fail_ota("Requested OTA version is not newer");
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, g_ota.request.url)) {
    fail_ota("Failed to start HTTP client");
    return false;
  }

  const char *normalized_checksum = "";
  const ChecksumMode checksum_mode = parse_checksum_mode(g_ota.request.checksum, &normalized_checksum);
  if (g_ota.request.checksum[0] != '\0' && checksum_mode == CHECKSUM_NONE) {
    http.end();
    fail_ota("Checksum must be empty, md5:<32hex>, sha256:<64hex>, or raw 32-char MD5");
    return false;
  }

  set_phase("download", "Downloading cloud firmware");
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    http.end();
    fail_ota("Cloud OTA HTTP GET failed");
    return false;
  }

  const int size = http.getSize();
  if (!Update.begin(size > 0 ? static_cast<size_t>(size) : UPDATE_SIZE_UNKNOWN)) {
    http.end();
    fail_ota("Update begin failed");
    return false;
  }
  if (checksum_mode == CHECKSUM_MD5 && !Update.setMD5(normalized_checksum)) {
    http.end();
    Update.abort();
    fail_ota("Failed to apply MD5 checksum guard");
    return false;
  }

  const bool streamed = stream_firmware(http, size, checksum_mode, normalized_checksum);
  http.end();
  if (!streamed) {
    Update.abort();
    fail_ota(checksum_mode == CHECKSUM_SHA256 ? "Cloud OTA SHA256 verification failed"
                                              : "Cloud OTA write incomplete");
    return false;
  }

  if (!Update.end(true)) {
    fail_ota("Cloud OTA finalize failed");
    return false;
  }

  set_phase("success", "Cloud OTA complete, reboot required");
  logger_log("INFO", "ota", "OTA_SUCCESS", "Cloud OTA completed successfully");
  g_ota.busy = false;
  g_ota.reboot_requested = true;
  return true;
}

}  // namespace

bool ota_manager_init() {
  memset(&g_ota, 0, sizeof(g_ota));
  set_phase("idle", "Idle");
  return true;
}

void ota_manager_mark_app_valid() {
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;
  if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK &&
      ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
    esp_ota_mark_app_valid_cancel_rollback();
  }
}

bool ota_manager_begin_local(size_t size_bytes, char *error, size_t error_size) {
  if (g_ota.busy) {
    strlcpy(error, "OTA already in progress", error_size);
    return false;
  }
  if (!Update.begin(size_bytes > 0 ? size_bytes : UPDATE_SIZE_UNKNOWN)) {
    strlcpy(error, "Failed to initialize OTA", error_size);
    return false;
  }

  g_ota.busy = true;
  g_ota.local_update_active = true;
  set_phase("upload", "Receiving local OTA image");
  logger_log("INFO", "ota", "OTA_START", "Local OTA upload started");
  return true;
}

bool ota_manager_write_local(const uint8_t *data, size_t data_len, char *error, size_t error_size) {
  if (!g_ota.local_update_active) {
    strlcpy(error, "Local OTA not active", error_size);
    return false;
  }

  const size_t written = Update.write(const_cast<uint8_t *>(data), data_len);
  if (written != data_len) {
    Update.abort();
    fail_ota("Local OTA write failed");
    strlcpy(error, "Local OTA write failed", error_size);
    return false;
  }
  return true;
}

bool ota_manager_finalize_local(char *error, size_t error_size) {
  if (!g_ota.local_update_active) {
    strlcpy(error, "Local OTA not active", error_size);
    return false;
  }
  if (!Update.end(true)) {
    fail_ota("Local OTA finalize failed");
    strlcpy(error, "Local OTA finalize failed", error_size);
    return false;
  }

  g_ota.busy = false;
  g_ota.local_update_active = false;
  g_ota.reboot_requested = true;
  set_phase("success", "Local OTA complete, reboot required");
  logger_log("INFO", "ota", "OTA_SUCCESS", "Local OTA completed successfully");
  return true;
}

void ota_manager_abort_local(const char *reason) {
  Update.abort();
  fail_ota(reason);
}

bool ota_manager_request_cloud_update(const OtaRequest &request, char *error, size_t error_size) {
  if (g_ota.busy) {
    strlcpy(error, "OTA already in progress", error_size);
    return false;
  }
  if (request.url[0] == '\0' || request.version[0] == '\0') {
    strlcpy(error, "OTA URL and version are required", error_size);
    return false;
  }
  const char *normalized_checksum = "";
  if (request.checksum[0] != '\0' &&
      parse_checksum_mode(request.checksum, &normalized_checksum) == CHECKSUM_NONE) {
    strlcpy(error, "Checksum must be empty, md5:<32hex>, sha256:<64hex>, or raw 32-char MD5",
            error_size);
    return false;
  }

  g_ota.request = request;
  g_ota.busy = true;
  g_ota.cloud_update_pending = true;
  set_phase("queued", "Cloud OTA queued");
  logger_log("INFO", "ota", "OTA_CHECK", "Cloud OTA request queued");
  error[0] = '\0';
  return true;
}

void ota_manager_update(uint32_t now_ms) {
  if (g_ota.cloud_update_pending) {
    g_ota.cloud_update_pending = false;
    run_cloud_update();
  }
}

bool ota_manager_is_busy() {
  return g_ota.busy;
}

bool ota_manager_should_reboot() {
  return g_ota.reboot_requested;
}

void ota_manager_clear_reboot_flag() {
  g_ota.reboot_requested = false;
}

const char *ota_manager_phase() {
  return g_ota.phase;
}

const char *ota_manager_message() {
  return g_ota.message;
}
