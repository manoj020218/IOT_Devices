import type { TankGuardSettingsDraft } from "../../shared/tankGuard.settings";
import { ActionSection } from "./sections/ActionSection";
import { AlarmSection } from "./sections/AlarmSection";
import { ConfigSection } from "./sections/ConfigSection";

export interface TankGuardSettingsSheetProps {
  open: boolean;
  saving?: boolean;
  value: TankGuardSettingsDraft;
  onClose: () => void;
  onConfigChange: (field: string, value: number) => void;
  onAlarmNumberChange: (field: string, value: number) => void;
  onAlarmBooleanChange: (field: string, value: boolean) => void;
  onActionChange: (field: string, value: number) => void;
  onSave: () => void;
}

export function TankGuardSettingsSheet({
  open,
  saving,
  value,
  onClose,
  onConfigChange,
  onAlarmNumberChange,
  onAlarmBooleanChange,
  onActionChange,
  onSave
}: TankGuardSettingsSheetProps) {
  if (!open) return null;

  return (
    <>
      <div
        className="jx-scrim"
        onClick={onClose}
        style={{ position: "fixed", inset: 0 }}
      />
      <aside
        className="jx-sheet"
        role="dialog"
        aria-label="Tank Guard settings"
        style={{ maxWidth: 560, width: "100%", right: 0 }}
      >
        <div className="jx-sh">
          <div>
            <h3 style={{ margin: 0 }}>Detail Settings</h3>
            <p style={{ margin: "6px 0 0" }}>
              Save sends one device command. Firmware should ack, then persist values into NVS.
            </p>
          </div>
          <button className="jx-close" type="button" onClick={onClose} aria-label="Close">
            x
          </button>
        </div>

        <div className="jx-sb" style={{ display: "grid", gap: 12 }}>
          <ConfigSection
            value={value.config}
            onChange={(field, next) => onConfigChange(field, next)}
          />
          <AlarmSection
            value={value.alarm}
            onNumberChange={(field, next) => onAlarmNumberChange(field, next)}
            onBooleanChange={(field, next) => onAlarmBooleanChange(field, next)}
          />
          <ActionSection
            value={value.actions}
            onChange={(field, next) => onActionChange(field, next)}
          />
          <div style={{ display: "flex", gap: 10 }}>
            <button className="jx-btn ghost" type="button" onClick={onClose}>
              Cancel
            </button>
            <button className="jx-btn" type="button" onClick={onSave} disabled={saving}>
              {saving ? "Saving..." : "Save To Device"}
            </button>
          </div>
        </div>
      </aside>
    </>
  );
}
