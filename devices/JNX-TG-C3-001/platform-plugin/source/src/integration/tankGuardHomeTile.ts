import type { PlatformDeviceSummary } from "../shared/plugin.types";
import type { TankGuardSettingsDraft } from "../shared/tankGuard.settings";
import type { TankGuardSnapshot } from "../shared/tankGuard.telemetry";

export interface TankGuardHomeTileModel {
  online: boolean;
  levelPct: number;
  tankMm: number;
  flowLitresPerMin: number;
  rssiDbm: number;
  pumpRunning: boolean;
  alert: boolean;
  history: number[];
  capacityLitres: number;
}

export function buildTankGuardHomeTileModel(
  device: PlatformDeviceSummary,
  snapshot: TankGuardSnapshot,
  settings: TankGuardSettingsDraft
): TankGuardHomeTileModel {
  return {
    online: device.cloudStatus === "online" || device.mqttStatus === "online",
    levelPct: snapshot.levelPct,
    tankMm: snapshot.waterLevelMm,
    flowLitresPerMin: snapshot.flowLitresPerMin,
    rssiDbm: snapshot.rssiDbm,
    pumpRunning: snapshot.pumpRunning,
    alert: snapshot.alarmState !== "normal" || snapshot.sensorStatus !== "ok",
    history: snapshot.historyPct,
    capacityLitres: settings.config.capacityLitres
  };
}
