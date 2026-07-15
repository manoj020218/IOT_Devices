#include "SettingsManager.h"

#include <ArduinoJson.h>

#include "../config/ProductConfig.h"
#include "SpeakerProfiles.h"
#include "ToneProfiles.h"

namespace {
constexpr char kPrefsNamespace[] = "jnx-sos";
constexpr char kKeyProfile[] = "profile";
constexpr char kKeySpeaker[] = "speaker";
constexpr char kKeyVoltage[] = "voltage";
constexpr char kKeyNormalDuty[] = "normalDuty";
constexpr char kKeyBoostDuty[] = "boostDuty";
constexpr char kKeyBoostSec[] = "boostSec";
constexpr char kKeyMaxDuty[] = "maxDuty";
constexpr char kKeyOnSec[] = "onSec";
constexpr char kKeyRestSec[] = "restSec";
constexpr char kKeyLongRun[] = "longRun";
constexpr char kKeySweepMin[] = "sweepMin";
constexpr char kKeySweepMax[] = "sweepMax";
constexpr char kKeySweepStep[] = "sweepStep";
constexpr char kKeyBurstOn[] = "burstOn";
constexpr char kKeyBurstOff[] = "burstOff";
constexpr char kKeyBurstLimit[] = "burstLim";
constexpr char kKeyVtEnabled[] = "vtEnabled";
constexpr char kKeyVtMode[] = "vtMode";
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
  updated.vtTriggerProfileId = profileId;
  return persistIfChanged_(clamp(updated));
}

bool SettingsManager::updateSpeakerProfile(SpeakerProfileId speakerProfileId) {
  DeviceSettings updated = settings_;
  applySpeakerDefaults(updated, speakerProfileId);
  return persistIfChanged_(clamp(updated));
}

bool SettingsManager::saveSettings(const DeviceSettings& candidate) {
  return persistIfChanged_(clamp(candidate));
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
  doc["speakerProfile"] = SpeakerProfiles::toString(settings_.speakerProfileId);
  doc["inputVoltageProfile"] = voltageProfileToString(settings_.inputVoltageProfile);
  doc["onDurationSec"] = settings_.onDurationSec;
  doc["restDurationSec"] = settings_.restDurationSec;
  doc["sweepMinHz"] = settings_.sweepMinHz;
  doc["sweepMaxHz"] = settings_.sweepMaxHz;
  doc["sweepStepMs"] = settings_.sweepStepMs;
  doc["burstOnMs"] = settings_.burstOnMs;
  doc["burstOffMs"] = settings_.burstOffMs;
  doc["burstCycleLimit"] = settings_.burstCycleLimit;
  doc["longRunMode"] = settings_.longRunMode;
  doc["vtTriggerEnabled"] = settings_.vtTriggerEnabled;
  doc["vtTriggerMode"] = vtTriggerModeToString(settings_.vtTriggerMode);
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

bool SettingsManager::importSettingsJson(const String& json) {
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, json)) {
    return false;
  }

  DeviceSettings imported = settings_;
  imported.selectedProfileId = doc["selectedProfileId"] | imported.selectedProfileId;
  imported.speakerProfileId =
      SpeakerProfiles::fromString(doc["speakerProfile"] | SpeakerProfiles::toString(imported.speakerProfileId));
  imported.inputVoltageProfile =
      voltageProfileFromString(doc["inputVoltageProfile"] |
                               voltageProfileToString(imported.inputVoltageProfile));
  imported.onDurationSec = doc["onDurationSec"] | imported.onDurationSec;
  imported.restDurationSec = doc["restDurationSec"] | imported.restDurationSec;
  imported.sweepMinHz = doc["sweepMinHz"] | imported.sweepMinHz;
  imported.sweepMaxHz = doc["sweepMaxHz"] | imported.sweepMaxHz;
  imported.sweepStepMs = doc["sweepStepMs"] | imported.sweepStepMs;
  imported.burstOnMs = doc["burstOnMs"] | imported.burstOnMs;
  imported.burstOffMs = doc["burstOffMs"] | imported.burstOffMs;
  imported.burstCycleLimit = doc["burstCycleLimit"] | imported.burstCycleLimit;
  imported.longRunMode = doc["longRunMode"] | imported.longRunMode;
  imported.vtTriggerEnabled = doc["vtTriggerEnabled"] | imported.vtTriggerEnabled;
  imported.vtTriggerMode = vtTriggerModeFromString(
      doc["vtTriggerMode"] | vtTriggerModeToString(imported.vtTriggerMode));
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

