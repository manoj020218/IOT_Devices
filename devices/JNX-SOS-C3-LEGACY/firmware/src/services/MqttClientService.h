#pragma once

#include <Arduino.h>

#include "../models/DeviceState.h"

class MqttClientService {
 public:
  void begin();
  void update();
  void publishState(const DeviceState& deviceState);
  void publishButtonEvent(const char* eventName);
  void publishSosEvent(const char* eventName, uint16_t pressCount, uint16_t durationSec,
                       uint8_t profileId, const char* retriggerMode);
  bool enabled() const;
};
