import { FiActivity, FiGlobe, FiRadio, FiWifi } from "react-icons/fi";

import type { TankGuardSnapshot } from "../../shared/tankGuard.telemetry";
import { tankGuardTheme } from "../tankGuard.theme";
import type { TankGuardPageModel } from "../services/tankGuardViewModel";

export interface TankGuardStatBarProps {
  snapshot: TankGuardSnapshot;
  model: TankGuardPageModel;
}

export function TankGuardStatBar({ snapshot, model }: TankGuardStatBarProps) {
  const cards = [
    { label: "WiFi RSSI", value: `${snapshot.rssiDbm} dBm`, icon: <FiRadio />, tone: model.signalTone },
    { label: "Flow", value: `${snapshot.flowLitresPerMin.toFixed(1)} L/min`, icon: <FiActivity />, tone: tankGuardTheme.ok },
    { label: "SSID", value: snapshot.wifiSsid || "Provisioning AP", icon: <FiWifi />, tone: tankGuardTheme.warn },
    { label: "Radio", value: `${snapshot.wifiTxPowerDbm.toFixed(1)} dBm`, icon: <FiGlobe />, tone: tankGuardTheme.text }
  ];

  return (
    <section
      style={{
        display: "grid",
        gridTemplateColumns: "repeat(auto-fit, minmax(140px, 1fr))",
        gap: 12
      }}
    >
      {cards.map((card) => (
        <article
          key={card.label}
          className="panel"
          style={{ padding: 14, background: "rgba(255,255,255,0.04)" }}
        >
          <div style={{ display: "flex", justifyContent: "space-between", gap: 12 }}>
            <span style={{ color: tankGuardTheme.muted, fontSize: 12 }}>{card.label}</span>
            <span style={{ color: card.tone }}>{card.icon}</span>
          </div>
          <div
            style={{
              marginTop: 12,
              fontWeight: 800,
              fontSize: card.label === "SSID" ? 16 : 20,
              color: tankGuardTheme.text
            }}
          >
            {card.value}
          </div>
        </article>
      ))}
    </section>
  );
}
