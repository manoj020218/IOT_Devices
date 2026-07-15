#pragma once

#include <Arduino.h>

enum class TonePatternMode : uint8_t {
  CONSTANT = 0,
  SWEEP = 1,
  ALTERNATE = 2,
  PULSE = 3,
  SOS = 4,
};

struct ToneProfile {
  uint8_t id;
  const char* name;
  const char* description;
  const char* frequencyPattern;
  const char* dutyProfile;
  const char* recommendedUse;
  uint8_t maxSafeDuty;
  bool longDurationAllowed;
  uint8_t testDurationSec;
  TonePatternMode mode;
  uint16_t freqAHz;
  uint16_t freqBHz;
  uint16_t intervalMs;
  uint16_t onMs;
  uint16_t offMs;
};
