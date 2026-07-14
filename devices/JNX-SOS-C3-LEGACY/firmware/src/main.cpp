#include <Arduino.h>

#if __has_include(<esp_task_wdt.h>)
#include <esp_task_wdt.h>
#define JNX_HAS_WDT 1
#else
#define JNX_HAS_WDT 0
#endif

#if __has_include(<esp_idf_version.h>)
#include <esp_idf_version.h>
#endif

#include "config/PinConfig.h"
#include "config/ProductConfig.h"
#include "config/ThermalConfig.h"
#include "models/DeviceState.h"
#include "services/BleProvisioningService.h"
#include "services/ButtonService.h"
#include "services/IndicatorService.h"
#include "services/MqttClientService.h"
#include "services/OtaService.h"
#include "services/SettingsManager.h"
#include "services/TelemetryService.h"
#include "services/ToneEngine.h"
#include "services/ToneProfiles.h"
#include "services/VtTriggerService.h"
#include "services/WebServerService.h"
#include "services/WifiManagerService.h"

static DeviceState snapshotState();

namespace {
enum class VtTriggerAction : uint8_t {
  IGNORED_DISABLED = 0,
  STARTED = 1,
  RESTARTED = 2,
  EXTENDED = 3,
  FAILED = 4,
};

SettingsManager gSettingsManager;
ToneEngine gToneEngine;
ButtonService gButtonService;
VtTriggerService gVtTriggerService;
IndicatorService gIndicatorService;
WifiManagerService gWifiManager(gSettingsManager);
OtaService gOtaService(gSettingsManager);
WebServerService gWebServer(gSettingsManager, gToneEngine, gWifiManager, gButtonService,
                            gOtaService, snapshotState);
MqttClientService gMqttClient;
TelemetryService gTelemetry(gMqttClient);
BleProvisioningService* gBleProvisioning = nullptr;
uint32_t gLastTelemetryMs = 0;
uint32_t gLastVtTriggerUptimeSec = 0;
uint16_t gSosPressCount = 0;
bool gVtTriggerSeen = false;
bool gVtSosSessionActive = false;
}  // namespace

static void setupWatchdog() {
#if JNX_HAS_WDT
#if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 5
  const esp_task_wdt_config_t config = {
      .timeout_ms = 15000,
      .idle_core_mask = 0,
      .trigger_panic = true,
  };
  esp_task_wdt_init(&config);
#else
  esp_task_wdt_init(15, true);
#endif
  esp_task_wdt_add(nullptr);
#endif
}

static void feedWatchdog() {
#if JNX_HAS_WDT
  esp_task_wdt_reset();
#endif
}

static bool timedSosRunning() {
  return gToneEngine.isActive() && gToneEngine.isSosOverride() &&
         gToneEngine.timedCommandActive();
}

static const char* vtTriggerActionToString(VtTriggerAction action) {
  switch (action) {
    case VtTriggerAction::IGNORED_DISABLED:
      return "ignored_disabled";
    case VtTriggerAction::STARTED:
      return "started";
    case VtTriggerAction::RESTARTED:
      return "restarted";
    case VtTriggerAction::EXTENDED:
      return "extended";
    case VtTriggerAction::FAILED:
    default:
      return "failed";
  }
}

static void publishVtSosEvent(const char* eventName) {
  const DeviceSettings& settings = gSettingsManager.getSettings();
  if (!settings.vtCloudNotify) {
    return;
  }
  gTelemetry.publishSosEvent(eventName, gSosPressCount, settings.vtTriggerDurationSec,
                             settings.vtTriggerProfileId,
                             SettingsManager::retriggerModeToString(settings.vtRetriggerMode));
}

static bool startVtSosSession() {
  const DeviceSettings& settings = gSettingsManager.getSettings();
  if (!gToneEngine.startCommandForDuration(settings.vtTriggerProfileId, settings,
                                           settings.vtTriggerDurationSec, true)) {
    return false;
  }
  gSosPressCount = 1;
  gVtSosSessionActive = true;
  publishVtSosEvent("vt_sos_start");
  return true;
}

static VtTriggerAction handleVtTriggerEvent() {
  const DeviceSettings& settings = gSettingsManager.getSettings();
  if (!settings.vtTriggerEnabled) {
    return VtTriggerAction::IGNORED_DISABLED;
  }
  if (!timedSosRunning()) {
    return startVtSosSession() ? VtTriggerAction::STARTED : VtTriggerAction::FAILED;
  }

  ++gSosPressCount;
  const bool updated = settings.vtRetriggerMode == SosRetriggerMode::RESTART
                           ? gToneEngine.restartTimedCommand(settings.vtTriggerDurationSec)
                           : gToneEngine.extendTimedCommand(settings.vtTriggerDurationSec);
  if (!updated) {
    return VtTriggerAction::FAILED;
  }

  publishVtSosEvent("vt_sos_repeat");
  return settings.vtRetriggerMode == SosRetriggerMode::RESTART
             ? VtTriggerAction::RESTARTED
             : VtTriggerAction::EXTENDED;
}

static void syncVtSosSession() {
  if (!gVtSosSessionActive || timedSosRunning()) {
    return;
  }
  publishVtSosEvent("vt_sos_end");
  gVtSosSessionActive = false;
  gSosPressCount = 0;
}

