import type { TankGuardAlarmSettings } from "../../../shared/tankGuard.settings";

export interface AlarmSectionProps {
  value: TankGuardAlarmSettings;
  onNumberChange: (field: keyof TankGuardAlarmSettings, value: number) => void;
  onBooleanChange: (field: keyof TankGuardAlarmSettings, value: boolean) => void;
}

export function AlarmSection({
  value,
  onNumberChange,
  onBooleanChange
}: AlarmSectionProps) {
  return (
    <section className="panel">
      <h3 style={{ marginTop: 0 }}>Alarm Settings</h3>
      <div style={{ display: "grid", gap: 12 }}>
        <label style={{ display: "flex", justifyContent: "space-between", gap: 12 }}>
          <span>Repeat Alerts</span>
          <input
            type="checkbox"
            checked={value.repeatEnabled}
            onChange={(event) => onBooleanChange("repeatEnabled", event.target.checked)}
          />
        </label>
        <label style={{ display: "grid", gap: 6 }}>
          <span>Repeat Minutes</span>
          <input
            type="number"
            value={value.repeatMinutes}
            onChange={(event) => onNumberChange("repeatMinutes", Number(event.target.value))}
          />
        </label>
        <label style={{ display: "grid", gap: 6 }}>
          <span>Low Level Alarm (%)</span>
          <input
            type="number"
            value={value.lowLevelPct}
            onChange={(event) => onNumberChange("lowLevelPct", Number(event.target.value))}
          />
        </label>
        <label style={{ display: "flex", justifyContent: "space-between", gap: 12 }}>
          <span>Notify On Sensor Fault</span>
          <input
            type="checkbox"
            checked={value.notifyOnSensorFault}
            onChange={(event) =>
              onBooleanChange("notifyOnSensorFault", event.target.checked)
            }
          />
        </label>
      </div>
    </section>
  );
}
