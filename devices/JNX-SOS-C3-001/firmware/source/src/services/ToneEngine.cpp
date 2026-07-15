#include "ToneEngine.h"

#include <driver/gpio.h>
#include <driver/ledc.h>

#include "SpeakerProfiles.h"
#include "ToneProfiles.h"

namespace {
constexpr ledc_mode_t kLedcMode = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_t kLedcTimer = LEDC_TIMER_0;
constexpr ledc_channel_t kLedcChannel = LEDC_CHANNEL_0;
constexpr ledc_timer_bit_t kLedcResolution = LEDC_TIMER_10_BIT;
constexpr uint32_t kDutyMax = (1U << 10U) - 1U;
constexpr uint16_t kMinToneHz = 300;
constexpr uint16_t kMaxToneHz = 2000;
constexpr uint16_t kBenchHoldMs = 1000;
constexpr uint16_t kBenchGapMs = 500;
constexpr uint16_t kBenchToneMs = 1000;
constexpr uint16_t kMinProfileTestMs = 15000;
constexpr uint16_t kMaxProfileTestMs = 30000;
constexpr uint16_t kMaxFixedTestMs = 3000;
constexpr uint32_t kShortSosMs = 200;
constexpr uint32_t kLongSosMs = 600;
constexpr uint32_t kSosGapMs = 200;
constexpr uint32_t kSosPauseMs = 800;
}  // namespace

void ToneEngine::begin(gpio_num_t pwmGpio) {
  pwmGpio_ = pwmGpio;
  pinMode(static_cast<uint8_t>(pwmGpio_), OUTPUT);
  digitalWrite(static_cast<uint8_t>(pwmGpio_), LOW);
  gpio_pulldown_en(pwmGpio_);
  configurePwm_();
  finish_(millis(), "BOOT");
  Serial.printf("[SIREN] GPIO=%u\n", static_cast<unsigned>(pwmGpio_));
  Serial.println("[SIREN] output initialized LOW");
}

void ToneEngine::update(uint32_t nowMs, const DeviceSettings& settings) {
  if (state_ == SirenState::COOLING_PAUSE) {
    if (nowMs >= coolingEndsMs_) {
      const bool resumableCommand = timedCommand_ || (commandActive_ && settings.longRunMode);
      if (resumableCommand) {
        if (!timedCommand_ || nowMs < timedEndsMs_) {
          burstCycleCount_ = 0;
          burstEndsMs_ = nowMs + settings.burstOnMs;
          activeEndsMs_ = nowMs + static_cast<uint32_t>(settings.onDurationSec) * 1000UL;
          if (timedCommand_) {
            activeEndsMs_ = min<uint32_t>(activeEndsMs_, timedEndsMs_);
          }
          enterState_(SirenState::STARTING, nowMs);
          Serial.println("[SIREN] resuming after cooling pause");
        } else {
          finish_(nowMs, "TIMED_COMPLETE");
        }
      } else {
        finish_(nowMs, nullptr);
      }
    }
    return;
  }

  if (mode_ == Mode::BENCH_TEST) {
    updateBench_(nowMs);
    return;
  }

  if (!isActive() || activeProfile_ == nullptr) {
    stopOutput_();
    return;
  }

  if (nowMs >= activeEndsMs_) {
    if (testMode_) {
      finish_(nowMs, "TEST_COMPLETE");
    } else if (timedCommand_ && nowMs >= timedEndsMs_) {
      finish_(nowMs, "TIMED_COMPLETE");
    } else {
      startCooling_(nowMs, settings,
                    timedCommand_ ? "MANDATORY_PAUSE" : "MAX_DURATION");
    }
    return;
  }

  if (timedCommand_ && nowMs >= timedEndsMs_) {
    finish_(nowMs, "TIMED_COMPLETE");
    return;
  }

  if (!settings.longRunMode && state_ == SirenState::BURST_PAUSE) {
    if (nowMs < burstPauseEndsMs_) {
      stopOutput_();
      return;
    }
    if (settings.burstCycleLimit != 0 && burstCycleCount_ >= settings.burstCycleLimit) {
      startCooling_(nowMs, settings, "BURST_LIMIT");
      return;
    }
    enterState_(SirenState::STARTING, nowMs);
    burstEndsMs_ = nowMs + settings.burstOnMs;
  }

  if (!testMode_ && !settings.longRunMode && settings.burstOnMs > 0 &&
      settings.burstOffMs > 0 && nowMs >= burstEndsMs_) {
    ++burstCycleCount_;
    enterState_(SirenState::BURST_PAUSE, nowMs);
    burstPauseEndsMs_ = nowMs + settings.burstOffMs;
    stopOutput_();
    return;
  }

  const uint16_t frequencyHz = selectFrequency_(nowMs, settings);
  if (frequencyHz == 0) {
    stopOutput_();
    return;
  }
  applyTone_(frequencyHz);
}

