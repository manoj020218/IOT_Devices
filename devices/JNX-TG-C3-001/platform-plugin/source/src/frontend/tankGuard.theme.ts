export const tankGuardTheme = {
  canvas: "linear-gradient(180deg, #081321 0%, #0f2333 48%, #112d3b 100%)",
  panel: "rgba(255,255,255,0.05)",
  border: "rgba(255,255,255,0.08)",
  text: "#f5fbff",
  muted: "#91a8b7",
  ok: "#41d6a4",
  warn: "#ffbe5c",
  alert: "#ff6d6d",
  waterTop: "#71d4ff",
  waterBottom: "#1f7de6"
} as const;

export function tankGuardLevelTone(levelPct: number): string {
  if (levelPct >= 70) return tankGuardTheme.ok;
  if (levelPct >= 30) return tankGuardTheme.warn;
  return tankGuardTheme.alert;
}

export function tankGuardSignalTone(rssiDbm: number): string {
  if (rssiDbm >= -65) return tankGuardTheme.ok;
  if (rssiDbm >= -78) return tankGuardTheme.warn;
  return tankGuardTheme.alert;
}

export function tankGuardStatusLabel(online: boolean, alarmState: string): string {
  if (!online) return "Offline";
  if (alarmState !== "normal") return "Alert";
  return "Live";
}
