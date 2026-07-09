# Tank Guard Device Plugin Plan

## Objective

Make Tank Guard the first real device plugin for Jenix One so future devices can follow the same contract.

## Platform flow

1. Device is provisioned and registered.
2. Device shows on `/home` with live or offline state.
3. Tapping the tile opens `/devices/:deviceId`.
4. PID dynamic page renderer loads the Tank Guard plugin page.
5. The plugin page reads the latest telemetry snapshot plus device metadata.
6. Settings changes publish a device command.
7. Device applies settings, stores them in NVS, then reports updated state.

## Tank Guard page content

- Hero area uses about 60% of the mobile page height.
- Animated tank visual shows current fill level.
- Live strip shows RSSI, L/min flow, connectivity, and local endpoint.
- Capacity-aware water math uses `capacityLitres`.
- Zero calibration is a first-class action.
- Detail settings opens one sheet with:
  - config settings
  - alarm settings
  - action settings

## Shared contract

The plugin contract needs:
- manifest
- telemetry schema
- settings schema
- command schema
- scene bindings

## Backend changes in main repo

- Add `latestTelemetry` or equivalent snapshot storage per device.
- Store a small history window for trend and flow math.
- Add authenticated command API for:
  - `refresh`
  - `zero_calibrate`
  - `apply_settings`
  - `motor_on`
  - `motor_off`
  - `alarm_test`
- Persist command status for UI feedback.

## Frontend changes in main repo

- `HomeDashboardPage.tsx`: navigate tile clicks to `/devices/:deviceId`.
- `deviceTelemetry.ts`: replace seeded demo metrics with real snapshot mapping.
- `PidDynamicPageRenderer.tsx`: resolve `tank-guard-mobile` through the plugin registry.
- `DeviceDetailPage.tsx`: fetch plugin state and render plugin page.

## Future device repeatability

Every new device should only add:
- one manifest
- one telemetry mapper
- one settings schema
- one page
- optional small components

Everything else should remain generic and platform-owned.

## Suggested phases

Phase 1:
- Add snapshot persistence and plugin registry.
- Replace demo metrics on `/home`.

Phase 2:
- Ship Tank Guard device page.
- Ship settings command/ack flow.

Phase 3:
- Promote the same pattern for other product lines.
