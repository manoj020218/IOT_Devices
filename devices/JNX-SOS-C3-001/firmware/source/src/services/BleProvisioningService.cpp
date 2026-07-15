#include "BleProvisioningService.h"

BleProvisioningService::BleProvisioningService(String advertisedName)
    : advertisedName_(advertisedName) {}

void BleProvisioningService::begin() {
  Serial.printf("BLE provisioning placeholder active: %s\n", advertisedName_.c_str());
}

const String& BleProvisioningService::advertisedName() const { return advertisedName_; }
