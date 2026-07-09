import type { DeviceTelemetrySnapshotRecord } from "../shared/plugin.types";
import { toTankGuardSnapshot, type TankGuardSnapshot } from "../shared/tankGuard.telemetry";

export interface TankGuardTelemetryStateRecord {
  deviceId: string;
  latest: DeviceTelemetrySnapshotRecord;
  samples: DeviceTelemetrySnapshotRecord[];
}

export interface TankGuardTelemetryRepository {
  get(deviceId: string): Promise<TankGuardTelemetryStateRecord | undefined>;
  save(record: TankGuardTelemetryStateRecord): Promise<TankGuardTelemetryStateRecord>;
}

export function keepRecentSamples(
  samples: DeviceTelemetrySnapshotRecord[],
  incoming: DeviceTelemetrySnapshotRecord,
  maxItems = 30
): DeviceTelemetrySnapshotRecord[] {
  return [...samples, incoming].slice(-maxItems);
}

export function computeFlowLitresPerMin(
  settingsCapacityLitres: number,
  usefulHeightMm: number,
  samples: DeviceTelemetrySnapshotRecord[]
): number {
  if (samples.length < 2 || usefulHeightMm <= 0 || settingsCapacityLitres <= 0) {
    return 0;
  }

  const prev = samples[samples.length - 2];
  const next = samples[samples.length - 1];
  const prevLevel = Number(prev.telemetry.tankLevelMm ?? 0);
  const nextLevel = Number(next.telemetry.tankLevelMm ?? 0);
  const elapsedMs =
    new Date(next.occurredAt).getTime() - new Date(prev.occurredAt).getTime();
  if (elapsedMs <= 0) {
    return 0;
  }

  const litresPerMm = settingsCapacityLitres / usefulHeightMm;
  const deltaLitres = (nextLevel - prevLevel) * litresPerMm;
  return Number(((deltaLitres / elapsedMs) * 60000).toFixed(2));
}

export function selectTankGuardSnapshot(
  state: TankGuardTelemetryStateRecord,
  capacityLitres: number,
  usefulHeightMm: number
): TankGuardSnapshot {
  const flowLitresPerMin = computeFlowLitresPerMin(
    capacityLitres,
    usefulHeightMm,
    state.samples
  );
  return toTankGuardSnapshot(
    {
      ...state.latest,
      history: state.samples.map((sample) => Number(sample.telemetry.tankLevelPct ?? 0))
    },
    flowLitresPerMin
  );
}
