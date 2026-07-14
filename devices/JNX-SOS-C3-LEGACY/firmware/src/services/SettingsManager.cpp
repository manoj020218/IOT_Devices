#include "SettingsManager.h"

#include <ArduinoJson.h>

#include "../config/ProductConfig.h"
#include "ToneProfiles.h"

namespace {
constexpr char kPrefsNamespace[] = "jnx-sos";
constexpr char kKeyProfile[] = "profile";
constexpr char kKeyVoltage[] = "voltage";
constexpr char kKeyNormalDuty[] = "normalDuty";
constexpr char kKeyBoostDuty[] = "boostDuty";
constexpr char kKeyBoostSec[] = "boostSec";
constexpr char kKeyMaxDuty[] = "maxDuty";
constexpr char kKeyOnSec[] = "onSec";
constexpr char kKeyRestSec[] = "restSec";
constexpr char kKeyLongRun[] = "longRun";
constexpr char kKeyVtEnabled[] = "vtEnabled";
constexpr char kKeyVtProfile[] = "vtProfile";
constexpr char kKeyVtDuration[] = "vtDuration";
constexpr char kKeyVtRetrigger[] = "vtRetrig";
constexpr char kKeyVtNotify[] = "vtNotify";
constexpr char kKeyWifiSsid[] = "wifiSsid";
constexpr char kKeyWifiPass[] = "wifiPass";
constexpr char kKeyAdminPass[] = "adminPass";
}  // namespace

void SettingsManager::begin() {
  prefs_.begin(kPrefsNamespace, false);
  load_();
}

const DeviceSettings& SettingsManager::getSettings() const { return settings_; }

bool SettingsManager::updateSelectedProfile(uint8_t profileId) {
  DeviceSettings updated = settings_;
  updated.selectedProfileId = profileId;
  return persistIfChanged_(clamp(updated));
}

bool SettingsManager::saveSettings(const DeviceSettings& candidate) {
  return persistIfChanged_(clamp(candidate));
}

bool SettingsManager::importSettingsJson(const String& json) {
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, json)) {
    return false;
  }

  DeviceSettings imported = settings_;
  imported.selectedProfileId = doc["selectedProfileId"] | imported.selectedProfileId;
  imported.inputVoltageProfile =
      voltageProfileFromString(doc["inputVoltageProfile"] |
                               voltageProfileToString(imported.inputVoltageProfile));
  imported.normalDutyPercent = doc["normalDutyPercent"] | imported.normalDutyPercent;
  imported.boostDutyPercent = doc["boostDutyPercent"] | imported.boostDutyPercent;
  imported.boostDurationSec = doc["boostDurationSec"] | imported.boostDurationSec;
  imported.maxDutyPercent = doc["maxDutyPercent"] | imported.maxDutyPercent;
  imported.onDurationSec = doc["onDurationSec"] | imported.onDurationSec;
  imported.restDurationSec = doc["restDurationSec"] | imported.restDurationSec;
  imported.longRunMode = doc["longRunMode"] | imported.longRunMode;
  imported.vtTriggerEnabled = doc["vtTriggerEnabled"] | imported.vtTriggerEnabled;
  imported.vtTriggerProfileId = doc["vtTriggerProfileId"] | imported.vtTriggerProfileId;
  imported.vtTriggerDurationSec =
      doc["vtTriggerDurationSec"] | imported.vtTriggerDurationSec;
  imported.vtRetriggerMode = retriggerModeFromString(
      doc["vtRetriggerMode"] | retriggerModeToString(imported.vtRetriggerMode));
  imported.vtCloudNotify = doc["vtCloudNotify"] | imported.vtCloudNotify;
  imported.wifiSsid = doc["wifiSsid"] | imported.wifiSsid;
  imported.wifiPassword = doc["wifiPassword"] | imported.wifiPassword;
  imported.otaAdminPassword = doc["otaAdminPassword"] | imported.otaAdminPassword;
  return persistIfChanged_(clamp(imported));
}

