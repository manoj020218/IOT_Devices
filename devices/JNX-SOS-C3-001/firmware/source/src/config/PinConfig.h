#pragma once

#include <Arduino.h>

namespace PinConfig {
constexpr gpio_num_t kPwmOutGpio = GPIO_NUM_4;
constexpr gpio_num_t kVtTriggerGpio = GPIO_NUM_3;
constexpr gpio_num_t kButtonGpio = GPIO_NUM_5;
constexpr gpio_num_t kStatusLedGpio = GPIO_NUM_8;
constexpr bool kStatusLedActiveLow = true;
}  // namespace PinConfig