bool ToneEngine::startCommand(uint8_t profileId, const DeviceSettings& settings, bool sosOverride) {
  return start_(profileId, settings, sosOverride, 0, Mode::COMMAND);
}

bool ToneEngine::startCommandForDuration(uint8_t profileId, const DeviceSettings& settings,
                                         uint16_t durationSec, bool sosOverride) {
  return start_(profileId, settings, sosOverride, durationSec, Mode::TIMED_COMMAND);
}

bool ToneEngine::startTest(uint8_t profileId, const DeviceSettings& settings, uint16_t durationSec) {
  return start_(profileId, settings, false, constrain(durationSec, static_cast<uint16_t>(15),
                                                       static_cast<uint16_t>(30)),
                Mode::PROFILE_TEST);
}

bool ToneEngine::startFixedTone(uint16_t frequencyHz, const DeviceSettings& settings,
                                uint16_t durationMs) {
  if (frequencyHz < kMinToneHz || frequencyHz > kMaxToneHz) {
    lastStopReason_ = "INVALID_FREQUENCY";
    enterState_(SirenState::FAULT, millis());
    return false;
  }
  forcedFixedFrequencyHz_ = frequencyHz;
  fixedTestDurationMs_ = min<uint16_t>(durationMs, kMaxFixedTestMs);
  return start_(3, settings, false, max<uint16_t>(1, fixedTestDurationMs_ / 1000U),
                Mode::FIXED_TONE_TEST);
}

bool ToneEngine::startBenchTest(const DeviceSettings& settings) {
  if (!validateStart_(3, settings)) {
    return false;
  }
  activeProfile_ = ToneProfiles::findById(3);
  activeSpeaker_ = SpeakerProfiles::findById(settings.speakerProfileId);
  activeProfileId_ = activeProfile_->id;
  mode_ = Mode::BENCH_TEST;
  testMode_ = true;
  commandActive_ = false;
  timedCommand_ = false;
  sosOverride_ = false;
  burstCycleCount_ = 0;
  commandStartedMs_ = millis();
  enterState_(SirenState::TEST, commandStartedMs_);
  benchStage_ = BenchStage::LOW_HOLD;
  lastStopReason_ = "TEST_START";
  stopOutput_();
  Serial.println("[SIREN TEST] Starting");
  return true;
}

bool ToneEngine::restartTimedCommand(uint16_t durationSec) {
  if (!timedCommand_ || !isActive()) {
    return false;
  }
  timedEndsMs_ = millis() + static_cast<uint32_t>(durationSec) * 1000UL;
  activeEndsMs_ = min<uint32_t>(activeEndsMs_, timedEndsMs_);
  return true;
}

bool ToneEngine::extendTimedCommand(uint16_t durationSec) {
  if (!timedCommand_ || !isActive()) {
    return false;
  }
  const uint32_t baseMs = max<uint32_t>(timedEndsMs_, millis());
  timedEndsMs_ = baseMs + static_cast<uint32_t>(durationSec) * 1000UL;
  return true;
}

