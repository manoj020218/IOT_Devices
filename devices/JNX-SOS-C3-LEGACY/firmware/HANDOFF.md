# Handoff Notes

This document is the maintenance handoff for the `Jenix Loud SOS Siren` firmware project.

## What This Firmware Does

- Generates audible-frequency siren patterns on `GPIO4` using ESP32-C3 LEDC square-wave output
- Drives a `12V` low-impedance horn indirectly through the external Robu MOSFET trigger module
- Persists tone selection and safety settings in NVS
- Enforces speaker-safe active duration, burst, and cooling behaviour
- Exposes a local web UI with AP mode, AP+STA mode, OTA upload, tone selection, and safety settings
- Uses the onboard LED on `GPIO8` as a small status indicator

## Key Files

- Entry point: [src/main.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/main.cpp)
- Pin defaults: [src/config/PinConfig.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/config/PinConfig.h)
- Product constants: [src/config/ProductConfig.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/config/ProductConfig.h)
- Tone engine: [src/services/ToneEngine.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/ToneEngine.cpp)
- Profiles: [src/services/ToneProfiles.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/ToneProfiles.cpp)
- Settings persistence: [src/services/SettingsManager.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/SettingsManager.cpp)
- Wi-Fi and mDNS: [src/services/WifiManagerService.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/WifiManagerService.cpp)
- Web API/UI: [src/services/WebServerService.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/WebServerService.cpp), [src/web/WebAssets.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/web/WebAssets.h)
- LED indicator logic: [src/services/IndicatorService.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/IndicatorService.cpp)

## Build And Flash

Build:

```powershell
pio run
```

Standard upload:

```powershell
pio run --target upload --upload-port COM8
```

Direct esptool upload used successfully during bring-up:

```powershell
python C:\Users\User\.platformio\packages\tool-esptoolpy\esptool.py --chip esp32c3 --port \\.\COM8 --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 .pio\build\esp32-c3-supermini\bootloader.bin 0x8000 .pio\build\esp32-c3-supermini\partitions.bin 0x10000 .pio\build\esp32-c3-supermini\firmware.bin
```

## Runtime Access

- AP SSID pattern: `JNX-SOS-XXXX`
- AP password: `12345678`
- AP fallback URL: `http://192.168.4.1`
- Preferred hostname: `http://jenix-sos.local`

When STA is connected, the UI now exposes the preferred `.local` URL and will attempt a one-time redirect when the page is opened through the raw station IP.

## Thermal Support Files

- [THERMAL.md](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/THERMAL.md)
- [THERMAL_SAFE_SETTINGS.json](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/THERMAL_SAFE_SETTINGS.json)
- [src/config/ThermalConfig.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/config/ThermalConfig.h)

## How To Change Pins

Edit [src/config/PinConfig.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/config/PinConfig.h):

- `kVtTriggerGpio`
- `kPwmOutGpio`
- `kButtonGpio`
- `kStatusLedGpio`
- `kStatusLedActiveLow`

If the onboard LED behaves backwards, flip `kStatusLedActiveLow`.

## Final Verified Pin Map

- `GPIO3` = RF receiver `VT` trigger input, active `3.3V HIGH`
- `GPIO4` = siren PWM output to the Robu MOSFET trigger module
- `GPIO5` = manual push button to `GND`
- `GPIO8` = onboard blue status LED, configured `active-low`
- `GND` must be common between the ESP32-C3, RF receiver, and MOSFET module

The final client-tested VT wiring uses `GPIO3`. A wrong connection to `GPIO2` will not trigger the siren.

## How To Change Tone Behavior

- Built-in profiles are defined in [src/services/ToneProfiles.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/ToneProfiles.cpp).
- Playback state logic is in [src/services/ToneEngine.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/ToneEngine.cpp).
- Duty and safety clamping are in [src/services/SettingsManager.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/SettingsManager.cpp) and [src/services/ToneEngine.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/ToneEngine.cpp).

## How To Change Web UI Behavior

- The REST endpoints are registered in [src/services/WebServerService.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/WebServerService.cpp).
- The HTML/CSS/JS is embedded in [src/web/WebAssets.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/web/WebAssets.h).

If the UI needs larger changes later, the first refactor should be splitting `WebAssets.h` into smaller HTML/JS fragments or generating compressed assets.

## Known Assumptions

- The Robu MOSFET trigger module accepts `3.3V PWM` correctly.
- The onboard LED is assumed to be on `GPIO8`.
- The current tested board revision uses an `active-low` onboard LED configuration.
- The final siren firmware is `12V` only and defaults to `Ahuja SUH-15 Safe`.
- Installer speaker profiles:
  - `Ahuja SUH-15 Safe`
  - `Ahuja SUH-25`
- `.local` access depends on client mDNS support and network behavior. The device advertises `jenix-sos.local`, but some phones or routers may still prefer direct IP fallback.

## Quick Regression Checklist After Any Change

- `pio run` completes successfully
- AP mode still comes up with `JNX-SOS-XXXX`
- `http://192.168.4.1` loads the UI
- `http://jenix-sos.local` resolves on a supported client
- Selected tone persists after reboot
- Short button press toggles siren
- Long press starts SOS
- `VT` pulse on `GPIO3` starts the configured SOS session
- Fixed-tone bench tests `500Hz`, `1000Hz`, and `1500Hz` work from the web UI
- Very long press clears Wi-Fi and reboots
- OTA upload still accepts a valid `.bin`
- Siren output idles LOW when stopped
