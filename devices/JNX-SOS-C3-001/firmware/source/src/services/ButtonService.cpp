#include "ButtonService.h"

namespace {
constexpr uint32_t kDebounceMs = 40;
constexpr uint32_t kLongPressMs = 3000;
constexpr uint32_t kVeryLongPressMs = 10000;
}  // namespace

void ButtonService::begin(gpio_num_t gpio) {
  gpio_ = gpio;
  pinMode(static_cast<uint8_t>(gpio_), INPUT_PULLUP);
  rawPressed_ = digitalRead(static_cast<uint8_t>(gpio_)) == LOW;
  stablePressed_ = rawPressed_;
}

void ButtonService::update(uint32_t nowMs) {
  const bool rawNow = digitalRead(static_cast<uint8_t>(gpio_)) == LOW;
  if (rawNow != rawPressed_) {
    rawPressed_ = rawNow;
    debounceStartedMs_ = nowMs;
  }

  if (rawNow != stablePressed_ && nowMs - debounceStartedMs_ >= kDebounceMs) {
    stablePressed_ = rawNow;
    if (stablePressed_) {
      pressStartedMs_ = nowMs;
      veryLongFired_ = false;
    } else if (!veryLongFired_) {
      const uint32_t pressDurationMs = nowMs - pressStartedMs_;
      pendingEvent_ = pressDurationMs >= kLongPressMs ? ButtonEvent::LONG_PRESS
                                                      : ButtonEvent::SHORT_PRESS;
    }
  }

  if (stablePressed_ && !veryLongFired_ &&
      nowMs - pressStartedMs_ >= kVeryLongPressMs) {
    pendingEvent_ = ButtonEvent::VERY_LONG_PRESS;
    veryLongFired_ = true;
  }
}

ButtonEvent ButtonService::consumeEvent() {
  const ButtonEvent event = pendingEvent_;
  pendingEvent_ = ButtonEvent::NONE;
  return event;
}

bool ButtonService::isPressed() const { return stablePressed_; }