void ToneEngine::stop() { stopWithReason("USER_CANCEL"); }

void ToneEngine::stopWithReason(const char* reason) { finish_(millis(), reason); }

void ToneEngine::toggleSelected(const DeviceSettings& settings) {
  if (isActive()) {
    stop();
    return;
  }
  startCommand(settings.selectedProfileId, settings, false);
}

SirenState ToneEngine::state() const { return state_; }

bool ToneEngine::isActive() const {
  return state_ != SirenState::IDLE && state_ != SirenState::FAULT;
}

bool ToneEngine::isTestMode() const { return testMode_; }

bool ToneEngine::isSosOverride() const { return sosOverride_; }

bool ToneEngine::commandActive() const { return commandActive_; }

bool ToneEngine::timedCommandActive() const { return timedCommand_; }

bool ToneEngine::outputActive() const { return currentFrequencyHz_ != 0; }

uint32_t ToneEngine::remainingMs(uint32_t nowMs, const DeviceSettings&) const {
  if (timedCommand_) {
    return timedEndsMs_ > nowMs ? timedEndsMs_ - nowMs : 0;
  }
  return activeEndsMs_ > nowMs ? activeEndsMs_ - nowMs : 0;
}

uint32_t ToneEngine::elapsedOnMs(uint32_t nowMs) const {
  return commandStartedMs_ == 0 ? 0 : nowMs - commandStartedMs_;
}

uint32_t ToneEngine::coolingRemainingMs(uint32_t nowMs) const {
  return coolingEndsMs_ > nowMs ? coolingEndsMs_ - nowMs : 0;
}

uint8_t ToneEngine::activeDutyPercent() const { return currentDutyPercent_; }

uint16_t ToneEngine::activeFrequencyHz() const { return currentFrequencyHz_; }

uint8_t ToneEngine::activeProfileId() const { return activeProfileId_; }

const ToneProfile* ToneEngine::activeProfile() const { return activeProfile_; }

const SpeakerProfile* ToneEngine::activeSpeakerProfile() const { return activeSpeaker_; }

const char* ToneEngine::lastStopReason() const { return lastStopReason_; }

bool ToneEngine::start_(uint8_t profileId, const DeviceSettings& settings, bool sosOverride,
                        uint16_t durationSec, Mode mode) {
  if (state_ == SirenState::COOLING_PAUSE && millis() < coolingEndsMs_) {
    lastStopReason_ = "COOLING_ACTIVE";
    return false;
  }
  if (!validateStart_(profileId, settings)) {
    return false;
  }

  activeProfile_ = ToneProfiles::findById(profileId);
  activeSpeaker_ = SpeakerProfiles::findById(settings.speakerProfileId);
  activeProfileId_ = profileId;
  mode_ = mode;
  testMode_ = mode == Mode::PROFILE_TEST || mode == Mode::FIXED_TONE_TEST;
  commandActive_ = mode == Mode::COMMAND || mode == Mode::TIMED_COMMAND;
  timedCommand_ = mode == Mode::TIMED_COMMAND;
  sosOverride_ = sosOverride;
  burstCycleCount_ = 0;
  commandStartedMs_ = millis();
  if (testMode_) {
    const uint32_t requestedMs = static_cast<uint32_t>(durationSec) * 1000UL;
    const uint16_t minMs = mode == Mode::PROFILE_TEST ? kMinProfileTestMs : 1000;
    const uint16_t maxMs = mode == Mode::PROFILE_TEST ? kMaxProfileTestMs : kMaxFixedTestMs;
    fixedTestDurationMs_ =
        static_cast<uint16_t>(min<uint32_t>(max<uint32_t>(requestedMs, minMs), maxMs));
    activeEndsMs_ = commandStartedMs_ + fixedTestDurationMs_;
  } else {
    fixedTestDurationMs_ = 0;
    activeEndsMs_ = commandStartedMs_ + static_cast<uint32_t>(settings.onDurationSec) * 1000UL;
  }
  timedEndsMs_ = timedCommand_ ? commandStartedMs_ + static_cast<uint32_t>(durationSec) * 1000UL
                               : activeEndsMs_;
  if (timedCommand_) {
    activeEndsMs_ = min<uint32_t>(activeEndsMs_, timedEndsMs_);
  }
  burstEndsMs_ = commandStartedMs_ + settings.burstOnMs;
  burstPauseEndsMs_ = 0;
  forcedFixedFrequencyHz_ = mode == Mode::FIXED_TONE_TEST ? forcedFixedFrequencyHz_ : 0;
  enterState_(SirenState::STARTING, commandStartedMs_);
  lastStopReason_ = "RUNNING";
  Serial.printf("[SIREN] Started speaker=%s tone=%s pattern=%s maxOn=%us\n",
                SpeakerProfiles::toString(settings.speakerProfileId), activeProfile_->name,
                activeProfile_->frequencyPattern,
                static_cast<unsigned>(settings.onDurationSec));
  return true;
}

