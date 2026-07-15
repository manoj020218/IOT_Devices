#pragma once

#include <Arduino.h>

#include "../models/SpeakerProfile.h"

class SpeakerProfiles {
 public:
  static const SpeakerProfile* all();
  static size_t count();
  static const SpeakerProfile* defaultProfile();
  static const SpeakerProfile* findById(SpeakerProfileId id);
  static const char* toString(SpeakerProfileId id);
  static SpeakerProfileId fromString(const String& value);
};
