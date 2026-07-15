#include "WifiManagerService.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>

#include "../config/ProductConfig.h"
#include "../config/ThermalConfig.h"

WifiManagerService::WifiManagerService(SettingsManager& settingsManager)
    : settingsManager_(settingsManager) {}

void WifiManagerService::begin() {
  const String suffix = suffixFromMac_();
  apSsid_ = String(ProductConfig::kApSsidPrefix) + suffix;
  bleName_ = String(ProductConfig::kBleNamePrefix) + suffix;

  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  delay(50);
  beginStationIfConfigured_();
  startMdns_();
}

void WifiManagerService::update(uint32_t nowMs) {
  if (!staEnabled_ || WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (nowMs - lastReconnectAttemptMs_ < ProductConfig::kWifiReconnectMs) {
    return;
  }

  const DeviceSettings& settings = settingsManager_.getSettings();
  if (settings.wifiSsid.isEmpty()) {
    return;
  }

  lastReconnectAttemptMs_ = nowMs;
  WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPassword.c_str());
}

bool WifiManagerService::saveCredentials(const String& ssid, const String& password) {
  DeviceSettings updated = settingsManager_.getSettings();
  updated.wifiSsid = ssid;
  updated.wifiPassword = password;
  const bool changed = settingsManager_.saveSettings(updated);
  if (changed) {
    restartNetwork();
  }
  return changed;
}

String WifiManagerService::scanNetworksJson() {
  DynamicJsonDocument doc(2048);
  JsonArray networks = doc.to<JsonArray>();
  const int count = WiFi.scanNetworks(false, true);
  for (int index = 0; index < count; ++index) {
    JsonObject entry = networks.createNestedObject();
    entry["ssid"] = WiFi.SSID(index);
    entry["rssi"] = WiFi.RSSI(index);
    entry["secure"] = WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  String json;
  serializeJson(doc, json);
  return json;
}

void WifiManagerService::restartNetwork() { begin(); }

String WifiManagerService::wifiMode() const {
  if (!staEnabled_) {
    return "AP";
  }
  return "AP+STA";
}

bool WifiManagerService::staConnected() const { return WiFi.status() == WL_CONNECTED; }

String WifiManagerService::stationIp() const {
  return staConnected() ? WiFi.localIP().toString() : "";
}

String WifiManagerService::accessPointIp() const { return WiFi.softAPIP().toString(); }

String WifiManagerService::connectedSsid() const {
  return staConnected() ? WiFi.SSID() : "";
}

String WifiManagerService::stationUrl() const {
  const String ip = stationIp();
  return ip.isEmpty() ? "" : String("http://") + ip;
}

String WifiManagerService::accessPointUrl() const {
  const String ip = accessPointIp();
  return ip.isEmpty() ? "" : String("http://") + ip;
}

String WifiManagerService::mdnsUrl() const {
  return String("http://") + ProductConfig::kMdnsHostLabel;
}

String WifiManagerService::mdnsHostname() const {
  return String(ProductConfig::kMdnsHostLabel);
}

const String& WifiManagerService::accessPointSsid() const { return apSsid_; }

const String& WifiManagerService::bleName() const { return bleName_; }

String WifiManagerService::suffixFromMac_() {
  const uint64_t mac = ESP.getEfuseMac();
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X", static_cast<unsigned int>(mac & 0xFFFFU));
  return String(suffix);
}

void WifiManagerService::startMdns_() {
  MDNS.end();
  if (MDNS.begin(ProductConfig::kMdnsHostname)) {
    MDNS.addService("http", "tcp", ProductConfig::kHttpPort);
  }
}

void WifiManagerService::applyThermalRadioConfig_() {
  WiFi.setSleep(ThermalConfig::kEnableWifiModemSleep);
  WiFi.setTxPower(ThermalConfig::kWifiTxPower);
}

void WifiManagerService::beginStationIfConfigured_() {
  const DeviceSettings& settings = settingsManager_.getSettings();
  const bool hasStation = !settings.wifiSsid.isEmpty();
  staEnabled_ = hasStation;

  WiFi.mode(hasStation ? WIFI_AP_STA : WIFI_AP);
  applyThermalRadioConfig_();
  WiFi.softAP(apSsid_.c_str(), ProductConfig::kDefaultApPassword);

  if (!hasStation) {
    return;
  }

  lastReconnectAttemptMs_ = millis();
  WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPassword.c_str());
}