bool ToneEngine::validateStart_(uint8_t profileId, const DeviceSettings& settings) {
  if (!pwmReady_ || ToneProfiles::findById(profileId) == nullptr) {
    lastStopReason_ = "PROFILE_NOT_FOUND";
    enterState_(SirenState::FAULT, millis());
    return false;
  }
  if (settings.sweepMinHz < kMinToneHz || settings.sweepMaxHz > kMaxToneHz ||
      settings.sweepMinHz >= settings.sweepMaxHz || settings.sweepStepMs < 5 ||
      settings.sweepStepMs > 100 || settings.onDurationSec > 30 || settings.restDurationSec < 5) {
    lastStopReason_ = "INVALID_CONFIG";
    enterState_(SirenState::FAULT, millis());
    Serial.println("[SIREN] invalid configuration rejected");
    return false;
  }
  return true;
}

void ToneEngine::configurePwm_() {
  ledc_timer_config_t timerConfig = {};
  timerConfig.speed_mode = kLedcMode;
  timerConfig.timer_num = kLedcTimer;
  timerConfig.duty_resolution = kLedcResolution;
  timerConfig.freq_hz = 1000;
  timerConfig.clk_cfg = LEDC_AUTO_CLK;
  pwmReady_ = ledc_timer_config(&timerConfig) == ESP_OK;

  ledc_channel_config_t channelConfig = {};
  channelConfig.gpio_num = static_cast<int>(pwmGpio_);
  channelConfig.speed_mode = kLedcMode;
  channelConfig.channel = kLedcChannel;
  channelConfig.intr_type = LEDC_INTR_DISABLE;
  channelConfig.timer_sel = kLedcTimer;
  channelConfig.duty = 0;
  channelConfig.hpoint = 0;
  pwmReady_ = pwmReady_ && ledc_channel_config(&channelConfig) == ESP_OK;
  if (!pwmReady_) {
    lastStopReason_ = "PWM_INIT_FAILED";
    enterState_(SirenState::FAULT, millis());
    Serial.println("[SIREN] GPIO or timer initialization failure");
  }
}

void ToneEngine::stopOutput_() {
  currentDutyPercent_ = 0;
  currentFrequencyHz_ = 0;
  if (pwmReady_) {
    ledc_stop(kLedcMode, kLedcChannel, 0);
  }
  digitalWrite(static_cast<uint8_t>(pwmGpio_), LOW);
}

void ToneEngine::applyTone_(uint16_t frequencyHz) {
  currentFrequencyHz_ = constrain(frequencyHz, kMinToneHz, kMaxToneHz);
  currentDutyPercent_ = 50;
  if (!pwmReady_) {
    return;
  }
  ledc_set_freq(kLedcMode, kLedcTimer, currentFrequencyHz_);
  ledc_set_duty(kLedcMode, kLedcChannel, kDutyMax / 2U);
  ledc_update_duty(kLedcMode, kLedcChannel);
}

