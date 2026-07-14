#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>

#include "ButtonService.h"
#include "OtaService.h"
#include "SettingsManager.h"
#include "ToneEngine.h"
#include "WifiManagerService.h"

class WebServerService {
 public:
  using StatusProvider = DeviceState (*)();

  WebServerService(SettingsManager& settingsManager, ToneEngine& toneEngine,
                   WifiManagerService& wifiManager, ButtonService& buttonService,
                   OtaService& otaService, StatusProvider statusProvider);

  void begin();
  void handleClient();

 private:
  SettingsManager& settingsManager_;
  ToneEngine& toneEngine_;
  WifiManagerService& wifiManager_;
  ButtonService& buttonService_;
  OtaService& otaService_;
  StatusProvider statusProvider_;
  WebServer server_{80};

  void registerRoutes_();
  bool parseJsonBody_(DynamicJsonDocument& doc);
  void sendJson_(int statusCode, const String& json);
  String buildStatusJson_() const;
  String buildProfilesJson_() const;
  String buildSettingsJson_() const;
  static String remainingLabel_(uint32_t remainingMs);
};
