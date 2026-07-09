export type PluginConnectivity = "online" | "offline" | "unknown";

export interface PlatformDeviceSummary {
  deviceId: string;
  pid: string;
  displayName: string;
  firmwareVersion?: string;
  hardwareRevision?: string;
  mqttStatus: PluginConnectivity;
  cloudStatus: PluginConnectivity;
  localStatus?: "available" | "unavailable" | "unknown";
  lastSeenAt?: string;
}

export interface DevicePluginTelemetryBinding {
  key: string;
  label: string;
  unit?: string;
  required?: boolean;
}

export interface DevicePluginCommandBinding {
  name: string;
  label: string;
  requiresAck: boolean;
  persistsOnDevice?: boolean;
}

export interface DevicePluginSceneBinding {
  telemetryTriggers: string[];
  actionCommands: string[];
}

export interface DevicePluginPageBinding {
  templateId: string;
  dynamicPageId: string;
  homeTileOpensDevicePage: boolean;
}

export interface DevicePluginManifest {
  pluginId: string;
  pid: string;
  productName: string;
  category: string;
  page: DevicePluginPageBinding;
  telemetry: DevicePluginTelemetryBinding[];
  commands: DevicePluginCommandBinding[];
  scenes: DevicePluginSceneBinding;
}

export interface DeviceTelemetrySnapshotRecord {
  deviceId: string;
  occurredAt: string;
  telemetry: Record<string, boolean | number | string>;
  history?: number[];
}

export interface DeviceCommandAckRecord {
  commandId: string;
  deviceId: string;
  status: "queued" | "completed" | "failed";
  acknowledgedAt?: string;
  errorMessage?: string;
}
