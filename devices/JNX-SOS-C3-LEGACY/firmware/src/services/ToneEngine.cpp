#include "ToneEngine.h"

#include <driver/ledc.h>

#include "ToneProfiles.h"

namespace {
constexpr ledc_mode_t kLedcMode = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_t kLedcTimer = LEDC_TIMER_0;
constexpr ledc_channel_t kLedcChannel = LEDC_CHANNEL_0;
constexpr ledc_timer_bit_t kLedcResolution = LEDC_TIMER_10_BIT;
constexpr uint32_t kDutyMax = (1U << 10U) - 1U;
constexpr uint16_t kMinToneHz = 100;
constexpr uint32_t kShortSosMs = 200;
constexpr uint32_t kLongSosMs = 600;
constexpr uint32_t kSosGapMs = 200;
constexpr uint32_t kSosPauseMs = 800;
}  // namespace

void ToneEngine::begin(gpio_num_t pwmGpio) {
  pwmGpio_ = pwmGpio;
  configurePwm_();
  stop();
}

void ToneEngine::update(uint32_t nowMs, const DeviceSettings& settings) {
  if (activeProfile_ == nullptr) {
    stop();
    return;
  }

  if (timedCommand_ && nowMs >= commandEndsMs_) {
    stop();
    return;
  }

  if (state_ == SirenState::PLAYING) {
    if (testMode_ && nowMs >= testEndsMs_) {
      stop();
      return;
    }

    if (!testMode_ && settings.longRunMode &&
        nowMs - stateStartedMs_ >= static_cast<uint32_t>(settings.onDurationSec) * 1000UL) {
      enterState_(SirenState::RESTING, nowMs);
      stopOutput_();
      return;
    }

    const ToneOutput output = computeOutput_(nowMs - stateStartedMs_, settings);
    if (output.enabled) {
      applyTone_(output.frequencyHz, output.dutyPercent);
    } else {
      stopOutput_();
    }
    return;
  }

  if (state_ == SirenState::RESTING) {
    stopOutput_();
    if (!commandActive_) {
      stop();
      return;
    }

    if (nowMs - stateStartedMs_ >= static_cast<uint32_t>(settings.restDurationSec) * 1000UL) {
      enterState_(SirenState::PLAYING, nowMs);
    }
    return;
  }

  stopOutput_();
}

bool ToneEngine::startCommand(uint8_t profileId, const DeviceSettings&, bool sosOverride) {
  activeProfile_ = ToneProfiles::findById(profileId);
  if (activeProfile_ == nullptr) {
    stop();
    return false;
  }

  activeProfileId_ = profileId;
  commandActive_ = true;
  testMode_ = false;
  sosOverride_ = sosOverride;
  timedCommand_ = false;
  commandEndsMs_ = 0;
  enterState_(SirenState::PLAYING, millis());
  return true;
}

bool ToneEngine::startCommandForDuration(uint8_t profileId, const DeviceSettings& settings,
                                         uint16_t durationSec, bool sosOverride) {
  if (!startCommand(profileId, settings, sosOverride)) {
    return false;
  }

  timedCommand_ = true;
  commandEndsMs_ = millis() + static_cast<uint32_t>(durationSec) * 1000UL;
  return true;
}

bool ToneEngine::startTest(uint8_t profileId, const DeviceSettings&, uint16_t durationSec) {
  activeProfile_ = ToneProfiles::findById(profileId);
  if (activeProfile_ == nullptr) {
    stop();
    return false;
  }

  activeProfileId_ = profileId;
  commandActive_ = false;
  testMode_ = true;
  sosOverride_ = false;
  timedCommand_ = false;
  commandEndsMs_ = 0;
  const uint32_t nowMs = millis();
  testEndsMs_ = nowMs + static_cast<uint32_t>(durationSec) * 1000UL;
  enterState_(SirenState::PLAYING, nowMs);
  return true;
}

bool ToneEngine::restartTimedCommand(uint16_t durationSec) {
  if (!timedCommand_ || !isActive()) {
    return false;
  }

  commandEndsMs_ = millis() + static_cast<uint32_t>(durationSec) * 1000UL;
  return true;
}

