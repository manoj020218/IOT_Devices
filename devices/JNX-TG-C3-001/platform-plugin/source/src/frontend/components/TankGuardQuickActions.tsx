import type { ReactNode } from "react";
import { FiRefreshCcw, FiSettings, FiTarget } from "react-icons/fi";

export interface TankGuardQuickActionsProps {
  busy?: boolean;
  onRefresh: () => void;
  onZeroCalibrate: () => void;
  onOpenSettings: () => void;
}

function actionButton(
  label: string,
  icon: ReactNode,
  onClick: () => void,
  disabled = false
) {
  return (
    <button className="jx-btn ghost" type="button" onClick={onClick} disabled={disabled}>
      {icon}
      {label}
    </button>
  );
}

export function TankGuardQuickActions({
  busy,
  onRefresh,
  onZeroCalibrate,
  onOpenSettings
}: TankGuardQuickActionsProps) {
  return (
    <section
      className="panel"
      style={{ display: "flex", gap: 12, flexWrap: "wrap", alignItems: "center" }}
    >
      {actionButton("Refresh", <FiRefreshCcw size={16} />, onRefresh, busy)}
      {actionButton("Zero Level Calibrate", <FiTarget size={16} />, onZeroCalibrate, busy)}
      {actionButton("Detail Settings", <FiSettings size={16} />, onOpenSettings, busy)}
    </section>
  );
}
