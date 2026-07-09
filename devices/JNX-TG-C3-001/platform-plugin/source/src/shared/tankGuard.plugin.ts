import type { DevicePluginManifest } from "./plugin.types";

export const tankGuardUiBinding = {
  uiMode: "remote-package",
  uiPackageId: "tank-guard-mobile",
  uiPackageVersion: "1.0.0"
} as const;

export const tankGuardPluginManifest: DevicePluginManifest = {
  pluginId: "tank-guard-mobile",
  pid: "JNX-TG-C3-001",
  productName: "Smart Tank Guard by Jenix",
  category: "Water Monitoring",
  page: {
    templateId: "tank-guard-mobile-v1",
    dynamicPageId: "tank-guard-mobile",
    homeTileOpensDevicePage: true
  },
  telemetry: [
    { key: "tankLevelPct", label: "Level", unit: "%", required: true },
    { key: "tankLevelMm", label: "Water Level", unit: "mm", required: true },
    { key: "wifiRssi", label: "WiFi RSSI", unit: "dBm", required: true },
    { key: "localUrl", label: "Local URL", required: false },
    { key: "pumpRunning", label: "Pump Running", required: true },
    { key: "alarmState", label: "Alarm State", required: true },
    { key: "sensorStatus", label: "Sensor Status", required: true }
  ],
  commands: [
    { name: "refresh", label: "Refresh", requiresAck: true },
    {
      name: "zero_calibrate",
      label: "Zero Calibrate",
      requiresAck: true,
      persistsOnDevice: true
    },
    {
      name: "apply_settings",
      label: "Apply Settings",
      requiresAck: true,
      persistsOnDevice: true
    },
    { name: "motor_on", label: "Motor ON", requiresAck: true },
    { name: "motor_off", label: "Motor OFF", requiresAck: true },
    { name: "alarm_test", label: "Alarm Test", requiresAck: true }
  ],
  scenes: {
    telemetryTriggers: ["tankLevelPct", "pumpRunning", "alarmState", "sensorStatus"],
    actionCommands: ["motor_on", "motor_off", "alarm_test", "apply_settings"]
  }
};
