#pragma once

#include <Arduino.h>

#include "../models/DeviceState.h"
#include "../models/SpeakerProfile.h"
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
  bool startFixedTone(uint16_t frequencyHz, const DeviceSettings& settings, uint16_t durationMs);
  bool startBenchTest(const DeviceSettings& settings);
  bool restartTimedCommand(uint16_t durationSec);
  bool extendTimedCommand(uint16_t durationSec);
  void stop();
  void stopWithReason(const char* reason);
  void toggleSelected(const DeviceSettings& settings);

  SirenState state() const;
  bool isActive() const;
  bool isTestMode() const;
  bool isSosOverride() const;
  bool commandActive() const;
  bool timedCommandActive() const;
  bool outputActive() const;
  uint32_t remainingMs(uint32_t nowMs, const DeviceSettings& settings) const;
  uint32_t elapsedOnMs(uint32_t nowMs) const;
  uint32_t coolingRemainingMs(uint32_t nowMs) const;
  uint8_t activeDutyPercent() const;
  uint16_t activeFrequencyHz() const;
  uint8_t activeProfileId() const;
  const ToneProfile* activeProfile() const;
  const SpeakerProfile* activeSpeakerProfile() const;
  const char* lastStopReason() const;

 private:
  enum class Mode : uint8_t {
    NONE = 0,
    COMMAND = 1,
    TIMED_COMMAND = 2,
    PROFILE_TEST = 3,
    FIXED_TONE_TEST = 4,
    BENCH_TEST = 5,
  };

  enum class BenchStage : uint8_t {
    LOW_HOLD = 0,
    TONE_500 = 1,
    GAP_1 = 2,
    TONE_1000 = 3,
    GAP_2 = 4,
    TONE_1500 = 5,
    COMPLETE = 6,
  };

  gpio_num_t pwmGpio_ = GPIO_NUM_NC;
  bool pwmReady_ = false;
  bool commandActive_ = false;
  bool testMode_ = false;
  bool sosOverride_ = false;
  bool timedCommand_ = false;
  uint32_t stateStartedMs_ = 0;
  uint32_t commandStartedMs_ = 0;
  uint32_t activeEndsMs_ = 0;
  uint32_t timedEndsMs_ = 0;
  uint32_t coolingEndsMs_ = 0;
  uint32_t burstEndsMs_ = 0;
  uint32_t burstPauseEndsMs_ = 0;
  uint16_t fixedTestDurationMs_ = 0;
  uint8_t activeProfileId_ = 0;
  uint8_t burstCycleCount_ = 0;
  uint16_t currentFrequencyHz_ = 0;
  uint8_t currentDutyPercent_ = 0;
  uint16_t forcedFixedFrequencyHz_ = 0;
  Mode mode_ = Mode::NONE;
  BenchStage benchStage_ = BenchStage::LOW_HOLD;
  const ToneProfile* activeProfile_ = nullptr;
  const SpeakerProfile* activeSpeaker_ = nullptr;
  SirenState state_ = SirenState::IDLE;
  const char* lastStopReason_ = "BOOT";

  bool start_(uint8_t profileId, const DeviceSettings& settings, bool sosOverride,
              uint16_t durationSec, Mode mode);
  bool validateStart_(uint8_t profileId, const DeviceSettings& settings);
  void configurePwm_();
  void stopOutput_();
  void applyTone_(uint16_t frequencyHz);
  void enterState_(SirenState nextState, uint32_t nowMs);
  void startCooling_(uint32_t nowMs, const DeviceSettings& settings, const char* reason);
  void finish_(uint32_t nowMs, const char* reason);
  void updateBench_(uint32_t nowMs);
  uint16_t selectFrequency_(uint32_t nowMs, const DeviceSettings& settings);
  uint16_t sweepFrequency_(uint32_t nowMs, const DeviceSettings& settings);
};