void ToneEngine::enterState_(SirenState nextState, uint32_t nowMs) {
  state_ = nextState;
  stateStartedMs_ = nowMs;
}

void ToneEngine::startCooling_(uint32_t nowMs, const DeviceSettings& settings, const char* reason) {
  stopOutput_();
  if (!timedCommand_ && !settings.longRunMode) {
    commandActive_ = false;
    sosOverride_ = false;
  }
  coolingEndsMs_ = nowMs + static_cast<uint32_t>(settings.restDurationSec) * 1000UL;
  enterState_(SirenState::COOLING_PAUSE, nowMs);
  lastStopReason_ = reason;
  Serial.printf("[SIREN] cooling pause started reason=%s remaining=%us\n", reason,
                static_cast<unsigned>(settings.restDurationSec));
}

void ToneEngine::finish_(uint32_t nowMs, const char* reason) {
  stopOutput_();
  activeProfile_ = nullptr;
  activeSpeaker_ = nullptr;
  activeProfileId_ = 0;
  commandStartedMs_ = 0;
  commandActive_ = false;
  testMode_ = false;
  sosOverride_ = false;
  timedCommand_ = false;
  mode_ = Mode::NONE;
  fixedTestDurationMs_ = 0;
  forcedFixedFrequencyHz_ = 0;
  burstCycleCount_ = 0;
  activeEndsMs_ = 0;
  timedEndsMs_ = 0;
  coolingEndsMs_ = 0;
  burstEndsMs_ = 0;
  burstPauseEndsMs_ = 0;
  benchStage_ = BenchStage::COMPLETE;
  enterState_(state_ == SirenState::FAULT ? SirenState::FAULT : SirenState::IDLE, nowMs);
  if (reason != nullptr) {
    lastStopReason_ = reason;
    Serial.printf("[SIREN] Stopped reason=%s\n", reason);
  }
}

void ToneEngine::updateBench_(uint32_t nowMs) {
  const uint32_t elapsed = nowMs - stateStartedMs_;
  switch (benchStage_) {
    case BenchStage::LOW_HOLD:
      stopOutput_();
      if (elapsed >= kBenchHoldMs) {
        benchStage_ = BenchStage::TONE_500;
        enterState_(SirenState::TEST, nowMs);
        Serial.println("[SIREN TEST] 500Hz");
      }
      break;
    case BenchStage::TONE_500:
      applyTone_(500);
      if (elapsed >= kBenchToneMs) {
        benchStage_ = BenchStage::GAP_1;
        enterState_(SirenState::TEST, nowMs);
      }
      break;
    case BenchStage::GAP_1:
      stopOutput_();
      if (elapsed >= kBenchGapMs) {
        benchStage_ = BenchStage::TONE_1000;
        enterState_(SirenState::TEST, nowMs);
        Serial.println("[SIREN TEST] 1000Hz");
      }
      break;
    case BenchStage::TONE_1000:
      applyTone_(1000);
      if (elapsed >= kBenchToneMs) {
        benchStage_ = BenchStage::GAP_2;
        enterState_(SirenState::TEST, nowMs);
      }
      break;
    case BenchStage::GAP_2:
      stopOutput_();
      if (elapsed >= kBenchGapMs) {
        benchStage_ = BenchStage::TONE_1500;
        enterState_(SirenState::TEST, nowMs);
        Serial.println("[SIREN TEST] 1500Hz");
      }
      break;
    case BenchStage::TONE_1500:
      applyTone_(1500);
      if (elapsed >= kBenchToneMs) {
        Serial.println("[SIREN TEST] Complete");
        finish_(nowMs, "TEST_COMPLETE");
      }
      break;
    case BenchStage::COMPLETE:
    default:
      finish_(nowMs, "TEST_COMPLETE");
      break;
  }
}