bool SettingsManager::restoreDefaults() {
  DeviceSettings defaults = defaultsFor(settings_.inputVoltageProfile);
  defaults.wifiSsid = settings_.wifiSsid;
  defaults.wifiPassword = settings_.wifiPassword;
  defaults.otaAdminPassword = settings_.otaAdminPassword;
  return persistIfChanged_(defaults);
}

bool SettingsManager::clearWifiCredentials() {
  DeviceSettings updated = settings_;
  updated.wifiSsid = "";
  updated.wifiPassword = "";
  return persistIfChanged_(updated);
}

bool SettingsManager::factoryReset() {
  DeviceSettings defaults = defaultsFor(InputVoltageProfile::V24);
  defaults.otaAdminPassword = ProductConfig::kDefaultAdminPassword;
  return persistIfChanged_(defaults);
}

String SettingsManager::exportSettingsJson() const {
  DynamicJsonDocument doc(1024);
  doc["selectedProfileId"] = settings_.selectedProfileId;
  doc["inputVoltageProfile"] = voltageProfileToString(settings_.inputVoltageProfile);
  doc["normalDutyPercent"] = settings_.normalDutyPercent;
  doc["boostDutyPercent"] = settings_.boostDutyPercent;
  doc["boostDurationSec"] = settings_.boostDurationSec;
  doc["maxDutyPercent"] = settings_.maxDutyPercent;
  doc["onDurationSec"] = settings_.onDurationSec;
  doc["restDurationSec"] = settings_.restDurationSec;
  doc["longRunMode"] = settings_.longRunMode;
  doc["vtTriggerEnabled"] = settings_.vtTriggerEnabled;
  doc["vtTriggerProfileId"] = settings_.vtTriggerProfileId;
  doc["vtTriggerDurationSec"] = settings_.vtTriggerDurationSec;
  doc["vtRetriggerMode"] = retriggerModeToString(settings_.vtRetriggerMode);
  doc["vtCloudNotify"] = settings_.vtCloudNotify;
  doc["wifiSsid"] = settings_.wifiSsid;
  doc["wifiPassword"] = settings_.wifiPassword;
  doc["otaAdminPassword"] = settings_.otaAdminPassword;
  String json;
  serializeJsonPretty(doc, json);
  return json;
}

DeviceSettings SettingsManager::defaultsFor(InputVoltageProfile voltageProfile) {
  DeviceSettings defaults;
  defaults.selectedProfileId = ToneProfiles::defaultProfile()->id;
  defaults.inputVoltageProfile = voltageProfile;
  defaults.normalDutyPercent = voltageProfile == InputVoltageProfile::V30 ? 40 : 55;
  defaults.boostDutyPercent = voltageProfile == InputVoltageProfile::V30 ? 60 : 70;
  defaults.boostDurationSec = 10;
  defaults.maxDutyPercent = voltageProfile == InputVoltageProfile::V30 ? 60 : 80;
  defaults.onDurationSec = 180;
  defaults.restDurationSec = 30;
  defaults.longRunMode = true;
  defaults.vtTriggerEnabled = true;
  defaults.vtTriggerProfileId = 9;
  defaults.vtTriggerDurationSec = 60;
  defaults.vtRetriggerMode = SosRetriggerMode::EXTEND;
  defaults.vtCloudNotify = false;
  defaults.otaAdminPassword = ProductConfig::kDefaultAdminPassword;
  return defaults;
}

