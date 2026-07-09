import type { DeviceTelemetrySnapshotRecord } from "./plugin.types";

export interface TankGuardSnapshot {
  levelPct: number;
  waterLevelMm: number;
  zeroLevelMm: number;
  topLevelMm: number;
  bottomLevelMm: number;
  flowLitresPerMin: number;
  pumpRunning: boolean;
  alarmState: string;
  sensorStatus: string;
  rssiDbm: number;
  wifiSsid: string;
  localIp: string;
  localUrl: string;
  wifiTxPowerDbm: number;
  historyPct: number[];
  occurredAt: string;
}

export const tankGuardTelemetryFields = [
  "tankLevelPct",
  "tankLevelMm",
  "zeroLevelMm",
  "topLevelMm",
  "bottomLevelMm",
  "pumpRunning",
  "alarmState",
  "sensorStatus",
  "wifiRssi",
  "wifiSsidName",
  "localIp",
  "localUrl",
  "wifiTxPowerDbm"
] as const;

function readNumber(value: unknown, fallback = 0): number {
  return typeof value === "number" && Number.isFinite(value) ? value : fallback;
}

function readString(value: unknown, fallback = ""): string {
  return typeof value === "string" ? value : fallback;
}

function readBoolean(value: unknown, fallback = false): boolean {
  return typeof value === "boolean" ? value : fallback;
}

export function toTankGuardSnapshot(
  record: DeviceTelemetrySnapshotRecord,
  flowLitresPerMin = 0
): TankGuardSnapshot {
  const telemetry = record.telemetry;
  return {
    levelPct: readNumber(telemetry.tankLevelPct),
    waterLevelMm: readNumber(telemetry.tankLevelMm),
    zeroLevelMm: readNumber(telemetry.zeroLevelMm),
    topLevelMm: readNumber(telemetry.topLevelMm),
    bottomLevelMm: readNumber(telemetry.bottomLevelMm),
    flowLitresPerMin,
    pumpRunning: readBoolean(telemetry.pumpRunning),
    alarmState: readString(telemetry.alarmState, "normal"),
    sensorStatus: readString(telemetry.sensorStatus, "unknown"),
    rssiDbm: readNumber(telemetry.wifiRssi, readNumber(telemetry.signalStrength, -127)),
    wifiSsid: readString(telemetry.wifiSsidName),
    localIp: readString(telemetry.localIp),
    localUrl: readString(telemetry.localUrl),
    wifiTxPowerDbm: readNumber(telemetry.wifiTxPowerDbm, 8.5),
    historyPct: record.history ?? [readNumber(telemetry.tankLevelPct)],
    occurredAt: record.occurredAt
  };
}
