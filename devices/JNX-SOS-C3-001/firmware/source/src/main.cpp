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
#include "services/SpeakerProfiles.h"
#include "services/SettingsManager.h"
#include "services/TelemetryService.h"
#include "services/ToneEngine.h"
#include "services/ToneProfiles.h"
#include "services/VtTriggerService.h"
#include "services/WebServerService.h"
#include "services/WifiManagerService.h"

static DeviceState snapshotState();
static void stopSirenForSystemReason(const char* reason);

namespace {
enum class VtTriggerAction : uint8_t {
  IGNORED_DISABLED = 0,
  STARTED = 1,
  RESTARTED = 2,
  EXTENDED = 3,
  TOGGLE_ON = 4,
  TOGGLE_OFF = 5,
  INCHING_ON = 6,
  INCHING_OFF = 7,
  FAILED = 8,
};

SettingsManager gSettingsManager;
ToneEngine gToneEngine;
ButtonService gButtonService;
VtTriggerService gVtTriggerService;
IndicatorService gIndicatorService;
WifiManagerService gWifiManager(gSettingsManager);
OtaService gOtaService(gSettingsManager, stopSirenForSystemReason);
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
bool gVtToggleLatched = false;
bool gVtInchingActive = false;
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

static void stopSirenForSystemReason(const char* reason) { gToneEngine.stopWithReason(reason); }

static bool timedSosRunning() {
  return gToneEngine.isActive() && gToneEngine.isSosOverride() && gToneEngine.timedCommandActive();
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
    case VtTriggerAction::TOGGLE_ON:
      return "toggle_on";
    case VtTriggerAction::TOGGLE_OFF:
      return "toggle_off";
    case VtTriggerAction::INCHING_ON:
      return "inching_on";
    case VtTriggerAction::INCHING_OFF:
      return "inching_off";
    case VtTriggerAction::FAILED:
    default:
      return "failed";
  }
}

static uint32_t vtMaintainWindowMs(const DeviceSettings& settings) {
  return static_cast<uint32_t>(max<uint16_t>(settings.vtTriggerDurationSec, 10)) * 1000UL;
}

static void clearVtLatchFlags() {
  gVtToggleLatched = false;
  gVtInchingActive = false;
}

static void publishVtSosEvent(const char* eventName) {
  const DeviceSettings& settings = gSettingsManager.getSettings();
  if (!settings.vtCloudNotify) {
    return;
  }

  gTelemetry.publishSosEvent(eventName, gSosPressCount, settings.vtTriggerDurationSec,
                             settings.selectedProfileId,
                             SettingsManager::retriggerModeToString(settings.vtRetriggerMode));
}

static bool startVtSosSession(bool publishStart = true) {
  const DeviceSettings& settings = gSettingsManager.getSettings();
  if (!gToneEngine.startCommandForDuration(settings.selectedProfileId, settings,
                                           settings.vtTriggerDurationSec, true)) {
    return false;
  }

  if (publishStart || gSosPressCount == 0) {
    gSosPressCount = 1;
  }
  gVtSosSessionActive = true;
  if (publishStart) {
    publishVtSosEvent("vt_sos_start");
  }
  return true;
}

static void maintainVtSosSession(uint32_t nowMs) {
  const DeviceSettings& settings = gSettingsManager.getSettings();
  const uint32_t keepAliveMs = vtMaintainWindowMs(settings);
  if (!timedSosRunning()) {
    startVtSosSession(false);
    return;
  }
  if (gToneEngine.remainingMs(nowMs, settings) <= min<uint32_t>(5000UL, keepAliveMs / 3U)) {
    gToneEngine.restartTimedCommand(settings.vtTriggerDurationSec);
  }
}

