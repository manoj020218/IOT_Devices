#pragma once

#include <Arduino.h>

class IndicatorService {
 public:
  void begin(gpio_num_t gpio, bool activeLow);
  void update(uint32_t nowMs, bool sosPlaying, bool buttonPressed);

 private:
  gpio_num_t gpio_ = GPIO_NUM_NC;
  bool activeLow_ = false;
  bool initialized_ = false;
  bool currentOn_ = false;
  bool previousButtonPressed_ = false;
  uint32_t pulseUntilMs_ = 0;

  void write_(bool on);
};
