#pragma once

#include <Arduino.h>
#include <Preferences.h>

enum class InputVoltageProfile : uint8_t {
  V24 = 0,
  V30 = 1,
};

enum class SosRetriggerMode : uint8_t {
  RESTART = 0,
  EXTEND = 1,
};

struct DeviceSettings {
  uint8_t selectedProfileId = 2;
  InputVoltageProfile inputVoltageProfile = InputVoltageProfile::V24;
  uint8_t normalDutyPercent = 55;
  uint8_t boostDutyPercent = 70;
  uint16_t boostDurationSec = 10;
  uint8_t maxDutyPercent = 80;
  uint16_t onDurationSec = 180;
  uint16_t restDurationSec = 30;
  bool longRunMode = true;
  bool vtTriggerEnabled = true;
  uint8_t vtTriggerProfileId = 9;
  uint16_t vtTriggerDurationSec = 60;
  SosRetriggerMode vtRetriggerMode = SosRetriggerMode::EXTEND;
  bool vtCloudNotify = false;
  String wifiSsid;
  String wifiPassword;
  String otaAdminPassword;
};

class SettingsManager {
 public:
  void begin();
  const DeviceSettings& getSettings() const;
  bool updateSelectedProfile(uint8_t profileId);
  bool saveSettings(const DeviceSettings& candidate);
  bool importSettingsJson(const String& json);
  bool restoreDefaults();
  bool clearWifiCredentials();
  bool factoryReset();
  String exportSettingsJson() const;

  static DeviceSettings defaultsFor(InputVoltageProfile voltageProfile);
  static DeviceSettings clamp(const DeviceSettings& raw);
  static const char* voltageProfileToString(InputVoltageProfile profile);
  static InputVoltageProfile voltageProfileFromString(const String& value);
  static const char* retriggerModeToString(SosRetriggerMode mode);
  static SosRetriggerMode retriggerModeFromString(const String& value);

 private:
  Preferences prefs_;
  DeviceSettings settings_;

  void load_();
  bool persistIfChanged_(const DeviceSettings& nextSettings);
  static bool equals_(const DeviceSettings& left, const DeviceSettings& right);
};
