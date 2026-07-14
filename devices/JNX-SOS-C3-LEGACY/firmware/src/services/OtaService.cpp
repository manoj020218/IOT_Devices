#include "OtaService.h"

#include <Update.h>

OtaService::OtaService(SettingsManager& settingsManager) : settingsManager_(settingsManager) {}

void OtaService::registerRoutes(WebServer& server) {
  server.on(
      "/update", HTTP_POST,
      [this, &server]() {
        if (!uploadAuthorized_) {
          server.send(401, "application/json",
                      "{\"ok\":false,\"error\":\"Unauthorized OTA password\"}");
          return;
        }

        const bool ok = !Update.hasError();
        server.send(ok ? 200 : 500, "application/json",
                    ok ? "{\"ok\":true,\"message\":\"OTA upload successful. Rebooting.\"}"
                       : "{\"ok\":false,\"error\":\"OTA upload failed\"}");
        delay(250);
        if (ok) {
          ESP.restart();
        }
      },
      [this, &server]() { handleUpload_(server); });
}

bool OtaService::isAuthorized_(WebServer& server) const {
  const String provided = server.header("X-Admin-Password");
  return provided == settingsManager_.getSettings().otaAdminPassword;
}

void OtaService::handleUpload_(WebServer& server) {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    uploadAuthorized_ = isAuthorized_(server);
    if (!uploadAuthorized_) {
      return;
    }

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
    return;
  }

  if (!uploadAuthorized_) {
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    if (!Update.end(true)) {
      Update.printError(Serial);
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    uploadAuthorized_ = false;
  }
}
