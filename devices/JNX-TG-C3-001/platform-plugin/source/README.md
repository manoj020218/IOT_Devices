# Tank Guard Plugin Package

This folder is the first reusable device-plugin package for Jenix One.

Goals:
- Keep `/home` as the common platform dashboard.
- Open Tank Guard on `/devices/:deviceId`.
- Render one mobile-first Tank Guard page on the existing Jenix theme.
- Use real telemetry, not demo metrics.
- Send settings to the device as commands, then persist them in device NVS.
- Keep the pattern reusable for future device types.

Package layout:
- `docs/`: rollout and integration plan.
- `src/shared/`: generic plugin contract plus Tank Guard manifest.
- `src/backend/`: telemetry snapshot and command API contract.
- `src/frontend/`: Tank Guard device page and small components.
- `src/integration/`: registry and renderer bridge examples.

Key design rules:
- One page per device type.
- Components stay small and focused.
- No settings-over-telemetry shortcut.
- Home tiles should link to `/devices/:deviceId`.
- PID and `dashboard.dynamicPages` remain the activation switch.

Recommended Tank Guard PID settings:
- `dashboard.templateId = "tank-guard-mobile-v1"`
- `dashboard.dynamicPages = ["tank-guard-mobile", "thresholds"]`

Backend work still required in the main platform repo:
- Persist latest telemetry snapshot per device.
- Expose latest telemetry to device detail.
- Add authenticated device command endpoint for plugin actions.
- Store command ack/result for operator feedback.

Frontend work still required in the main platform repo:
- Replace demo home metrics mapping with real telemetry mapping.
- Route device tiles to `/devices/:deviceId`.
- Load the plugin page through the PID dynamic renderer.
