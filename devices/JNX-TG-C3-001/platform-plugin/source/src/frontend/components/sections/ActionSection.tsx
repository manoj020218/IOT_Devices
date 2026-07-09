import type { TankGuardActionSettings } from "../../../shared/tankGuard.settings";

export interface ActionSectionProps {
  value: TankGuardActionSettings;
  onChange: (field: keyof TankGuardActionSettings, value: number) => void;
}

function renderNumber(
  label: string,
  value: number,
  onChange: (next: number) => void
) {
  return (
    <label style={{ display: "grid", gap: 6 }}>
      <span>{label}</span>
      <input
        type="number"
        value={value}
        onChange={(event) => onChange(Number(event.target.value))}
      />
    </label>
  );
}

export function ActionSection({ value, onChange }: ActionSectionProps) {
  return (
    <section className="panel">
      <h3 style={{ marginTop: 0 }}>Action Settings</h3>
      <div style={{ display: "grid", gap: 12 }}>
        {renderNumber("Power Restore Wait (min)", value.powerRestoreWaitMinutes, (next) =>
          onChange("powerRestoreWaitMinutes", next)
        )}
        {renderNumber("Motor Start Confirm (min)", value.motorStartConfirmMinutes, (next) =>
          onChange("motorStartConfirmMinutes", next)
        )}
        {renderNumber("Motor OFF Confirm (min)", value.motorOffConfirmMinutes, (next) =>
          onChange("motorOffConfirmMinutes", next)
        )}
        {renderNumber("RF ON Pulse (ms)", value.rfOnPulseMs, (next) =>
          onChange("rfOnPulseMs", next)
        )}
        {renderNumber("RF OFF Pulse (ms)", value.rfOffPulseMs, (next) =>
          onChange("rfOffPulseMs", next)
        )}
        {renderNumber("RF Alarm Pulse (ms)", value.rfAlarmPulseMs, (next) =>
          onChange("rfAlarmPulseMs", next)
        )}
      </div>
    </section>
  );
}
