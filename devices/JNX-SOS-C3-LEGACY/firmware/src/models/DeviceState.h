#pragma once

#include <Arduino.h>

enum class SirenState : uint8_t {
  OFF = 0,
  PLAYING = 1,
  RESTING = 2,
};

inline const char* sirenStateToString(SirenState state) {
  switch (state) {
    case SirenState::PLAYING:
      return "PLAYING";
    case SirenState::RESTING:
      return "RESTING";
    case SirenState::OFF:
    default:
      return "OFF";
  }
}

struct DeviceState {
  SirenState sirenState = SirenState::OFF;
  uint8_t selectedProfileId = 2;
  String selectedProfileName;
  String activeProfileName;
  String wifiMode;
  String ipAddress;
  String apIpAddress;
  String connectedSsid;
  uint16_t activeFrequencyHz = 0;
  uint8_t activeDutyPercent = 0;
  uint32_t remainingMs = 0;
  uint32_t uptimeSec = 0;
  bool buttonPressed = false;
  bool vtTriggerHigh = false;
  bool commandActive = false;
  bool testMode = false;
  bool sosActive = false;
  bool staConnected = false;
  uint16_t sosPressCount = 0;
  uint16_t sosDurationSec = 0;
  uint8_t sosTriggerProfileId = 9;
  String sosRetriggerMode;
  bool sosCloudNotify = false;
};
