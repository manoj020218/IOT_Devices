#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "SettingsManager.h"

class WifiManagerService {
 public:
  explicit WifiManagerService(SettingsManager& settingsManager);

  void begin();
  void update(uint32_t nowMs);
  bool saveCredentials(const String& ssid, const String& password);
  String scanNetworksJson();
  void restartNetwork();

  String wifiMode() const;
  bool staConnected() const;
  String stationIp() const;
  String accessPointIp() const;
  String connectedSsid() const;
  String stationUrl() const;
  String accessPointUrl() const;
  String mdnsUrl() const;
  String mdnsHostname() const;
  const String& accessPointSsid() const;
  const String& bleName() const;

 private:
  SettingsManager& settingsManager_;
  String apSsid_;
  String bleName_;
  uint32_t lastReconnectAttemptMs_ = 0;
  bool staEnabled_ = false;

  static String suffixFromMac_();
  void applyThermalRadioConfig_();
  void startMdns_();
  void beginStationIfConfigured_();
};
