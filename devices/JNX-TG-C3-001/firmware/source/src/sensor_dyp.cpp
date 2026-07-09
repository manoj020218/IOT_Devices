#include "sensor_dyp.h"

namespace {

HardwareSerial g_sensor_serial(1);
SensorSnapshot g_snapshot = {SENSOR_NO_DATA, 0, 0, 0, false};
uint16_t g_samples[SENSOR_FILTER_SAMPLES] = {};
uint8_t g_sample_count = 0;
uint8_t g_sample_cursor = 0;
bool g_filter_ready = false;

uint16_t sorted_trimmed_average() {
  uint16_t scratch[SENSOR_FILTER_SAMPLES];
  for (uint8_t i = 0; i < SENSOR_FILTER_SAMPLES; ++i) {
    scratch[i] = g_samples[i];
  }

  for (uint8_t i = 0; i < SENSOR_FILTER_SAMPLES; ++i) {
    for (uint8_t j = i + 1; j < SENSOR_FILTER_SAMPLES; ++j) {
      if (scratch[j] < scratch[i]) {
        const uint16_t tmp = scratch[i];
        scratch[i] = scratch[j];
        scratch[j] = tmp;
      }
    }
  }

  uint32_t sum = 0;
  for (uint8_t i = 1; i < SENSOR_FILTER_SAMPLES - 1; ++i) {
    sum += scratch[i];
  }
  return static_cast<uint16_t>(sum / (SENSOR_FILTER_SAMPLES - 2));
}

bool read_frame(uint16_t &distance_mm) {
  while (g_sensor_serial.available() > 0 && g_sensor_serial.peek() != 0xFF) {
    g_sensor_serial.read();
  }

  if (g_sensor_serial.available() < 4) {
    return false;
  }

  uint8_t frame[4] = {};
  const size_t read = g_sensor_serial.readBytes(frame, sizeof(frame));
  if (read != sizeof(frame)) {
    return false;
  }

  if (frame[0] != 0xFF) {
    return false;
  }

  const uint8_t checksum = static_cast<uint8_t>((frame[0] + frame[1] + frame[2]) & 0xFF);
  if (checksum != frame[3]) {
    g_snapshot.status = SENSOR_INVALID;
    return false;
  }

  const uint16_t distance = static_cast<uint16_t>((frame[1] << 8) | frame[2]);
  if (distance < SENSOR_MIN_MM || distance > SENSOR_MAX_MM) {
    g_snapshot.status = SENSOR_INVALID;
    return false;
  }

  distance_mm = distance;
  return true;
}

void push_sample(uint16_t value) {
  if (g_filter_ready && g_snapshot.filtered_distance_mm > 0) {
    const uint16_t base = g_snapshot.filtered_distance_mm;
    const uint16_t delta = (value > base) ? (value - base) : (base - value);
    if (delta > SENSOR_MAX_STEP_MM) {
      return;
    }
  }

  g_samples[g_sample_cursor] = value;
  g_sample_cursor = (g_sample_cursor + 1U) % SENSOR_FILTER_SAMPLES;
  if (g_sample_count < SENSOR_FILTER_SAMPLES) {
    ++g_sample_count;
  }
  g_filter_ready = (g_sample_count >= SENSOR_FILTER_SAMPLES);
}

}  // namespace

bool sensor_dyp_init() {
  g_sensor_serial.begin(SENSOR_BAUD_RATE, SERIAL_8N1, PIN_SENSOR_RX, PIN_SENSOR_TX);
  sensor_dyp_reset();
  return true;
}

void sensor_dyp_reset() {
  memset(g_samples, 0, sizeof(g_samples));
  g_sample_count = 0;
  g_sample_cursor = 0;
  g_filter_ready = false;
  g_snapshot.status = SENSOR_NO_DATA;
  g_snapshot.raw_distance_mm = 0;
  g_snapshot.filtered_distance_mm = 0;
  g_snapshot.last_valid_ms = 0;
  g_snapshot.valid_frame_seen = false;
}

void sensor_dyp_update(uint32_t now_ms) {
  bool any_frame = false;
  uint16_t distance = 0;
  while (read_frame(distance)) {
    any_frame = true;
    g_snapshot.valid_frame_seen = true;
    g_snapshot.raw_distance_mm = distance;
    g_snapshot.last_valid_ms = now_ms;
    push_sample(distance);
  }

  if (g_filter_ready) {
    g_snapshot.filtered_distance_mm = sorted_trimmed_average();
  } else if (any_frame) {
    g_snapshot.filtered_distance_mm = g_snapshot.raw_distance_mm;
  }

  if (g_snapshot.last_valid_ms == 0) {
    g_snapshot.status = SENSOR_NO_DATA;
  } else if ((now_ms - g_snapshot.last_valid_ms) >= SENSOR_FAULT_TIMEOUT_MS) {
    g_snapshot.status = SENSOR_FAULT;
  } else if (g_snapshot.filtered_distance_mm == 0) {
    g_snapshot.status = SENSOR_NO_DATA;
  } else {
    g_snapshot.status = SENSOR_OK;
  }
}

const SensorSnapshot &sensor_dyp_get_snapshot() {
  return g_snapshot;
}

