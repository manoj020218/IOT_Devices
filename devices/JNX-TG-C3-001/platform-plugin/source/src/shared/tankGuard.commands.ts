import type { TankGuardSettingsDraft } from "./tankGuard.settings";

export type TankGuardCommandName =
  | "refresh"
  | "zero_calibrate"
  | "apply_settings"
  | "motor_on"
  | "motor_off"
  | "alarm_test";

export interface TankGuardCommandEnvelope {
  command: TankGuardCommandName;
  payload?: Record<string, unknown>;
  requiresAck: boolean;
}

export function buildRefreshCommand(): TankGuardCommandEnvelope {
  return { command: "refresh", requiresAck: true };
}

export function buildZeroCalibrateCommand(): TankGuardCommandEnvelope {
  return { command: "zero_calibrate", requiresAck: true };
}

export function buildApplySettingsCommand(
  settings: TankGuardSettingsDraft
): TankGuardCommandEnvelope {
  return {
    command: "apply_settings",
    requiresAck: true,
    payload: {
      schemaVersion: 1,
      settings
    }
  };
}

export function buildActionCommand(
  action: "motor_on" | "motor_off" | "alarm_test"
): TankGuardCommandEnvelope {
  return { command: action, requiresAck: true };
}
