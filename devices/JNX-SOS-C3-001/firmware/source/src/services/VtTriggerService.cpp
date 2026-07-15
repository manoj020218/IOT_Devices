#include "VtTriggerService.h"

namespace {
constexpr uint32_t kTriggerLockoutMs = 250;
}  // namespace

void VtTriggerService::begin(gpio_num_t gpio) {
  gpio_ = gpio;
  pinMode(static_cast<uint8_t>(gpio_), INPUT_PULLDOWN);
  rawHigh_ = digitalRead(static_cast<uint8_t>(gpio_)) == HIGH;
  stableHigh_ = rawHigh_;
}

void VtTriggerService::update(uint32_t nowMs) {
  const bool rawNow = digitalRead(static_cast<uint8_t>(gpio_)) == HIGH;
  if (rawNow == rawHigh_) {
    return;
  }

  rawHigh_ = rawNow;
  stableHigh_ = rawNow;
  lastEdgeMs_ = nowMs;
  if (stableHigh_ &&
      (lastTriggerMs_ == 0 || nowMs - lastTriggerMs_ >= kTriggerLockoutMs)) {
    pendingRisingEdge_ = true;
    lastTriggerMs_ = nowMs;
    return;
  }
  pendingFallingEdge_ = !stableHigh_;
}

bool VtTriggerService::consumeRisingEdge() {
  const bool triggered = pendingRisingEdge_;
  pendingRisingEdge_ = false;
  return triggered;
}

bool VtTriggerService::consumeFallingEdge() {
  const bool triggered = pendingFallingEdge_;
  pendingFallingEdge_ = false;
  return triggered;
}

bool VtTriggerService::isHigh() const { return stableHigh_; }
