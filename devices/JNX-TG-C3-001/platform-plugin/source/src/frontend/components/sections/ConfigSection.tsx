import type { TankGuardConfigSettings } from "../../../shared/tankGuard.settings";

export interface ConfigSectionProps {
  value: TankGuardConfigSettings;
  onChange: (field: keyof TankGuardConfigSettings, value: number) => void;
}

function renderNumber(
  label: string,
  value: number,
  onChange: (next: number) => void
) {
  return (
    <label style={{ display: "grid", gap: 6 }}>
      <span style={{ fontSize: 13, fontWeight: 600 }}>{label}</span>
      <input
        type="number"
        value={value}
        onChange={(event) => onChange(Number(event.target.value))}
      />
    </label>
  );
}

export function ConfigSection({ value, onChange }: ConfigSectionProps) {
  return (
    <section className="panel">
      <h3 style={{ marginTop: 0 }}>Config Settings</h3>
      <div style={{ display: "grid", gap: 12 }}>
        {renderNumber("Full Tank Capacity (L)", value.capacityLitres, (next) =>
          onChange("capacityLitres", next)
        )}
        {renderNumber("WiFi TX Power (dBm)", value.wifiTxPowerDbm, (next) =>
          onChange("wifiTxPowerDbm", next)
        )}
        {renderNumber("Zero Level (mm)", value.zeroLevelMm, (next) =>
          onChange("zeroLevelMm", next)
        )}
        {renderNumber("Bottom Motor Start Level (mm)", value.bottomMotorStartLevelMm, (next) =>
          onChange("bottomMotorStartLevelMm", next)
        )}
        {renderNumber("Top Motor OFF Level (mm)", value.topMotorOffLevelMm, (next) =>
          onChange("topMotorOffLevelMm", next)
        )}
        {renderNumber("Overflow Margin (mm)", value.overflowMarginMm, (next) =>
          onChange("overflowMarginMm", next)
        )}
      </div>
    </section>
  );
}
