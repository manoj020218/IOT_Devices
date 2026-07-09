import type { PlatformDeviceSummary } from "../../shared/plugin.types";
import type { TankGuardSettingsDraft } from "../../shared/tankGuard.settings";
import type { TankGuardSnapshot } from "../../shared/tankGuard.telemetry";
import {
  tankGuardLevelTone,
  tankGuardSignalTone,
  tankGuardStatusLabel
} from "../tankGuard.theme";

export interface TankGuardPageModel {
  online: boolean;
  statusLabel: string;
  levelTone: string;
  signalTone: string;
  tankHeightMm: number;
  waterLitres: number;
  capacityLitres: number;
  usablePct: number;
  waterText: string;
  heroHint: string;
}

function isOnline(device: PlatformDeviceSummary): boolean {
  return device.cloudStatus === "online" || device.mqttStatus === "online";
}

export function buildTankGuardPageModel(
  device: PlatformDeviceSummary,
  snapshot: TankGuardSnapshot,
  settings: TankGuardSettingsDraft
): TankGuardPageModel {
  const online = isOnline(device);
  const tankHeightMm = Math.max(
    1,
    snapshot.topLevelMm - Math.max(snapshot.zeroLevelMm, snapshot.bottomLevelMm)
  );
  const usablePct = Math.max(0, Math.min(100, snapshot.levelPct));
  const capacityLitres = Math.max(0, settings.config.capacityLitres);
  const waterLitres = Math.round((usablePct / 100) * capacityLitres);
  const heroHint = snapshot.pumpRunning
    ? "Pump input is active. Flow is estimated from recent level change."
    : "Pump input is idle. Flow will settle to zero when the level is stable.";

  return {
    online,
    statusLabel: tankGuardStatusLabel(online, snapshot.alarmState),
    levelTone: tankGuardLevelTone(usablePct),
    signalTone: tankGuardSignalTone(snapshot.rssiDbm),
    tankHeightMm,
    waterLitres,
    capacityLitres,
    usablePct,
    waterText: `${waterLitres.toLocaleString()} / ${capacityLitres.toLocaleString()} L`,
    heroHint
  };
}
