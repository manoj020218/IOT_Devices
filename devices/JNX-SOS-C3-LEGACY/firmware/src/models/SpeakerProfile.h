#pragma once

#include <Arduino.h>

enum class SpeakerProfileId : uint8_t {
  SUH15_SAFE = 1,
  SUH25 = 2,
};

struct SpeakerProfile {
  SpeakerProfileId id;
  const char* name;
  const char* description;
  uint16_t sweepMinHz;
  uint16_t sweepMaxHz;
  uint16_t sweepStepMs;
  uint16_t maxOnDurationSec;
  uint16_t coolingPauseSec;
  uint16_t burstOnMs;
  uint16_t burstOffMs;
};
