#include "TelemetryService.h"

TelemetryService::TelemetryService(MqttClientService& mqttClient) : mqttClient_(mqttClient) {}

void TelemetryService::publishState(const DeviceState& deviceState) {
  mqttClient_.publishState(deviceState);
}

void TelemetryService::publishButtonEvent(const char* eventName) {
  mqttClient_.publishButtonEvent(eventName);
}

void TelemetryService::publishSosEvent(const char* eventName, uint16_t pressCount,
                                       uint16_t durationSec, uint8_t profileId,
                                       const char* retriggerMode) {
  mqttClient_.publishSosEvent(eventName, pressCount, durationSec, profileId, retriggerMode);
}