DeviceSettings SettingsManager::clamp(const DeviceSettings& raw) {
  DeviceSettings safe = raw;
  if (ToneProfiles::findById(safe.selectedProfileId) == nullptr) {
    safe.selectedProfileId = ToneProfiles::defaultProfile()->id;
  }

  safe.normalDutyPercent = constrain(safe.normalDutyPercent, static_cast<uint8_t>(5),
                                     static_cast<uint8_t>(95));
  safe.boostDutyPercent = constrain(safe.boostDutyPercent, static_cast<uint8_t>(5),
                                    static_cast<uint8_t>(95));
  safe.maxDutyPercent = constrain(safe.maxDutyPercent, static_cast<uint8_t>(5),
                                  static_cast<uint8_t>(95));
  if (safe.inputVoltageProfile == InputVoltageProfile::V30) {
    safe.maxDutyPercent = min<uint8_t>(safe.maxDutyPercent, 60);
  } else {
    safe.maxDutyPercent = min<uint8_t>(safe.maxDutyPercent, 80);
  }

  safe.boostDurationSec = constrain(safe.boostDurationSec, static_cast<uint16_t>(0),
                                    static_cast<uint16_t>(120));
  safe.onDurationSec = constrain(safe.onDurationSec, static_cast<uint16_t>(10),
                                 static_cast<uint16_t>(1800));
  safe.restDurationSec = constrain(safe.restDurationSec, static_cast<uint16_t>(5),
                                   static_cast<uint16_t>(600));
  if (ToneProfiles::findById(safe.vtTriggerProfileId) == nullptr) {
    safe.vtTriggerProfileId = 9;
  }
  safe.vtTriggerDurationSec =
      constrain(safe.vtTriggerDurationSec, static_cast<uint16_t>(10),
                static_cast<uint16_t>(1800));
  safe.wifiSsid.trim();
  safe.otaAdminPassword.trim();
  if (safe.otaAdminPassword.isEmpty()) {
    safe.otaAdminPassword = ProductConfig::kDefaultAdminPassword;
  }
  return safe;
}

const char* SettingsManager::voltageProfileToString(InputVoltageProfile profile) {
  return profile == InputVoltageProfile::V30 ? "30V" : "24V";
}

InputVoltageProfile SettingsManager::voltageProfileFromString(const String& value) {
  return value.equalsIgnoreCase("30V") ? InputVoltageProfile::V30
                                       : InputVoltageProfile::V24;
}

const char* SettingsManager::retriggerModeToString(SosRetriggerMode mode) {
  return mode == SosRetriggerMode::RESTART ? "restart" : "extend";
}

SosRetriggerMode SettingsManager::retriggerModeFromString(const String& value) {
  return value.equalsIgnoreCase("restart") ? SosRetriggerMode::RESTART
                                           : SosRetriggerMode::EXTEND;
}

void SettingsManager::load_() {
  DeviceSettings loaded = defaultsFor(InputVoltageProfile::V24);
  loaded.selectedProfileId = prefs_.getUChar(kKeyProfile, loaded.selectedProfileId);
  loaded.inputVoltageProfile = static_cast<InputVoltageProfile>(
      prefs_.getUChar(kKeyVoltage, static_cast<uint8_t>(loaded.inputVoltageProfile)));
  loaded.normalDutyPercent = prefs_.getUChar(kKeyNormalDuty, loaded.normalDutyPercent);
  loaded.boostDutyPercent = prefs_.getUChar(kKeyBoostDuty, loaded.boostDutyPercent);
  loaded.boostDurationSec = prefs_.getUShort(kKeyBoostSec, loaded.boostDurationSec);
  loaded.maxDutyPercent = prefs_.getUChar(kKeyMaxDuty, loaded.maxDutyPercent);
  loaded.onDurationSec = prefs_.getUShort(kKeyOnSec, loaded.onDurationSec);
  loaded.restDurationSec = prefs_.getUShort(kKeyRestSec, loaded.restDurationSec);
  loaded.longRunMode = prefs_.getBool(kKeyLongRun, loaded.longRunMode);
  loaded.vtTriggerEnabled = prefs_.getBool(kKeyVtEnabled, loaded.vtTriggerEnabled);
  loaded.vtTriggerProfileId = prefs_.getUChar(kKeyVtProfile, loaded.vtTriggerProfileId);
  loaded.vtTriggerDurationSec = prefs_.getUShort(kKeyVtDuration, loaded.vtTriggerDurationSec);
  loaded.vtRetriggerMode = static_cast<SosRetriggerMode>(
      prefs_.getUChar(kKeyVtRetrigger, static_cast<uint8_t>(loaded.vtRetriggerMode)));
  loaded.vtCloudNotify = prefs_.getBool(kKeyVtNotify, loaded.vtCloudNotify);
  loaded.wifiSsid = prefs_.getString(kKeyWifiSsid, loaded.wifiSsid);
  loaded.wifiPassword = prefs_.getString(kKeyWifiPass, loaded.wifiPassword);
  loaded.otaAdminPassword = prefs_.getString(kKeyAdminPass, loaded.otaAdminPassword);
  settings_ = clamp(loaded);
}

