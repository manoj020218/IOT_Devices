import {
  buildApplySettingsCommand,
  buildRefreshCommand,
  buildZeroCalibrateCommand
} from "../../shared/tankGuard.commands";
import type { TankGuardSettingsDraft } from "../../shared/tankGuard.settings";

export interface TankGuardCommandTransport {
  post(input: {
    deviceId: string;
    pluginId: string;
    command: Record<string, unknown>;
  }): Promise<Record<string, unknown>>;
}

export interface TankGuardCommandClient {
  refresh(deviceId: string): Promise<Record<string, unknown>>;
  zeroCalibrate(deviceId: string): Promise<Record<string, unknown>>;
  applySettings(
    deviceId: string,
    settings: TankGuardSettingsDraft
  ): Promise<Record<string, unknown>>;
}

export function createTankGuardCommandClient(
  transport: TankGuardCommandTransport
): TankGuardCommandClient {
  const pluginId = "tank-guard-mobile";

  return {
    refresh(deviceId) {
      return transport.post({
        deviceId,
        pluginId,
        command: buildRefreshCommand()
      });
    },
    zeroCalibrate(deviceId) {
      return transport.post({
        deviceId,
        pluginId,
        command: buildZeroCalibrateCommand()
      });
    },
    applySettings(deviceId, settings) {
      return transport.post({
        deviceId,
        pluginId,
        command: buildApplySettingsCommand(settings)
      });
    }
  };
}