static VtTriggerAction handleVtTriggerRise() {
  const DeviceSettings& settings = gSettingsManager.getSettings();
  if (!settings.vtTriggerEnabled) {
    return VtTriggerAction::IGNORED_DISABLED;
  }

  if (settings.vtTriggerMode == VtTriggerMode::TOGGLE) {
    if (gVtToggleLatched) {
      ++gSosPressCount;
      clearVtLatchFlags();
      if (gVtSosSessionActive) {
        gToneEngine.stopWithReason("VT_TOGGLE_OFF");
      }
      return VtTriggerAction::TOGGLE_OFF;
    }
    if (!startVtSosSession()) {
      return VtTriggerAction::FAILED;
    }
    gVtToggleLatched = true;
    gVtInchingActive = false;
    return VtTriggerAction::TOGGLE_ON;
  }

  if (settings.vtTriggerMode == VtTriggerMode::INCHING) {
    if (timedSosRunning()) {
      return VtTriggerAction::INCHING_ON;
    }
    if (!startVtSosSession()) {
      return VtTriggerAction::FAILED;
    }
    gVtInchingActive = true;
    gVtToggleLatched = false;
    return VtTriggerAction::INCHING_ON;
  }

  if (!timedSosRunning()) {
    clearVtLatchFlags();
    return startVtSosSession() ? VtTriggerAction::STARTED : VtTriggerAction::FAILED;
  }

  ++gSosPressCount;
  const bool updated =
      settings.vtRetriggerMode == SosRetriggerMode::RESTART
          ? gToneEngine.restartTimedCommand(settings.vtTriggerDurationSec)
          : gToneEngine.extendTimedCommand(settings.vtTriggerDurationSec);
  if (updated) {
    publishVtSosEvent("vt_sos_repeat");
    return settings.vtRetriggerMode == SosRetriggerMode::RESTART
               ? VtTriggerAction::RESTARTED
               : VtTriggerAction::EXTENDED;
  }

  return VtTriggerAction::FAILED;
}

static VtTriggerAction handleVtTriggerFall() {
  const DeviceSettings& settings = gSettingsManager.getSettings();
  if (!settings.vtTriggerEnabled) {
    return VtTriggerAction::IGNORED_DISABLED;
  }
  if (settings.vtTriggerMode != VtTriggerMode::INCHING || !gVtInchingActive) {
    return VtTriggerAction::FAILED;
  }
  clearVtLatchFlags();
  if (gVtSosSessionActive) {
    gToneEngine.stopWithReason("VT_INCHING_RELEASE");
  }
  return VtTriggerAction::INCHING_OFF;
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
  const SpeakerProfile* speaker = SpeakerProfiles::findById(settings.speakerProfileId);

  DeviceState state;
  state.sirenState = gToneEngine.state();
  state.selectedProfileId = settings.selectedProfileId;
  state.selectedProfileName = selected ? selected->name : "Unknown";
  state.activeProfileName = active ? active->name : "";
  state.speakerProfileName = speaker ? speaker->name : "";
  state.wifiMode = gWifiManager.wifiMode();
  state.ipAddress = gWifiManager.stationIp();
  state.apIpAddress = gWifiManager.accessPointIp();
  state.connectedSsid = gWifiManager.connectedSsid();
  state.activeFrequencyHz = gToneEngine.activeFrequencyHz();
  state.activeDutyPercent = gToneEngine.activeDutyPercent();
  state.remainingMs = gToneEngine.remainingMs(millis(), settings);
  state.elapsedOnMs = gToneEngine.elapsedOnMs(millis());
  state.coolingRemainingMs = gToneEngine.coolingRemainingMs(millis());
  state.uptimeSec = millis() / 1000UL;
  state.buttonPressed = gButtonService.isPressed();
  state.vtTriggerHigh = gVtTriggerService.isHigh();
  state.vtTriggerSeen = gVtTriggerSeen;
  state.vtControlLatched = gVtToggleLatched || gVtInchingActive;
  state.commandActive = gToneEngine.commandActive();
  state.testMode = gToneEngine.isTestMode();
  state.sosActive = gToneEngine.isSosOverride();
  state.staConnected = gWifiManager.staConnected();
  state.vtLastTriggerUptimeSec = gLastVtTriggerUptimeSec;
  state.sosPressCount = gSosPressCount;
  state.sosDurationSec = settings.vtTriggerDurationSec;
  state.sosTriggerProfileId = settings.selectedProfileId;
  state.vtTriggerMode = SettingsManager::vtTriggerModeToString(settings.vtTriggerMode);
  state.sosRetriggerMode = SettingsManager::retriggerModeToString(settings.vtRetriggerMode);
  state.sosCloudNotify = settings.vtCloudNotify;
  state.lastStopReason = gToneEngine.lastStopReason();
  return state;
}