bool ToneEngine::extendTimedCommand(uint16_t durationSec) {
  if (!timedCommand_ || !isActive()) {
    return false;
  }

  const uint32_t nowMs = millis();
  const uint32_t baseMs = commandEndsMs_ > nowMs ? commandEndsMs_ : nowMs;
  commandEndsMs_ = baseMs + static_cast<uint32_t>(durationSec) * 1000UL;
  return true;
}

void ToneEngine::stop() {
  commandActive_ = false;
  testMode_ = false;
  sosOverride_ = false;
  timedCommand_ = false;
  activeProfile_ = nullptr;
  activeProfileId_ = 0;
  commandEndsMs_ = 0;
  enterState_(SirenState::OFF, millis());
  stopOutput_();
}

void ToneEngine::toggleSelected(const DeviceSettings& settings) {
  if (isActive()) {
    stop();
    return;
  }
  startCommand(settings.selectedProfileId, settings, false);
}

SirenState ToneEngine::state() const { return state_; }

bool ToneEngine::isActive() const {
  return state_ == SirenState::PLAYING || state_ == SirenState::RESTING;
}

bool ToneEngine::isTestMode() const { return testMode_; }

bool ToneEngine::isSosOverride() const { return sosOverride_; }

bool ToneEngine::commandActive() const { return commandActive_; }

bool ToneEngine::timedCommandActive() const { return timedCommand_; }

uint32_t ToneEngine::remainingMs(uint32_t nowMs, const DeviceSettings& settings) const {
  if (timedCommand_ && isActive()) {
    return commandEndsMs_ > nowMs ? commandEndsMs_ - nowMs : 0;
  }

  if (state_ == SirenState::PLAYING) {
    if (testMode_) {
      return testEndsMs_ > nowMs ? testEndsMs_ - nowMs : 0;
    }
    if (!settings.longRunMode) {
      return 0;
    }
    const uint32_t total = static_cast<uint32_t>(settings.onDurationSec) * 1000UL;
    const uint32_t elapsed = nowMs - stateStartedMs_;
    return elapsed >= total ? 0 : total - elapsed;
  }

  if (state_ == SirenState::RESTING) {
    const uint32_t total = static_cast<uint32_t>(settings.restDurationSec) * 1000UL;
    const uint32_t elapsed = nowMs - stateStartedMs_;
    return elapsed >= total ? 0 : total - elapsed;
  }

  return 0;
}

uint8_t ToneEngine::activeDutyPercent() const { return currentDutyPercent_; }

uint16_t ToneEngine::activeFrequencyHz() const { return currentFrequencyHz_; }

uint8_t ToneEngine::activeProfileId() const { return activeProfileId_; }

const ToneProfile* ToneEngine::activeProfile() const { return activeProfile_; }

void ToneEngine::configurePwm_() {
  ledc_timer_config_t timerConfig = {};
  timerConfig.speed_mode = kLedcMode;
  timerConfig.timer_num = kLedcTimer;
  timerConfig.duty_resolution = kLedcResolution;
  timerConfig.freq_hz = 2000;
  timerConfig.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&timerConfig);

  ledc_channel_config_t channelConfig = {};
  channelConfig.gpio_num = static_cast<int>(pwmGpio_);
  channelConfig.speed_mode = kLedcMode;
  channelConfig.channel = kLedcChannel;
  channelConfig.intr_type = LEDC_INTR_DISABLE;
  channelConfig.timer_sel = kLedcTimer;
  channelConfig.duty = 0;
  channelConfig.hpoint = 0;
  ledc_channel_config(&channelConfig);
  pwmReady_ = true;
}

void ToneEngine::stopOutput_() {
  currentDutyPercent_ = 0;
  currentFrequencyHz_ = 0;
  if (pwmReady_) {
    ledc_stop(kLedcMode, kLedcChannel, 0);
  }
}

