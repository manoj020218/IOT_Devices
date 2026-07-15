#pragma once

#include <Arduino.h>

enum class SirenState : uint8_t {
  IDLE = 0,
  STARTING = 1,
  SWEEP_UP = 2,
  SWEEP_DOWN = 3,
  BURST_PAUSE = 4,
  COOLING_PAUSE = 5,
  STOPPING = 6,
  FAULT = 7,
  TEST = 8,
};

inline const char* sirenStateToString(SirenState state) {
  switch (state) {
    case SirenState::STARTING:
      return "STARTING";
    case SirenState::SWEEP_UP:
      return "SWEEP_UP";
    case SirenState::SWEEP_DOWN:
      return "SWEEP_DOWN";
    case SirenState::BURST_PAUSE:
      return "BURST_PAUSE";
    case SirenState::COOLING_PAUSE:
      return "COOLING_PAUSE";
    case SirenState::STOPPING:
      return "STOPPING";
    case SirenState::FAULT:
      return "FAULT";
    case SirenState::TEST:
      return "TEST";
    case SirenState::IDLE:
    default:
      return "IDLE";
  }
}

struct DeviceState {
  SirenState sirenState = SirenState::IDLE;
  uint8_t selectedProfileId = 1;
  String selectedProfileName;
  String activeProfileName;
  String speakerProfileName;
  String wifiMode;
  String ipAddress;
  String apIpAddress;
  String connectedSsid;
  uint16_t activeFrequencyHz = 0;
  uint8_t activeDutyPercent = 0;
  uint32_t remainingMs = 0;
  uint32_t elapsedOnMs = 0;
  uint32_t coolingRemainingMs = 0;
  uint32_t uptimeSec = 0;
  bool buttonPressed = false;
  bool vtTriggerHigh = false;
  bool vtTriggerSeen = false;
  bool commandActive = false;
  bool testMode = false;
  bool sosActive = false;
  bool staConnected = false;
  bool vtControlLatched = false;
  uint32_t vtLastTriggerUptimeSec = 0;
  uint16_t sosPressCount = 0;
  uint16_t sosDurationSec = 0;
  uint8_t sosTriggerProfileId = 1;
  String vtTriggerMode;
  String sosRetriggerMode;
  bool sosCloudNotify = false;
  String lastStopReason;
};
