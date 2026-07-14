#pragma once

#include <Arduino.h>

#include "../models/DeviceState.h"
#include "../models/ToneProfile.h"
#include "SettingsManager.h"

class ToneEngine {
 public:
  void begin(gpio_num_t pwmGpio);
  void update(uint32_t nowMs, const DeviceSettings& settings);
  bool startCommand(uint8_t profileId, const DeviceSettings& settings, bool sosOverride = false);
  bool startCommandForDuration(uint8_t profileId, const DeviceSettings& settings,
                               uint16_t durationSec, bool sosOverride = false);
  bool startTest(uint8_t profileId, const DeviceSettings& settings, uint16_t durationSec);
  bool restartTimedCommand(uint16_t durationSec);
  bool extendTimedCommand(uint16_t durationSec);
  void stop();
  void toggleSelected(const DeviceSettings& settings);

  SirenState state() const;
  bool isActive() const;
  bool isTestMode() const;
  bool isSosOverride() const;
  bool commandActive() const;
  bool timedCommandActive() const;
  uint32_t remainingMs(uint32_t nowMs, const DeviceSettings& settings) const;
  uint8_t activeDutyPercent() const;
  uint16_t activeFrequencyHz() const;
  uint8_t activeProfileId() const;
  const ToneProfile* activeProfile() const;

 private:
  struct ToneOutput {
    bool enabled = false;
    uint16_t frequencyHz = 0;
    uint8_t dutyPercent = 0;
  };

  gpio_num_t pwmGpio_ = GPIO_NUM_NC;
  bool pwmReady_ = false;
  bool commandActive_ = false;
  bool testMode_ = false;
  bool sosOverride_ = false;
  bool timedCommand_ = false;
  uint32_t stateStartedMs_ = 0;
  uint32_t testEndsMs_ = 0;
  uint32_t commandEndsMs_ = 0;
  uint8_t activeProfileId_ = 0;
  uint16_t currentFrequencyHz_ = 0;
  uint8_t currentDutyPercent_ = 0;
  const ToneProfile* activeProfile_ = nullptr;
  SirenState state_ = SirenState::OFF;

  void configurePwm_();
  void stopOutput_();
  void applyTone_(uint16_t frequencyHz, uint8_t dutyPercent);
  void enterState_(SirenState nextState, uint32_t nowMs);
  ToneOutput computeOutput_(uint32_t elapsedMs, const DeviceSettings& settings) const;
  uint8_t chooseDuty_(uint32_t playElapsedMs, const DeviceSettings& settings) const;
};