void ToneEngine::applyTone_(uint16_t frequencyHz, uint8_t dutyPercent) {
  currentFrequencyHz_ = max<uint16_t>(frequencyHz, kMinToneHz);
  currentDutyPercent_ = dutyPercent;
  if (!pwmReady_) {
    return;
  }

  ledc_set_freq(kLedcMode, kLedcTimer, currentFrequencyHz_);
  const uint32_t duty = (kDutyMax * currentDutyPercent_) / 100U;
  ledc_set_duty(kLedcMode, kLedcChannel, duty);
  ledc_update_duty(kLedcMode, kLedcChannel);
}

void ToneEngine::enterState_(SirenState nextState, uint32_t nowMs) {
  state_ = nextState;
  stateStartedMs_ = nowMs;
}

ToneEngine::ToneOutput ToneEngine::computeOutput_(uint32_t elapsedMs,
                                                  const DeviceSettings& settings) const {
  ToneOutput output;
  if (activeProfile_ == nullptr) {
    return output;
  }

  output.enabled = true;
  output.dutyPercent = chooseDuty_(elapsedMs, settings);

  switch (activeProfile_->mode) {
    case TonePatternMode::CONSTANT:
      output.frequencyHz = activeProfile_->freqAHz;
      return output;
    case TonePatternMode::SWEEP: {
      const uint32_t cycle = max<uint16_t>(activeProfile_->intervalMs, 100);
      const float phase = static_cast<float>(elapsedMs % cycle) / static_cast<float>(cycle);
      const float triangle = phase < 0.5f ? phase * 2.0f : (1.0f - phase) * 2.0f;
      output.frequencyHz = activeProfile_->freqAHz +
                           static_cast<uint16_t>((activeProfile_->freqBHz - activeProfile_->freqAHz) *
                                                 triangle);
      return output;
    }
    case TonePatternMode::ALTERNATE: {
      const uint32_t interval = max<uint16_t>(activeProfile_->intervalMs, 50);
      const bool firstTone = ((elapsedMs / interval) % 2U) == 0U;
      output.frequencyHz = firstTone ? activeProfile_->freqAHz : activeProfile_->freqBHz;
      return output;
    }
    case TonePatternMode::PULSE: {
      const uint32_t cycle =
          static_cast<uint32_t>(activeProfile_->onMs) + activeProfile_->offMs;
      const uint32_t phase = cycle == 0 ? 0 : elapsedMs % cycle;
      output.enabled = phase < activeProfile_->onMs;
      output.frequencyHz = activeProfile_->freqAHz;
      return output;
    }
    case TonePatternMode::SOS: {
      constexpr uint32_t kCycleMs =
          (kShortSosMs * 3U) + (kLongSosMs * 3U) + (kSosGapMs * 8U) + kSosPauseMs;
      const uint32_t phase = elapsedMs % kCycleMs;
      const uint32_t shortBlock = (kShortSosMs + kSosGapMs) * 3U;
      const uint32_t longBlock = (kLongSosMs + kSosGapMs) * 3U;
      uint32_t local = phase;
      uint32_t onMs = kShortSosMs;

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
        output.enabled = false;
        return output;
      }

      output.enabled = (local % (onMs + kSosGapMs)) < onMs;
      output.frequencyHz = activeProfile_->freqAHz;
      return output;
    }
    default:
      output.enabled = false;
      return output;
  }
}

uint8_t ToneEngine::chooseDuty_(uint32_t playElapsedMs, const DeviceSettings& settings) const {
  uint8_t duty = playElapsedMs < static_cast<uint32_t>(settings.boostDurationSec) * 1000UL
                     ? settings.boostDutyPercent
                     : settings.normalDutyPercent;

  const uint8_t voltageMax =
      settings.inputVoltageProfile == InputVoltageProfile::V30 ? 60 : 80;
  duty = min<uint8_t>(duty, settings.maxDutyPercent);
  duty = min<uint8_t>(duty, voltageMax);

  if (activeProfile_ != nullptr) {
    duty = min<uint8_t>(duty, activeProfile_->maxSafeDuty);
  }
  return duty;
}