static DeviceState snapshotState() {
  const DeviceSettings& settings = gSettingsManager.getSettings();
  const ToneProfile* selected = ToneProfiles::findById(settings.selectedProfileId);
  const ToneProfile* active = gToneEngine.activeProfile();
  DeviceState state;
  state.sirenState = gToneEngine.state();
  state.selectedProfileId = settings.selectedProfileId;
  state.selectedProfileName = selected ? selected->name : "Unknown";
  state.activeProfileName = active ? active->name : "";
  state.wifiMode = gWifiManager.wifiMode();
  state.ipAddress = gWifiManager.stationIp();
  state.apIpAddress = gWifiManager.accessPointIp();
  state.connectedSsid = gWifiManager.connectedSsid();
  state.activeFrequencyHz = gToneEngine.activeFrequencyHz();
  state.activeDutyPercent = gToneEngine.activeDutyPercent();
  state.remainingMs = gToneEngine.remainingMs(millis(), settings);
  state.uptimeSec = millis() / 1000UL;
  state.buttonPressed = gButtonService.isPressed();
  state.vtTriggerHigh = gVtTriggerService.isHigh();
  state.commandActive = gToneEngine.commandActive();
  state.testMode = gToneEngine.isTestMode();
  state.sosActive = gToneEngine.isSosOverride();
  state.staConnected = gWifiManager.staConnected();
  state.sosPressCount = gSosPressCount;
  state.sosDurationSec = settings.vtTriggerDurationSec;
  state.sosTriggerProfileId = settings.vtTriggerProfileId;
  state.sosRetriggerMode = SettingsManager::retriggerModeToString(settings.vtRetriggerMode);
  state.sosCloudNotify = settings.vtCloudNotify;
  return state;
}

static void handleButtonEvent(ButtonEvent event) {
  switch (event) {
    case ButtonEvent::SHORT_PRESS:
      gToneEngine.toggleSelected(gSettingsManager.getSettings());
      gTelemetry.publishButtonEvent("short_press");
      break;
    case ButtonEvent::LONG_PRESS:
      gToneEngine.startCommand(9, gSettingsManager.getSettings(), true);
      gTelemetry.publishButtonEvent("long_press");
      break;
    case ButtonEvent::VERY_LONG_PRESS:
      gTelemetry.publishButtonEvent("very_long_press");
      gToneEngine.stop();
      gSettingsManager.clearWifiCredentials();
      delay(150);
      ESP.restart();
      break;
    case ButtonEvent::NONE:
    default:
      break;
  }
}

void setup() {
  Serial.begin(ProductConfig::kSerialBaud);
  delay(200);
  Serial.println();
  Serial.printf("%s %s\n", ProductConfig::kProductName, ProductConfig::kFirmwareVersion);
  setupWatchdog();
  gSettingsManager.begin();
  gToneEngine.begin(PinConfig::kPwmOutGpio);
  gButtonService.begin(PinConfig::kButtonGpio);
  gVtTriggerService.begin(PinConfig::kVtTriggerGpio);
  gIndicatorService.begin(PinConfig::kStatusLedGpio, PinConfig::kStatusLedActiveLow);
  gWifiManager.begin();
  gBleProvisioning = new BleProvisioningService(gWifiManager.bleName());
  gBleProvisioning->begin();
  gMqttClient.begin();
  gWebServer.begin();
  Serial.printf("AP SSID: %s\n", gWifiManager.accessPointSsid().c_str());
  Serial.printf("AP IP: %s\n", gWifiManager.accessPointIp().c_str());
  Serial.printf("mDNS: %s\n", ProductConfig::kMdnsHostLabel);
  if (ThermalConfig::kEmitBootThermalSummary) {
    Serial.println("Thermal config: Wi-Fi modem sleep enabled, TX power reduced.");
  }
}

void loop() {
  const uint32_t nowMs = millis();
  gButtonService.update(nowMs);
  gVtTriggerService.update(nowMs);
  handleButtonEvent(gButtonService.consumeEvent());
  if (gVtTriggerService.consumeRisingEdge()) {
    gVtTriggerSeen = true;
    gLastVtTriggerUptimeSec = nowMs / 1000UL;
    const DeviceSettings& settings = gSettingsManager.getSettings();
    const VtTriggerAction action = handleVtTriggerEvent();
    Serial.printf("[VT] gpio=%u action=%s enabled=%d count=%u profile=%u duration=%us mode=%s uptime=%lus\n",
                  static_cast<unsigned>(PinConfig::kVtTriggerGpio),
                  vtTriggerActionToString(action), settings.vtTriggerEnabled ? 1 : 0,
                  static_cast<unsigned>(gSosPressCount),
                  static_cast<unsigned>(settings.vtTriggerProfileId),
                  static_cast<unsigned>(settings.vtTriggerDurationSec),
                  SettingsManager::retriggerModeToString(settings.vtRetriggerMode),
                  static_cast<unsigned long>(gLastVtTriggerUptimeSec));
  }
  gToneEngine.update(nowMs, gSettingsManager.getSettings());
  syncVtSosSession();
  gIndicatorService.update(nowMs,
                           gToneEngine.state() == SirenState::PLAYING &&
                               gToneEngine.isSosOverride(),
                           gButtonService.isPressed());
  gWifiManager.update(nowMs);
  gWebServer.handleClient();
  gMqttClient.update();
  if (nowMs - gLastTelemetryMs >= 5000UL) {
    gLastTelemetryMs = nowMs;
    gTelemetry.publishState(snapshotState());
  }
  feedWatchdog();
  delay(1);
}
