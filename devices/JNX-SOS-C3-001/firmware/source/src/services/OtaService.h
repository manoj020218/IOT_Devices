#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "SettingsManager.h"

class OtaService {
 public:
  using StopCallback = void (*)(const char* reason);

  OtaService(SettingsManager& settingsManager, StopCallback stopCallback);
  void registerRoutes(WebServer& server);

 private:
  SettingsManager& settingsManager_;
  StopCallback stopCallback_ = nullptr;
  bool uploadAuthorized_ = false;

  bool isAuthorized_(WebServer& server) const;
  void handleUpload_(WebServer& server);
};