uint16_t ToneEngine::selectFrequency_(uint32_t nowMs, const DeviceSettings& settings) {
  static_cast<void>(settings);
  const uint32_t elapsedMs = nowMs - commandStartedMs_;
  if (mode_ == Mode::FIXED_TONE_TEST) {
    enterState_(SirenState::TEST, nowMs);
    return forcedFixedFrequencyHz_;
  }
  switch (activeProfile_->mode) {
    case TonePatternMode::CONSTANT:
      enterState_(testMode_ ? SirenState::TEST : SirenState::STARTING, nowMs);
      return activeProfile_->freqAHz;
    case TonePatternMode::ALTERNATE: {
      const uint16_t intervalMs = max<uint16_t>(activeProfile_->intervalMs, 50);
      const bool firstTone = ((elapsedMs / intervalMs) % 2U) == 0U;
      enterState_(firstTone ? SirenState::SWEEP_UP : SirenState::SWEEP_DOWN, nowMs);
      return firstTone ? activeProfile_->freqAHz : activeProfile_->freqBHz;
    }
    case TonePatternMode::PULSE: {
      const uint32_t cycleMs =
          static_cast<uint32_t>(activeProfile_->onMs) + activeProfile_->offMs;
      if (cycleMs == 0) {
        enterState_(SirenState::STARTING, nowMs);
        return activeProfile_->freqAHz;
      }
      const bool enabled = (elapsedMs % cycleMs) < activeProfile_->onMs;
      enterState_(enabled ? SirenState::STARTING : SirenState::STOPPING, nowMs);
      return enabled ? activeProfile_->freqAHz : 0;
    }
    case TonePatternMode::SOS: {
      constexpr uint32_t kCycleMs =
          (kShortSosMs * 3U) + (kLongSosMs * 3U) + (kSosGapMs * 8U) + kSosPauseMs;
      const uint32_t phase = elapsedMs % kCycleMs;
      const uint32_t shortBlock = (kShortSosMs + kSosGapMs) * 3U;
      const uint32_t longBlock = (kLongSosMs + kSosGapMs) * 3U;
      uint32_t local = 0;
      uint32_t onMs = 0;

      if (phase < shortBlock) {
        local = phase;
        onMs = kShortSosMs;
      } else if (phase < shortBlock + longBlock) {
        local = phase - shortBlock;
        onMs = kLongSosMs;
      } else if (phase < shortBlock + longBlock + shortBlock) {
        local = phase - shortBlock - longBlock;
        onMs = kShortSosMs;
      } else {
        enterState_(SirenState::STOPPING, nowMs);
        return 0;
      }

      const bool enabled = (local % (onMs + kSosGapMs)) < onMs;
      enterState_(enabled ? SirenState::STARTING : SirenState::STOPPING, nowMs);
      return enabled ? activeProfile_->freqAHz : 0;
    }
    case TonePatternMode::SWEEP:
    default:
      return sweepFrequency_(nowMs, settings);
  }
}

uint16_t ToneEngine::sweepFrequency_(uint32_t nowMs, const DeviceSettings& settings) {
  static_cast<void>(settings);
  const uint16_t minHz = activeProfile_->freqAHz;
  const uint16_t maxHz = activeProfile_->freqBHz;
  const uint32_t cycleMs = max<uint16_t>(activeProfile_->intervalMs, 100);
  const uint32_t phaseMs = (nowMs - commandStartedMs_) % cycleMs;
  const bool rising = phaseMs < (cycleMs / 2U);
  enterState_(rising ? SirenState::SWEEP_UP : SirenState::SWEEP_DOWN, nowMs);
  const float phase = static_cast<float>(phaseMs) / static_cast<float>(cycleMs);
  const float triangle = phase < 0.5f ? phase * 2.0f : (1.0f - phase) * 2.0f;
  const float span = static_cast<float>(maxHz - minHz);
  return static_cast<uint16_t>(minHz + (span * triangle));
}
