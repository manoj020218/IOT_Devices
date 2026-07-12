# Smart Tank Guard by Jenix

Production ESP32-C3 firmware for `JNX-TG-A02YYUW-C3`.
This device is an offline-first tank automation controller using a DYP `A02YYUW` UART ultrasonic sensor and three RF pulse outputs isolated through PC817 optocouplers.

## Hardware Summary

- ESP32-C3 development board
- `12V DC` input with buck-converted regulated `5V`
- DYP `A02YYUW` ultrasonic sensor over UART
- `GPIO1` RF Motor ON pulse through PC817
- `GPIO2` RF Motor OFF pulse through PC817
- `GPIO3` RF Alarm pulse through PC817
- `GPIO4` optional status LED

Use [wire pin.md](</D:/IOT Device/TankGuard/Sensor/A02W/HW/wire pin.md:1>) as the installer wiring source of truth.

## GPIO Map

- `GPIO20` sensor RX into ESP32-C3 from sensor `TX`
- `GPIO21` sensor TX from ESP32-C3 to sensor `RX`
- `GPIO1` RF Motor ON pulse
- `GPIO2` RF Motor OFF pulse
- `GPIO3` RF Alarm pulse
- `GPIO4` status LED or spare

Important board note:
- This hardware line uses `GPIO20` and `GPIO21` for the sensor UART during normal runtime. Do not design field tooling that assumes those pins remain available for USB data functions.

## Firmware Architecture

- Main login page and dashboard are embedded in firmware HTML.
- `NVS / Preferences` stores config, runtime snapshot, and login password.
- `LittleFS` is used only for bounded rotating event logs exposed through the local WebUI and download API.
- RF outputs are pulse-bounded and protected by hard timeout logic.
- Tank logic is non-blocking and uses `millis()` state machines.
- BLE is used for device discovery only, not full BLE provisioning.

## Project Layout

- `include/project_config.h` constants, enums, device config, runtime models
- `src/main.cpp` boot, WiFi, BLE, MQTT, OTA, and loop orchestration
- `src/config_manager.*` defaults, validation, schema migration, CRC persistence
- `src/sensor_dyp.*` DYP UART parsing and filtered readings
- `src/rf_output.*` pulse-safe RF output control
- `src/tank_logic.*` automation state machine and alarms
- `src/mqtt_client.*` broker connection, publish model, command intake
- `src/ota_manager.*` local and cloud OTA orchestration
- `src/web_server.*` login, dashboard, HTTP API, local OTA
- `src/logger.*` bounded log storage and export
- `data/` optional filesystem assets if needed later

## Build And Flash

1. Install PlatformIO.
2. Build firmware:

```powershell
pio run
```

3. Flash firmware:

```powershell
pio run -t upload
```

4. Filesystem upload is not required for the current root WebUI because `/login` and `/` are embedded in firmware.

Optional only if future filesystem assets are used:

```powershell
pio run -t uploadfs
```

## First Boot And Discovery

On first boot, or whenever station WiFi is not configured, the device starts:

- WiFi AP: `JNX-TG-XXXX`
- BLE advertisement name: `JNX-TG-XXXX`
- Local URL: `http://192.168.4.1/`
- mDNS name when available: `http://jnx-tg-xxxx.local/`

Where `XXXX` is the last 4 hex characters of the ESP32-C3 MAC address.

Important discovery rule:
- BLE is advertisement-only identity for APK or installer discovery.
- The device may not appear as a normal paired Bluetooth device in phone Bluetooth settings.
- Use a BLE scanner app or in-app BLE scan flow.

## Web Login And Local UI

Default first-flash password:

- `Hanuman@2026`

The local WebUI includes:

- Login page with session cookie auth
- Dashboard with live status
- Config form for WiFi, MQTT, tank thresholds, OTA URL, and metadata
- Manual RF pulse actions
- Local `.bin` OTA upload
- Cloud or VPS OTA queue form
- Log view and CSV export
- Password change flow

Core local routes:

- `GET /login`
- `POST /login`
- `GET /logout`
- `GET /api/ping`
- `GET /api/status`
- `GET /api/config`
- `POST /api/config`
- `POST /api/set-zero`
- `POST /api/manual`
- `GET /api/logs`
- `POST /api/restart`
- `POST /api/factory-reset`
- `GET /api/ota/status`
- `POST /api/ota/cloud`
- `POST /api/change-password`
- `POST /update`