bool SettingsManager::persistIfChanged_(const DeviceSettings& nextSettings) {
  if (equals_(settings_, nextSettings)) {
    return false;
  }

  prefs_.putUChar(kKeyProfile, nextSettings.selectedProfileId);
  prefs_.putUChar(kKeyVoltage, static_cast<uint8_t>(nextSettings.inputVoltageProfile));
  prefs_.putUChar(kKeyNormalDuty, nextSettings.normalDutyPercent);
  prefs_.putUChar(kKeyBoostDuty, nextSettings.boostDutyPercent);
  prefs_.putUShort(kKeyBoostSec, nextSettings.boostDurationSec);
  prefs_.putUChar(kKeyMaxDuty, nextSettings.maxDutyPercent);
  prefs_.putUShort(kKeyOnSec, nextSettings.onDurationSec);
  prefs_.putUShort(kKeyRestSec, nextSettings.restDurationSec);
  prefs_.putBool(kKeyLongRun, nextSettings.longRunMode);
  prefs_.putBool(kKeyVtEnabled, nextSettings.vtTriggerEnabled);
  prefs_.putUChar(kKeyVtProfile, nextSettings.vtTriggerProfileId);
  prefs_.putUShort(kKeyVtDuration, nextSettings.vtTriggerDurationSec);
  prefs_.putUChar(kKeyVtRetrigger, static_cast<uint8_t>(nextSettings.vtRetriggerMode));
  prefs_.putBool(kKeyVtNotify, nextSettings.vtCloudNotify);
  prefs_.putString(kKeyWifiSsid, nextSettings.wifiSsid);
  prefs_.putString(kKeyWifiPass, nextSettings.wifiPassword);
  prefs_.putString(kKeyAdminPass, nextSettings.otaAdminPassword);
  settings_ = nextSettings;
  return true;
}

bool SettingsManager::equals_(const DeviceSettings& left, const DeviceSettings& right) {
  return left.selectedProfileId == right.selectedProfileId &&
         left.inputVoltageProfile == right.inputVoltageProfile &&
         left.normalDutyPercent == right.normalDutyPercent &&
         left.boostDutyPercent == right.boostDutyPercent &&
         left.boostDurationSec == right.boostDurationSec &&
         left.maxDutyPercent == right.maxDutyPercent &&
         left.onDurationSec == right.onDurationSec &&
         left.restDurationSec == right.restDurationSec &&
         left.longRunMode == right.longRunMode &&
         left.vtTriggerEnabled == right.vtTriggerEnabled &&
         left.vtTriggerProfileId == right.vtTriggerProfileId &&
         left.vtTriggerDurationSec == right.vtTriggerDurationSec &&
         left.vtRetriggerMode == right.vtRetriggerMode &&
         left.vtCloudNotify == right.vtCloudNotify &&
         left.wifiSsid == right.wifiSsid &&
         left.wifiPassword == right.wifiPassword &&
         left.otaAdminPassword == right.otaAdminPassword;
}
