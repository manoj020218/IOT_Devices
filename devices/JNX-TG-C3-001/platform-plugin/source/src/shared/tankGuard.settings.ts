export interface TankGuardConfigSettings {
  capacityLitres: number;
  wifiTxPowerDbm: number;
  zeroLevelMm: number;
  bottomMotorStartLevelMm: number;
  topMotorOffLevelMm: number;
  overflowMarginMm: number;
}

export interface TankGuardAlarmSettings {
  repeatEnabled: boolean;
  repeatMinutes: number;
  lowLevelPct: number;
  notifyOnSensorFault: boolean;
}

export interface TankGuardActionSettings {
  powerRestoreWaitMinutes: number;
  motorStartConfirmMinutes: number;
  motorOffConfirmMinutes: number;
  rfOnPulseMs: number;
  rfOffPulseMs: number;
  rfAlarmPulseMs: number;
}

export interface TankGuardSettingsDraft {
  config: TankGuardConfigSettings;
  alarm: TankGuardAlarmSettings;
  actions: TankGuardActionSettings;
}

export const tankGuardDefaultSettings: TankGuardSettingsDraft = {
  config: {
    capacityLitres: 1000,
    wifiTxPowerDbm: 8.5,
    zeroLevelMm: 0,
    bottomMotorStartLevelMm: 300,
    topMotorOffLevelMm: 900,
    overflowMarginMm: 30
  },
  alarm: {
    repeatEnabled: true,
    repeatMinutes: 2,
    lowLevelPct: 20,
    notifyOnSensorFault: true
  },
  actions: {
    powerRestoreWaitMinutes: 3,
    motorStartConfirmMinutes: 3,
    motorOffConfirmMinutes: 3,
    rfOnPulseMs: 500,
    rfOffPulseMs: 500,
    rfAlarmPulseMs: 500
  }
};
