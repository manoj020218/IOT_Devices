#pragma once

#include <Arduino.h>

class BleProvisioningService {
 public:
  explicit BleProvisioningService(String advertisedName);
  void begin();
  const String& advertisedName() const;

 private:
  String advertisedName_;
};
