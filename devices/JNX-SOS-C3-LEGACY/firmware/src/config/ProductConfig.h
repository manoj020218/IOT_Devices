#pragma once

#include <Arduino.h>

#ifndef ENABLE_MQTT
#define ENABLE_MQTT 0
#endif

namespace ProductConfig {
constexpr char kProductName[] = "Jenix Loud SOS Siren";
constexpr char kProductId[] = "PD-JNX-SOS-25W-C3";
constexpr char kProductCategory[] = "Emergency Siren / Safety Alert";
constexpr char kFirmwareName[] = "jnx-sos-c3";
constexpr char kFirmwareVersion[] = "1.0.0";
constexpr char kApSsidPrefix[] = "JNX-SOS-";
constexpr char kBleNamePrefix[] = "JNX-SOS-BLE-";
constexpr char kMdnsHostname[] = "jenix-sos";
constexpr char kMdnsHostLabel[] = "jenix-sos.local";
constexpr char kDefaultApPassword[] = "12345678";
constexpr char kDefaultAdminPassword[] = "admin123";
constexpr uint16_t kHttpPort = 80;
constexpr uint32_t kSerialBaud = 115200;
constexpr uint16_t kDefaultTestDurationSec = 10;
constexpr uint16_t kWifiReconnectMs = 30000;
constexpr uint16_t kWifiConnectTimeoutMs = 15000;
constexpr uint8_t kProfileCount = 10;
}  // namespace ProductConfig
