import { useEffect, useState } from "react";

import type { PlatformDeviceSummary } from "../shared/plugin.types";
import type { TankGuardSettingsDraft } from "../shared/tankGuard.settings";
import type { TankGuardSnapshot } from "../shared/tankGuard.telemetry";
import { TankGuardQuickActions } from "./components/TankGuardQuickActions";
import { TankGuardSettingsSheet } from "./components/TankGuardSettingsSheet";
import { TankGuardStatBar } from "./components/TankGuardStatBar";
import { TankHero } from "./components/TankHero";
import { buildTankGuardPageModel } from "./services/tankGuardViewModel";

export interface TankGuardDevicePageProps {
  device: PlatformDeviceSummary;
  snapshot: TankGuardSnapshot;
  settings: TankGuardSettingsDraft;
  busy?: boolean;
  saving?: boolean;
  onRefresh: () => void;
  onZeroCalibrate: () => void;
  onSaveSettings: (next: TankGuardSettingsDraft) => void;
}

export function TankGuardDevicePage({
  device,
  snapshot,
  settings,
  busy,
  saving,
  onRefresh,
  onZeroCalibrate,
  onSaveSettings
}: TankGuardDevicePageProps) {
  const [open, setOpen] = useState(false);
  const [draft, setDraft] = useState(settings);

  useEffect(() => {
    setDraft(settings);
  }, [settings]);

  const model = buildTankGuardPageModel(device, snapshot, draft);

  return (
    <>
      <div style={{ display: "grid", gap: 14 }}>
        <TankHero device={device} snapshot={snapshot} model={model} />
        <TankGuardStatBar snapshot={snapshot} model={model} />
        <section className="panel">
          <div className="scene-section-head">
            <div>
              <span className="eyebrow">Calibration</span>
              <h2 style={{ marginBottom: 4 }}>Zero and Capacity Controls</h2>
              <p className="hint-text">
                Zero level is {snapshot.zeroLevelMm} mm. Full capacity is {draft.config.capacityLitres} L.
              </p>
            </div>
          </div>
        </section>
        <TankGuardQuickActions
          busy={busy}
          onRefresh={onRefresh}
          onZeroCalibrate={onZeroCalibrate}
          onOpenSettings={() => setOpen(true)}
        />
      </div>

      <TankGuardSettingsSheet
        open={open}
        saving={saving}
        value={draft}
        onClose={() => setOpen(false)}
        onConfigChange={(field, value) =>
          setDraft((current) => ({
            ...current,
            config: { ...current.config, [field]: value }
          }))
        }
        onAlarmNumberChange={(field, value) =>
          setDraft((current) => ({
            ...current,
            alarm: { ...current.alarm, [field]: value }
          }))
        }
        onAlarmBooleanChange={(field, value) =>
          setDraft((current) => ({
            ...current,
            alarm: { ...current.alarm, [field]: value }
          }))
        }
        onActionChange={(field, value) =>
          setDraft((current) => ({
            ...current,
            actions: { ...current.actions, [field]: value }
          }))
        }
        onSave={() => {
          onSaveSettings(draft);
          setOpen(false);
        }}
      />
    </>
  );
}
