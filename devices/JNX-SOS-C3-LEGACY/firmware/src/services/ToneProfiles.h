#pragma once

#include <Arduino.h>

#include "../models/ToneProfile.h"

class ToneProfiles {
 public:
  static const ToneProfile* all();
  static size_t count();
  static const ToneProfile* findById(uint8_t id);
  static const ToneProfile* defaultProfile();
};