DeviceSettings SettingsManager::defaultsFor(InputVoltageProfile voltageProfile) {
  DeviceSettings defaults;
  defaults.selectedProfileId = ToneProfiles::defaultProfile()->id;
  defaults.speakerProfileId = SpeakerProfiles::defaultProfile()->id;
  defaults.inputVoltageProfile = voltageProfile;
  defaults.boostDurationSec = 0;
  defaults.normalDutyPercent = 50;
  defaults.boostDutyPercent = 50;
  defaults.maxDutyPercent = 50;
  defaults.longRunMode = true;
  applySpeakerDefaults(defaults, defaults.speakerProfileId);
  defaults.vtTriggerEnabled = true;
  defaults.vtTriggerMode = VtTriggerMode::INCHING;
  defaults.vtTriggerProfileId = defaults.selectedProfileId;
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
  if (safe.speakerProfileId != SpeakerProfileId::SUH15_SAFE &&
      safe.speakerProfileId != SpeakerProfileId::SUH25) {
    safe.speakerProfileId = SpeakerProfiles::defaultProfile()->id;
  }

  safe.inputVoltageProfile = InputVoltageProfile::V24;
  safe.normalDutyPercent = 50;
  safe.boostDutyPercent = 50;
  safe.boostDurationSec = 0;
  safe.maxDutyPercent = 50;
  safe.sweepMinHz =
      constrain(safe.sweepMinHz, static_cast<uint16_t>(300), static_cast<uint16_t>(1999));
  safe.sweepMaxHz =
      constrain(safe.sweepMaxHz, static_cast<uint16_t>(301), static_cast<uint16_t>(2000));
  if (safe.sweepMinHz >= safe.sweepMaxHz) {
    applySpeakerDefaults(safe, safe.speakerProfileId);
  }
  safe.sweepStepMs =
      constrain(safe.sweepStepMs, static_cast<uint16_t>(5), static_cast<uint16_t>(100));
  safe.onDurationSec =
      constrain(safe.onDurationSec, static_cast<uint16_t>(1), static_cast<uint16_t>(30));
  safe.restDurationSec =
      constrain(safe.restDurationSec, static_cast<uint16_t>(5), static_cast<uint16_t>(30));
  safe.burstOnMs =
      constrain(safe.burstOnMs, static_cast<uint16_t>(0), static_cast<uint16_t>(5000));
  safe.burstOffMs =
      constrain(safe.burstOffMs, static_cast<uint16_t>(0), static_cast<uint16_t>(5000));
  safe.burstCycleLimit = constrain(safe.burstCycleLimit, static_cast<uint8_t>(0),
                                   static_cast<uint8_t>(30));
  if (safe.vtTriggerMode != VtTriggerMode::TIMED &&
      safe.vtTriggerMode != VtTriggerMode::TOGGLE &&
      safe.vtTriggerMode != VtTriggerMode::INCHING) {
    safe.vtTriggerMode = VtTriggerMode::INCHING;
  }
  if (ToneProfiles::findById(safe.vtTriggerProfileId) == nullptr) {
    safe.vtTriggerProfileId = safe.selectedProfileId;
  }
  safe.vtTriggerProfileId = safe.selectedProfileId;
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

void SettingsManager::applySpeakerDefaults(DeviceSettings& settings,
                                           SpeakerProfileId speakerProfileId) {
  const SpeakerProfile* profile = SpeakerProfiles::findById(speakerProfileId);
  settings.speakerProfileId = profile->id;
  settings.inputVoltageProfile = InputVoltageProfile::V24;
  settings.normalDutyPercent = 50;
  settings.boostDutyPercent = 50;
  settings.boostDurationSec = 0;
  settings.maxDutyPercent = 50;
  settings.onDurationSec = profile->maxOnDurationSec;
  settings.restDurationSec = profile->coolingPauseSec;
  settings.sweepMinHz = profile->sweepMinHz;
  settings.sweepMaxHz = profile->sweepMaxHz;
  settings.sweepStepMs = profile->sweepStepMs;
  settings.burstOnMs = profile->burstOnMs;
  settings.burstOffMs = profile->burstOffMs;
  settings.burstCycleLimit = 0;
}

const char* SettingsManager::voltageProfileToString(InputVoltageProfile profile) {
  static_cast<void>(profile);
  return "12V";
}

InputVoltageProfile SettingsManager::voltageProfileFromString(const String& value) {
  static_cast<void>(value);
  return InputVoltageProfile::V24;
}

const char* SettingsManager::retriggerModeToString(SosRetriggerMode mode) {
  return mode == SosRetriggerMode::RESTART ? "restart" : "extend";
}

SosRetriggerMode SettingsManager::retriggerModeFromString(const String& value) {
  return value.equalsIgnoreCase("restart") ? SosRetriggerMode::RESTART
                                           : SosRetriggerMode::EXTEND;
}

const char* SettingsManager::vtTriggerModeToString(VtTriggerMode mode) {
  switch (mode) {
    case VtTriggerMode::TIMED:
      return "timed";
    case VtTriggerMode::TOGGLE:
      return "toggle";
    case VtTriggerMode::INCHING:
    default:
      return "inching";
  }
}

VtTriggerMode SettingsManager::vtTriggerModeFromString(const String& value) {
  if (value.equalsIgnoreCase("timed")) {
    return VtTriggerMode::TIMED;
  }
  if (value.equalsIgnoreCase("toggle")) {
    return VtTriggerMode::TOGGLE;
  }
  return VtTriggerMode::INCHING;
}

void SettingsManager::load_() {
  DeviceSettings defaults = defaultsFor(InputVoltageProfile::V24);
  defaults.selectedProfileId = prefs_.getUChar(kKeyProfile, defaults.selectedProfileId);
  defaults.speakerProfileId =
      static_cast<SpeakerProfileId>(prefs_.getUChar(kKeySpeaker, static_cast<uint8_t>(defaults.speakerProfileId)));
  defaults.inputVoltageProfile =
      static_cast<InputVoltageProfile>(prefs_.getUChar(kKeyVoltage, static_cast<uint8_t>(defaults.inputVoltageProfile)));
  defaults.normalDutyPercent = prefs_.getUChar(kKeyNormalDuty, defaults.normalDutyPercent);
  defaults.boostDutyPercent = prefs_.getUChar(kKeyBoostDuty, defaults.boostDutyPercent);
  defaults.boostDurationSec = prefs_.getUShort(kKeyBoostSec, defaults.boostDurationSec);
  defaults.maxDutyPercent = prefs_.getUChar(kKeyMaxDuty, defaults.maxDutyPercent);
  defaults.onDurationSec = prefs_.getUShort(kKeyOnSec, defaults.onDurationSec);
  defaults.restDurationSec = prefs_.getUShort(kKeyRestSec, defaults.restDurationSec);
  defaults.longRunMode = prefs_.getBool(kKeyLongRun, defaults.longRunMode);
  defaults.sweepMinHz = prefs_.getUShort(kKeySweepMin, defaults.sweepMinHz);
  defaults.sweepMaxHz = prefs_.getUShort(kKeySweepMax, defaults.sweepMaxHz);
  defaults.sweepStepMs = prefs_.getUShort(kKeySweepStep, defaults.sweepStepMs);
  defaults.burstOnMs = prefs_.getUShort(kKeyBurstOn, defaults.burstOnMs);
  defaults.burstOffMs = prefs_.getUShort(kKeyBurstOff, defaults.burstOffMs);
  defaults.burstCycleLimit = prefs_.getUChar(kKeyBurstLimit, defaults.burstCycleLimit);
  defaults.vtTriggerEnabled = prefs_.getBool(kKeyVtEnabled, defaults.vtTriggerEnabled);
  defaults.vtTriggerMode = static_cast<VtTriggerMode>(
      prefs_.getUChar(kKeyVtMode, static_cast<uint8_t>(defaults.vtTriggerMode)));
  defaults.vtTriggerProfileId = prefs_.getUChar(kKeyVtProfile, defaults.vtTriggerProfileId);
  defaults.vtTriggerDurationSec =
      prefs_.getUShort(kKeyVtDuration, defaults.vtTriggerDurationSec);
  defaults.vtRetriggerMode = static_cast<SosRetriggerMode>(
      prefs_.getUChar(kKeyVtRetrigger, static_cast<uint8_t>(defaults.vtRetriggerMode)));
  defaults.vtCloudNotify = prefs_.getBool(kKeyVtNotify, defaults.vtCloudNotify);
  defaults.wifiSsid = prefs_.getString(kKeyWifiSsid, "");
  defaults.wifiPassword = prefs_.getString(kKeyWifiPass, "");
  defaults.otaAdminPassword =
      prefs_.getString(kKeyAdminPass, ProductConfig::kDefaultAdminPassword);
  settings_ = clamp(defaults);
}

bool SettingsManager::persistIfChanged_(const DeviceSettings& nextSettings) {
  if (equals_(settings_, nextSettings)) {
    return false;
  }

  prefs_.putUChar(kKeyProfile, nextSettings.selectedProfileId);
  prefs_.putUChar(kKeySpeaker, static_cast<uint8_t>(nextSettings.speakerProfileId));
  prefs_.putUChar(kKeyVoltage, static_cast<uint8_t>(nextSettings.inputVoltageProfile));
  prefs_.putUChar(kKeyNormalDuty, nextSettings.normalDutyPercent);
  prefs_.putUChar(kKeyBoostDuty, nextSettings.boostDutyPercent);
  prefs_.putUShort(kKeyBoostSec, nextSettings.boostDurationSec);
  prefs_.putUChar(kKeyMaxDuty, nextSettings.maxDutyPercent);
  prefs_.putUShort(kKeyOnSec, nextSettings.onDurationSec);
  prefs_.putUShort(kKeyRestSec, nextSettings.restDurationSec);
  prefs_.putBool(kKeyLongRun, nextSettings.longRunMode);
  prefs_.putUShort(kKeySweepMin, nextSettings.sweepMinHz);
  prefs_.putUShort(kKeySweepMax, nextSettings.sweepMaxHz);
  prefs_.putUShort(kKeySweepStep, nextSettings.sweepStepMs);
  prefs_.putUShort(kKeyBurstOn, nextSettings.burstOnMs);
  prefs_.putUShort(kKeyBurstOff, nextSettings.burstOffMs);
  prefs_.putUChar(kKeyBurstLimit, nextSettings.burstCycleLimit);
  prefs_.putBool(kKeyVtEnabled, nextSettings.vtTriggerEnabled);
  prefs_.putUChar(kKeyVtMode, static_cast<uint8_t>(nextSettings.vtTriggerMode));
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
         left.speakerProfileId == right.speakerProfileId &&
         left.inputVoltageProfile == right.inputVoltageProfile &&
         left.normalDutyPercent == right.normalDutyPercent &&
         left.boostDutyPercent == right.boostDutyPercent &&
         left.boostDurationSec == right.boostDurationSec &&
         left.maxDutyPercent == right.maxDutyPercent &&
         left.onDurationSec == right.onDurationSec &&
         left.restDurationSec == right.restDurationSec &&
         left.longRunMode == right.longRunMode &&
         left.sweepMinHz == right.sweepMinHz &&
         left.sweepMaxHz == right.sweepMaxHz &&
         left.sweepStepMs == right.sweepStepMs &&
         left.burstOnMs == right.burstOnMs &&
         left.burstOffMs == right.burstOffMs &&
         left.burstCycleLimit == right.burstCycleLimit &&
         left.vtTriggerEnabled == right.vtTriggerEnabled &&
         left.vtTriggerMode == right.vtTriggerMode &&
         left.vtTriggerProfileId == right.vtTriggerProfileId &&
         left.vtTriggerDurationSec == right.vtTriggerDurationSec &&
         left.vtRetriggerMode == right.vtRetriggerMode &&
         left.vtCloudNotify == right.vtCloudNotify &&
         left.wifiSsid == right.wifiSsid &&
         left.wifiPassword == right.wifiPassword &&
         left.otaAdminPassword == right.otaAdminPassword;
}
