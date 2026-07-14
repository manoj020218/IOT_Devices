#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "SettingsManager.h"

class OtaService {
 public:
  explicit OtaService(SettingsManager& settingsManager);
  void registerRoutes(WebServer& server);

 private:
  SettingsManager& settingsManager_;
  bool uploadAuthorized_ = false;

  bool isAuthorized_(WebServer& server) const;
  void handleUpload_(WebServer& server);
};