Auth rules:

- `/api/ping` is the only intended quick health-check route without login.
- All config, action, logs, and OTA routes require the session cookie after `/login`.
- Blank `wifi_password` or `mqtt_password` in config updates means keep the existing saved password.

## MQTT Integration

Topics:

- `jnx/tg/{device_id}/telemetry`
- `jnx/tg/{device_id}/status`
- `jnx/tg/{device_id}/event`
- `jnx/tg/{device_id}/alarm`
- `jnx/tg/{device_id}/config`
- `jnx/tg/{device_id}/cmd`
- `jnx/tg/{device_id}/ota`

Device behavior:

- Telemetry and status are JSON.
- `status` is published retained.
- The device republishes `config` and `status` when MQTT reconnects.
- Local automation continues even when MQTT is down.
- Default broker endpoint is `mqtt.iotsoft.in:1883`; use this DNS host instead of a raw VPS IP so server moves do not require device-side broker changes.

Supported command examples:

```json
{"command":"motor_on","override":false}
```

```json
{"command":"motor_off"}
```

```json
{"command":"alarm_test"}
```

```json
{"command":"set_config","bottom_motor_start_level_mm":320,"top_motor_off_level_mm":920}
```

```json
{"command":"ota_update","url":"https://example.com/fw.bin","version":"JNX-TG-C3 v1.0.1","checksum":"32-char-md5-hex"}
```

MQTT write rules:

- `set_config` may contain only the fields being changed.
- Missing fields must remain unchanged.
- `motor_on` may include `override:true` for explicit top-limit bypass from backend control.
- `motor_off` must always remain allowed.

## VPS And APK Contract

### APK discovery expectations

- Discover unit by BLE name `JNX-TG-XXXX`.
- Guide installer to join AP `JNX-TG-XXXX`.
- Open `http://192.168.4.1/`.
- Do not depend on BLE credential transfer in this firmware revision.

### VPS responsibilities

- Host the MQTT broker used by the device.
- Persist latest telemetry, status, config snapshot, alarms, and events by `device_id`.
- Host OTA binaries over HTTP or HTTPS.
- Publish remote config changes to `jnx/tg/{device_id}/cmd` using `set_config`.
- Publish remote OTA rollout using `ota_update`.
- Treat AP-only devices as local-only until valid station WiFi and MQTT connectivity exist.

## OTA

Two OTA paths are supported.

### Local OTA

- Login to the local WebUI
- Upload a firmware `.bin` through `/update`
- Device validates the image and reboots on success

### Cloud Or VPS OTA

- Queue update from WebUI using `/api/ota/cloud`
- Or send MQTT command `ota_update`
- Payload includes `url`, `version`, and optional `checksum`
- Device downloads only when the requested version is newer
- Optional checksum currently supports 32-character MD5

## Reliability Strategy

- Config and runtime use versioned CRC-protected binary blobs.
- Legacy config migration is supported across schema updates.
- Runtime writes are throttled to reduce flash wear.
- Sensor data is range-checked, filtered, and faulted on stale data.
- RF outputs cannot remain continuously active beyond timeout protection.
- WiFi reconnect and MQTT reconnect are non-blocking.
- Power-restore behavior uses persisted last state and last command.
- Motor status is always reported as assumed, never confirmed.

## Validation Checklist

- Bench boot test with valid sensor frames
- Verify AP and BLE names match the same `JNX-TG-XXXX`
- Verify `/login` and password flow
- Verify `/api/status` updates with live sensor movement
- Verify each manual RF pulse channel is momentary only
- Dry-run alarm test for no-rise condition
- Overflow alarm test for above-top condition
- OFF-failed test by simulating continued rise after OFF pulse
- Power-loss recovery test during `WAIT_RISE_CONFIRM` and `FILLING`
- WiFi outage test with AP fallback recovery
- MQTT broker outage test with continued local automation
- Local OTA upload test
- Cloud OTA request test with MD5 when checksum validation is required

## Notes

- This is RF-only version 1.
- Motor state must be shown only as `Assumed ON`, `Assumed OFF`, or `Unknown`.
- No fake feedback or fake motor confirmation should be added.
