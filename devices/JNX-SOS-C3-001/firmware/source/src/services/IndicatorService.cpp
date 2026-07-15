#include "IndicatorService.h"

namespace {
constexpr uint16_t kButtonPulseMs = 200;
}  // namespace

void IndicatorService::begin(gpio_num_t gpio, bool activeLow) {
  gpio_ = gpio;
  activeLow_ = activeLow;
  initialized_ = false;
  if (gpio_ == GPIO_NUM_NC) {
    return;
  }
  pinMode(static_cast<uint8_t>(gpio_), OUTPUT);
  write_(false);
}

void IndicatorService::update(uint32_t nowMs, bool sosPlaying, bool buttonPressed) {
  if (buttonPressed && !previousButtonPressed_) {
    pulseUntilMs_ = nowMs + kButtonPulseMs;
  }
  previousButtonPressed_ = buttonPressed;

  const bool pulseActive = nowMs < pulseUntilMs_;
  write_(sosPlaying || pulseActive);
}

void IndicatorService::write_(bool on) {
  if (gpio_ == GPIO_NUM_NC) {
    return;
  }

  if (initialized_ && on == currentOn_) {
    return;
  }

  initialized_ = true;
  currentOn_ = on;
  const uint8_t level = (activeLow_ ? !on : on) ? HIGH : LOW;
  digitalWrite(static_cast<uint8_t>(gpio_), level);
}
