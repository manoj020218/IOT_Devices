import type { PlatformDeviceSummary } from "../shared/plugin.types";
import type { TankGuardSettingsDraft } from "../shared/tankGuard.settings";
import type { TankGuardSnapshot } from "../shared/tankGuard.telemetry";
import { TankGuardDevicePage } from "../frontend/TankGuardDevicePage";

export interface TankGuardDynamicPageProps {
  device: PlatformDeviceSummary;
  snapshot: TankGuardSnapshot;
  settings: TankGuardSettingsDraft;
  onRefresh: () => void;
  onZeroCalibrate: () => void;
  onSaveSettings: (next: TankGuardSettingsDraft) => void;
}

export function TankGuardDynamicPage(props: TankGuardDynamicPageProps) {
  return <TankGuardDevicePage {...props} />;
}
