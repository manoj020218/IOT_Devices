#pragma once

#include <Arduino.h>

class VtTriggerService {
 public:
  void begin(gpio_num_t gpio);
  void update(uint32_t nowMs);
  bool consumeRisingEdge();
  bool consumeFallingEdge();
  bool isHigh() const;

 private:
  gpio_num_t gpio_ = GPIO_NUM_NC;
  bool rawHigh_ = false;
  bool stableHigh_ = false;
  bool pendingRisingEdge_ = false;
  bool pendingFallingEdge_ = false;
  uint32_t lastEdgeMs_ = 0;
  uint32_t lastTriggerMs_ = 0;
};
