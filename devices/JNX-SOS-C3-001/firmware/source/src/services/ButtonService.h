#pragma once

#include <Arduino.h>

enum class ButtonEvent : uint8_t {
  NONE = 0,
  SHORT_PRESS = 1,
  LONG_PRESS = 2,
  VERY_LONG_PRESS = 3,
};

class ButtonService {
 public:
  void begin(gpio_num_t gpio);
  void update(uint32_t nowMs);
  ButtonEvent consumeEvent();
  bool isPressed() const;

 private:
  gpio_num_t gpio_ = GPIO_NUM_NC;
  bool stablePressed_ = false;
  bool rawPressed_ = false;
  bool veryLongFired_ = false;
  uint32_t debounceStartedMs_ = 0;
  uint32_t pressStartedMs_ = 0;
  ButtonEvent pendingEvent_ = ButtonEvent::NONE;
};
