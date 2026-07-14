#include "WebServerService.h"

#include <ArduinoJson.h>

#include "../config/ProductConfig.h"
#include "../web/WebAssets.h"
#include "ToneProfiles.h"

WebServerService::WebServerService(SettingsManager& settingsManager, ToneEngine& toneEngine,
                                   WifiManagerService& wifiManager,
                                   ButtonService& buttonService, OtaService& otaService,
                                   StatusProvider statusProvider)
    : settingsManager_(settingsManager),
      toneEngine_(toneEngine),
      wifiManager_(wifiManager),
      buttonService_(buttonService),
      otaService_(otaService),
      statusProvider_(statusProvider) {}

void WebServerService::begin() {
  const char* headerKeys[] = {"X-Admin-Password"};
  server_.collectHeaders(headerKeys, 1);
  registerRoutes_();
  otaService_.registerRoutes(server_);
  server_.begin();
}

void WebServerService::handleClient() { server_.handleClient(); }

void WebServerService::registerRoutes_() {
  server_.on("/", HTTP_GET, [this]() { server_.send_P(200, "text/html", WebAssets::kIndexHtml); });
  server_.on("/api/status", HTTP_GET, [this]() { sendJson_(200, buildStatusJson_()); });
  server_.on("/api/profiles", HTTP_GET, [this]() { sendJson_(200, buildProfilesJson_()); });
  server_.on("/api/settings", HTTP_GET, [this]() { sendJson_(200, buildSettingsJson_()); });
  server_.on("/api/settings/export", HTTP_GET,
             [this]() { server_.send(200, "application/json", settingsManager_.exportSettingsJson()); });
  server_.on("/api/wifi/scan", HTTP_GET,
             [this]() { sendJson_(200, wifiManager_.scanNetworksJson()); });

  server_.on("/api/select-profile", HTTP_POST, [this]() {
    DynamicJsonDocument doc(256);
    if (!parseJsonBody_(doc)) {
      sendJson_(400, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
      return;
    }
    settingsManager_.updateSelectedProfile(doc["id"] | 0);
    sendJson_(200, "{\"ok\":true}");
  });

  server_.on("/api/test-profile", HTTP_POST, [this]() {
    DynamicJsonDocument doc(256);
    if (!parseJsonBody_(doc)) {
      sendJson_(400, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
      return;
    }
    const uint8_t id = doc["id"] | 0;
    const uint16_t durationSec = doc["durationSec"] | ProductConfig::kDefaultTestDurationSec;
    if (!toneEngine_.startTest(id, settingsManager_.getSettings(), durationSec)) {
      sendJson_(400, "{\"ok\":false,\"error\":\"Unknown tone profile\"}");
      return;
    }
    sendJson_(200, "{\"ok\":true}");
  });

  server_.on("/api/stop", HTTP_POST, [this]() {
    toneEngine_.stop();
    sendJson_(200, "{\"ok\":true}");
  });

  server_.on("/api/sos", HTTP_POST, [this]() {
    toneEngine_.startCommand(9, settingsManager_.getSettings(), true);
    sendJson_(200, "{\"ok\":true}");
  });

  server_.on("/api/settings", HTTP_POST, [this]() {
    DynamicJsonDocument doc(768);
    if (!parseJsonBody_(doc)) {
      sendJson_(400, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
      return;
    }

    DeviceSettings updated = settingsManager_.getSettings();
    updated.inputVoltageProfile = SettingsManager::voltageProfileFromString(
        doc["inputVoltageProfile"] |
        SettingsManager::voltageProfileToString(updated.inputVoltageProfile));
    updated.normalDutyPercent = doc["normalDutyPercent"] | updated.normalDutyPercent;
    updated.boostDutyPercent = doc["boostDutyPercent"] | updated.boostDutyPercent;
    updated.boostDurationSec = doc["boostDurationSec"] | updated.boostDurationSec;
    updated.maxDutyPercent = doc["maxDutyPercent"] | updated.maxDutyPercent;
    updated.onDurationSec = doc["onDurationSec"] | updated.onDurationSec;
    updated.restDurationSec = doc["restDurationSec"] | updated.restDurationSec;
    updated.longRunMode = doc["longRunMode"] | updated.longRunMode;
    updated.vtTriggerEnabled = doc["vtTriggerEnabled"] | updated.vtTriggerEnabled;
    updated.vtTriggerProfileId = doc["vtTriggerProfileId"] | updated.vtTriggerProfileId;
    updated.vtTriggerDurationSec = doc["vtTriggerDurationSec"] | updated.vtTriggerDurationSec;
    updated.vtRetriggerMode = SettingsManager::retriggerModeFromString(
        doc["vtRetriggerMode"] |
        SettingsManager::retriggerModeToString(updated.vtRetriggerMode));
    updated.vtCloudNotify = doc["vtCloudNotify"] | updated.vtCloudNotify;
    updated.otaAdminPassword = String(static_cast<const char*>(doc["otaAdminPassword"] | ""));
    settingsManager_.saveSettings(updated);
    sendJson_(200, "{\"ok\":true}");
  });

  server_.on("/api/settings/default", HTTP_POST, [this]() {
    settingsManager_.restoreDefaults();
    sendJson_(200, "{\"ok\":true}");
  });

  server_.on("/api/settings/import", HTTP_POST, [this]() {
    DynamicJsonDocument doc(2048);
    if (!parseJsonBody_(doc) || !settingsManager_.importSettingsJson(doc["json"] | "")) {
      sendJson_(400, "{\"ok\":false,\"error\":\"Invalid settings payload\"}");
      return;
    }
    sendJson_(200, "{\"ok\":true}");
  });

  server_.on("/api/wifi/save", HTTP_POST, [this]() {
    DynamicJsonDocument doc(512);
    if (!parseJsonBody_(doc)) {
      sendJson_(400, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
      return;
    }
    wifiManager_.saveCredentials(doc["ssid"] | "", doc["password"] | "");
    sendJson_(200, "{\"ok\":true,\"message\":\"Wi-Fi settings saved\"}");
  });

  server_.on("/api/reboot", HTTP_POST, [this]() {
    sendJson_(200, "{\"ok\":true,\"message\":\"Rebooting\"}");
    delay(150);
    ESP.restart();
  });

  server_.on("/api/factory-reset", HTTP_POST, [this]() {
    toneEngine_.stop();
    settingsManager_.factoryReset();
    sendJson_(200, "{\"ok\":true,\"message\":\"Factory reset complete. Rebooting.\"}");
    delay(200);
    ESP.restart();
  });

  server_.onNotFound([this]() {
    if (server_.uri().startsWith("/api/")) {
      sendJson_(404, "{\"ok\":false,\"error\":\"Not found\"}");
      return;
    }
    server_.sendHeader("Location", "/");
    server_.send(302, "text/plain", "");
  });
}

bool WebServerService::parseJsonBody_(DynamicJsonDocument& doc) {
  return deserializeJson(doc, server_.arg("plain")) == DeserializationError::Ok;
}

void WebServerService::sendJson_(int statusCode, const String& json) {
  server_.send(statusCode, "application/json", json);
}

String WebServerService::buildStatusJson_() const {
  const DeviceSettings& settings = settingsManager_.getSettings();
  const ToneProfile* selected = ToneProfiles::findById(settings.selectedProfileId);
  const ToneProfile* active = toneEngine_.activeProfile();
  const ToneProfile* vtProfile = ToneProfiles::findById(settings.vtTriggerProfileId);
  const DeviceState state = statusProvider_ ? statusProvider_() : DeviceState();
  const uint32_t remainingMs = toneEngine_.remainingMs(millis(), settings);
  DynamicJsonDocument doc(1536);
  doc["deviceName"] = ProductConfig::kProductName;
  doc["pid"] = ProductConfig::kProductId;
  doc["firmwareVersion"] = ProductConfig::kFirmwareVersion;
  doc["wifiMode"] = wifiManager_.wifiMode();
  doc["ipAddress"] = wifiManager_.stationIp();
  doc["apIpAddress"] = wifiManager_.accessPointIp();
  doc["stationUrl"] = wifiManager_.stationUrl();
  doc["apUrl"] = wifiManager_.accessPointUrl();
  doc["mdnsUrl"] = wifiManager_.mdnsUrl();
  doc["mdnsHost"] = wifiManager_.mdnsHostname();
  doc["preferredUrl"] = wifiManager_.staConnected() ? wifiManager_.mdnsUrl()
                                                    : wifiManager_.accessPointUrl();
  doc["selectedProfileId"] = settings.selectedProfileId;
  doc["selectedTone"] = selected ? selected->name : "Unknown";
  doc["sirenState"] = sirenStateToString(toneEngine_.state());
  doc["uptimeSec"] = state.uptimeSec;
  doc["remainingMs"] = remainingMs;
  doc["remainingLabel"] = remainingLabel_(remainingMs);
  doc["activeDutyPercent"] = toneEngine_.activeDutyPercent();
  doc["activeFrequencyHz"] = toneEngine_.activeFrequencyHz();
  doc["buttonPressed"] = state.buttonPressed;
  doc["vtTriggerEnabled"] = settings.vtTriggerEnabled;
  doc["vtTriggerHigh"] = state.vtTriggerHigh;
  doc["vtTriggerProfileId"] = settings.vtTriggerProfileId;
  doc["vtTriggerProfileName"] = vtProfile ? vtProfile->name : "";
  doc["vtTriggerDurationSec"] = settings.vtTriggerDurationSec;
  doc["vtRetriggerMode"] = SettingsManager::retriggerModeToString(settings.vtRetriggerMode);
  doc["vtCloudNotify"] = settings.vtCloudNotify;
  doc["sosPressCount"] = state.sosPressCount;
  doc["commandActive"] = toneEngine_.commandActive();
  doc["testMode"] = toneEngine_.isTestMode();
  doc["sosActive"] = toneEngine_.isSosOverride();
  doc["staConnected"] = wifiManager_.staConnected();
  doc["connectedSsid"] = wifiManager_.connectedSsid();
  doc["activeTone"] = active ? active->name : "";
  String json;
  serializeJson(doc, json);
  return json;
}

String WebServerService::buildProfilesJson_() const {
  DynamicJsonDocument doc(4096);
  JsonArray items = doc.to<JsonArray>();
  for (size_t index = 0; index < ToneProfiles::count(); ++index) {
    const ToneProfile& profile = ToneProfiles::all()[index];
    JsonObject item = items.createNestedObject();
    item["id"] = profile.id;
    item["name"] = profile.name;
    item["description"] = profile.description;
    item["frequencyPattern"] = profile.frequencyPattern;
    item["dutyProfile"] = profile.dutyProfile;
    item["recommendedUse"] = profile.recommendedUse;
    item["maxSafeDuty"] = profile.maxSafeDuty;
    item["longDurationAllowed"] = profile.longDurationAllowed;
  }
  String json;
  serializeJson(doc, json);
  return json;
}

String WebServerService::buildSettingsJson_() const {
  const DeviceSettings& settings = settingsManager_.getSettings();
  DynamicJsonDocument doc(1024);
  doc["selectedProfileId"] = settings.selectedProfileId;
  doc["inputVoltageProfile"] = SettingsManager::voltageProfileToString(settings.inputVoltageProfile);
  doc["normalDutyPercent"] = settings.normalDutyPercent;
  doc["boostDutyPercent"] = settings.boostDutyPercent;
  doc["boostDurationSec"] = settings.boostDurationSec;
  doc["maxDutyPercent"] = settings.maxDutyPercent;
  doc["onDurationSec"] = settings.onDurationSec;
  doc["restDurationSec"] = settings.restDurationSec;
  doc["longRunMode"] = settings.longRunMode;
  doc["vtTriggerEnabled"] = settings.vtTriggerEnabled;
  doc["vtTriggerProfileId"] = settings.vtTriggerProfileId;
  doc["vtTriggerDurationSec"] = settings.vtTriggerDurationSec;
  doc["vtRetriggerMode"] = SettingsManager::retriggerModeToString(settings.vtRetriggerMode);
  doc["vtCloudNotify"] = settings.vtCloudNotify;
  doc["wifiSsid"] = settings.wifiSsid;
  doc["wifiPassword"] = settings.wifiPassword;
  doc["otaAdminPassword"] = settings.otaAdminPassword;
  doc["apSsid"] = wifiManager_.accessPointSsid();
  doc["bleName"] = wifiManager_.bleName();
  doc["apUrl"] = wifiManager_.accessPointUrl();
  doc["stationUrl"] = wifiManager_.stationUrl();
  doc["mdnsUrl"] = wifiManager_.mdnsUrl();
  doc["mdnsHost"] = wifiManager_.mdnsHostname();
  String json;
  serializeJson(doc, json);
  return json;
}

String WebServerService::remainingLabel_(uint32_t remainingMs) {
  const uint32_t totalSeconds = remainingMs / 1000UL;
  const uint32_t minutes = totalSeconds / 60UL;
  const uint32_t seconds = totalSeconds % 60UL;
  char buffer[12];
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu", static_cast<unsigned long>(minutes),
           static_cast<unsigned long>(seconds));
  return String(buffer);
}
