#pragma once

#include <WiFi.h>

namespace ThermalConfig {
constexpr bool kEnableWifiModemSleep = true;
constexpr wifi_power_t kWifiTxPower = WIFI_POWER_8_5dBm;
constexpr bool kEmitBootThermalSummary = true;
}  // namespace ThermalConfig
