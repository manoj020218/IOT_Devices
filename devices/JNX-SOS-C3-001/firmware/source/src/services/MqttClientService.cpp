#include "MqttClientService.h"

void MqttClientService::begin() {}

void MqttClientService::update() {}

void MqttClientService::publishState(const DeviceState&) {}

void MqttClientService::publishButtonEvent(const char*) {}

void MqttClientService::publishSosEvent(const char*, uint16_t, uint16_t, uint8_t,
                                        const char*) {}

bool MqttClientService::enabled() const { return ENABLE_MQTT != 0; }
