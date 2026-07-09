import type { DeviceCommandAckRecord, PlatformDeviceSummary } from "../shared/plugin.types";
import { buildApplySettingsCommand, buildZeroCalibrateCommand } from "../shared/tankGuard.commands";
import type { TankGuardSettingsDraft } from "../shared/tankGuard.settings";

export interface TankGuardCommandPublisher {
  publish(deviceId: string, message: Record<string, unknown>): Promise<string>;
}

export interface TankGuardCommandService {
  applySettings(
    device: PlatformDeviceSummary,
    settings: TankGuardSettingsDraft
  ): Promise<DeviceCommandAckRecord>;
  zeroCalibrate(device: PlatformDeviceSummary): Promise<DeviceCommandAckRecord>;
}

export function createTankGuardCommandService(
  publisher: TankGuardCommandPublisher
): TankGuardCommandService {
  async function queue(
    device: PlatformDeviceSummary,
    command: Record<string, unknown>
  ): Promise<DeviceCommandAckRecord> {
    const commandId = await publisher.publish(device.deviceId, command);
    return {
      commandId,
      deviceId: device.deviceId,
      status: "queued"
    };
  }

  return {
    applySettings(device, settings) {
      return queue(device, buildApplySettingsCommand(settings));
    },
    zeroCalibrate(device) {
      return queue(device, buildZeroCalibrateCommand());
    }
  };
}