static void handleButtonEvent(ButtonEvent event) {
  switch (event) {
    case ButtonEvent::SHORT_PRESS:
      gToneEngine.toggleSelected(gSettingsManager.getSettings());
      gTelemetry.publishButtonEvent("short_press");
      break;
    case ButtonEvent::LONG_PRESS:
      gToneEngine.startCommand(gSettingsManager.getSettings().selectedProfileId,
                               gSettingsManager.getSettings(), true);
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
  Serial.printf("[SIREN] profile=%s\n", SpeakerProfiles::toString(gSettingsManager.getSettings().speakerProfileId));
  Serial.printf("[VT] gpio=%u mode=%s duration=%us retrigger=%s\n",
                static_cast<unsigned>(PinConfig::kVtTriggerGpio),
                SettingsManager::vtTriggerModeToString(
                    gSettingsManager.getSettings().vtTriggerMode),
                static_cast<unsigned>(gSettingsManager.getSettings().vtTriggerDurationSec),
                SettingsManager::retriggerModeToString(
                    gSettingsManager.getSettings().vtRetriggerMode));
  if (ThermalConfig::kEmitBootThermalSummary) {
    Serial.println("Thermal config: Wi-Fi modem sleep enabled, TX power reduced.");
  }
}

void loop() {
  const uint32_t nowMs = millis();
  const DeviceSettings& settings = gSettingsManager.getSettings();
  gButtonService.update(nowMs);
  gVtTriggerService.update(nowMs);
  handleButtonEvent(gButtonService.consumeEvent());
  if (gVtTriggerService.consumeRisingEdge()) {
    gVtTriggerSeen = true;
    gLastVtTriggerUptimeSec = nowMs / 1000UL;
    const VtTriggerAction action = handleVtTriggerRise();
    Serial.printf("[VT] gpio=%u edge=rise action=%s enabled=%d count=%u profile=%u duration=%us mode=%s high=%d uptime=%lus\n",
                  static_cast<unsigned>(PinConfig::kVtTriggerGpio),
                  vtTriggerActionToString(action), settings.vtTriggerEnabled ? 1 : 0,
                  static_cast<unsigned>(gSosPressCount),
                  static_cast<unsigned>(settings.selectedProfileId),
                  static_cast<unsigned>(settings.vtTriggerDurationSec),
                  SettingsManager::vtTriggerModeToString(settings.vtTriggerMode),
                  gVtTriggerService.isHigh() ? 1 : 0,
                  static_cast<unsigned long>(gLastVtTriggerUptimeSec));
  }
  if (gVtTriggerService.consumeFallingEdge()) {
    const VtTriggerAction action = handleVtTriggerFall();
    Serial.printf("[VT] gpio=%u edge=fall action=%s enabled=%d mode=%s high=%d uptime=%lus\n",
                  static_cast<unsigned>(PinConfig::kVtTriggerGpio),
                  vtTriggerActionToString(action), settings.vtTriggerEnabled ? 1 : 0,
                  SettingsManager::vtTriggerModeToString(settings.vtTriggerMode),
                  gVtTriggerService.isHigh() ? 1 : 0,
                  static_cast<unsigned long>(nowMs / 1000UL));
  }

  if (!settings.vtTriggerEnabled) {
    clearVtLatchFlags();
    if (gVtSosSessionActive) {
      gToneEngine.stopWithReason("VT_DISABLED");
    }
  } else if (settings.vtTriggerMode == VtTriggerMode::TOGGLE && gVtToggleLatched) {
    maintainVtSosSession(nowMs);
  } else if (settings.vtTriggerMode == VtTriggerMode::INCHING) {
    gVtInchingActive = gVtTriggerService.isHigh();
    if (gVtInchingActive) {
      maintainVtSosSession(nowMs);
    }
  } else {
    clearVtLatchFlags();
  }

  gToneEngine.update(nowMs, settings);
  syncVtSosSession();
  gIndicatorService.update(nowMs, gToneEngine.outputActive(),
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
