import { useId } from "react";

import type { PlatformDeviceSummary } from "../../shared/plugin.types";
import type { TankGuardSnapshot } from "../../shared/tankGuard.telemetry";
import { tankGuardTheme } from "../tankGuard.theme";
import type { TankGuardPageModel } from "../services/tankGuardViewModel";

export interface TankHeroProps {
  device: PlatformDeviceSummary;
  snapshot: TankGuardSnapshot;
  model: TankGuardPageModel;
}

export function TankHero({ device, snapshot, model }: TankHeroProps) {
  const id = useId().replace(/[:]/g, "");
  const fillTop = 100 - Math.max(6, Math.min(95, model.usablePct));
  const chip = (label: string, value: string) => (
    <span
      style={{
        padding: "6px 10px",
        borderRadius: 999,
        background: "rgba(255,255,255,0.07)",
        border: `1px solid ${tankGuardTheme.border}`,
        color: tankGuardTheme.text,
        fontSize: 12
      }}
    >
      {label}: {value}
    </span>
  );

  return (
    <section
      className="panel"
      style={{
        minHeight: 420,
        padding: 20,
        background: tankGuardTheme.canvas,
        color: tankGuardTheme.text,
        overflow: "hidden"
      }}
    >
      <div style={{ display: "flex", justifyContent: "space-between", gap: 12 }}>
        <div>
          <p className="device-pid-label" style={{ margin: 0 }}>
            Smart Tank Guard
          </p>
          <h2 style={{ margin: "6px 0 4px", fontSize: 26 }}>{device.displayName}</h2>
          <p style={{ margin: 0, color: tankGuardTheme.muted }}>
            {model.statusLabel} | {snapshot.sensorStatus} | {device.pid}
          </p>
        </div>
        <div
          style={{
            padding: "8px 12px",
            borderRadius: 999,
            height: "fit-content",
            background: "rgba(255,255,255,0.08)",
            color: model.levelTone,
            fontWeight: 700
          }}
        >
          {Math.round(model.usablePct)}%
        </div>
      </div>

      <div
        style={{
          marginTop: 18,
          display: "grid",
          gridTemplateColumns: "minmax(0,1.4fr) minmax(220px,1fr)",
          gap: 18
        }}
      >
        <div
          style={{
            borderRadius: 24,
            padding: 16,
            border: `1px solid ${tankGuardTheme.border}`,
            background: "rgba(255,255,255,0.04)"
          }}
        >
          <svg viewBox="0 0 100 140" width="100%" height="260" preserveAspectRatio="xMidYMid meet">
            <defs>
              <clipPath id={`tank-${id}`}>
                <rect x="15" y="10" width="70" height="118" rx="14" ry="14" />
              </clipPath>
              <linearGradient id={`water-${id}`} x1="0" x2="0" y1="0" y2="1">
                <stop offset="0" stopColor={tankGuardTheme.waterTop} />
                <stop offset="1" stopColor={tankGuardTheme.waterBottom} />
              </linearGradient>
            </defs>
            <rect x="15" y="10" width="70" height="118" rx="14" ry="14" fill="rgba(255,255,255,0.05)" />
            <rect x="22" y="2" width="56" height="10" rx="5" fill="rgba(255,255,255,0.12)" />
            <g clipPath={`url(#tank-${id})`}>
              <rect x="15" y="10" width="70" height="118" fill="rgba(255,255,255,0.03)" />
              <path fill={`url(#water-${id})`}>
                <animate
                  attributeName="d"
                  dur="3.3s"
                  repeatCount="indefinite"
                  values={`M15,${fillTop} q18,-5 35,0 t35,0 V128 H15 Z;M15,${fillTop} q18,5 35,0 t35,0 V128 H15 Z;M15,${fillTop} q18,-5 35,0 t35,0 V128 H15 Z`}
                />
              </path>
              <path fill={tankGuardTheme.waterTop} opacity="0.28">
                <animate
                  attributeName="d"
                  dur="2.4s"
                  repeatCount="indefinite"
                  values={`M15,${fillTop + 4} q18,4 35,0 t35,0 V128 H15 Z;M15,${fillTop + 4} q18,-4 35,0 t35,0 V128 H15 Z;M15,${fillTop + 4} q18,4 35,0 t35,0 V128 H15 Z`}
                />
              </path>
            </g>
            <rect x="15" y="10" width="70" height="118" rx="14" ry="14" fill="none" stroke={model.levelTone} strokeWidth="2" />
          </svg>
        </div>

        <div style={{ display: "grid", gap: 14, alignContent: "start" }}>
          <div>
            <p style={{ margin: 0, color: tankGuardTheme.muted, fontSize: 12 }}>Current water</p>
            <h3 style={{ margin: "8px 0 4px", fontSize: 30, color: model.levelTone }}>
              {model.waterText}
            </h3>
            <p style={{ margin: 0, color: tankGuardTheme.muted }}>
              {snapshot.waterLevelMm} mm sensed height inside a {model.tankHeightMm} mm usable column.
            </p>
          </div>
          <div style={{ display: "flex", flexWrap: "wrap", gap: 8 }}>
            {chip("Pump", snapshot.pumpRunning ? "Running" : "Idle")}
            {chip("Alarm", snapshot.alarmState)}
            {chip("RSSI", `${snapshot.rssiDbm} dBm`)}
          </div>
          <div
            style={{
              padding: 14,
              borderRadius: 18,
              background: "rgba(7,16,26,0.4)",
              border: `1px solid ${tankGuardTheme.border}`
            }}
          >
            <p style={{ margin: 0, fontSize: 12, color: tankGuardTheme.muted }}>Local access</p>
            <p style={{ margin: "6px 0 0", fontWeight: 700 }}>{snapshot.localUrl || "Pending mDNS"}</p>
            <p style={{ margin: "4px 0 0", color: tankGuardTheme.muted }}>{snapshot.localIp || "IP pending"}</p>
          </div>
          <p style={{ margin: 0, color: tankGuardTheme.muted }}>{model.heroHint}</p>
        </div>
      </div>
    </section>
  );
}
