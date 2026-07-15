# Client Handoff

This document is the final handoff for the `Jenix Loud SOS Siren` firmware delivered on the `ESP32-C3 Super Mini`.

## Delivery Summary

- Product: `Jenix Loud SOS Siren`
- Product ID: `PD-JNX-SOS-25W-C3`
- Firmware version: `1.2.0`
- Board used for final flash: `ESP32-C3 Super Mini`
- Final verified upload port during delivery: `COM8`
- Final target use: `12V` horn warning siren with local web control, VT trigger input, and saved tone selection

## Delivered Functions

- Audible siren output on `GPIO4` using ESP32-C3 LEDC square-wave generation
- Manual push button input on `GPIO5`
- RF receiver `VT` input on `GPIO3`
- Onboard blue LED status indication on `GPIO8`
- Local AP mode and AP+STA web interface
- OTA firmware upload from the web interface
- Saved tone selection in NVS
- Saved VT trigger settings in NVS
- Installer speaker profile selection in NVS
- Ten application-ready siren tone profiles
- VT trigger modes:
  - `Inching`
  - `Timed`
  - `Toggle`

## Final Verified Wiring

- `GPIO3` = RF receiver `VT` output input, active `3.3V HIGH`
- `GPIO4` = siren PWM output to the Robu MOSFET trigger module signal input
- `GPIO5` = manual push button to `GND`
- `GPIO8` = onboard blue LED, configured `active-low`
- ESP32 `GND`, RF receiver `GND`, MOSFET signal `GND`, and supply negative must be common

Important field note:

- The VT trigger was verified on `GPIO3`
- A wrong connection to `GPIO2` will not trigger the siren
- The horn load-side ground connection on the MOSFET path must be complete or the horn will stay silent even if the module LED blinks

## Horn / Power Arrangement

- Horn type verified for this firmware direction: low-impedance `8 ohm` horn speaker
- Tested intended horn profiles:
  - `Ahuja SUH-15 Safe`
  - `Ahuja SUH-25`
- Power model for this delivered firmware: `12V` horn system
- The ESP32-C3 itself must be powered from regulated `5V` or USB, not directly from `12V`

## Web Access

If no router Wi-Fi is configured, the unit starts its own AP:

- SSID pattern: `JNX-SOS-XXXX`
- AP password: `12345678`
- AP fallback URL: `http://192.168.4.1`

If station Wi-Fi is configured, the device also advertises:

- Preferred hostname: `http://jenix-sos.local`

## Default Credentials

- Default admin password: `admin123`

This password is used for:

- OTA upload
- speaker profile changes

If changed from the web UI, the new saved password becomes the active admin password.

## Saved Settings Behavior

These settings are stored in NVS and survive power loss:

- selected tone profile
- VT trigger enable/disable
- VT trigger mode
- VT trigger duration
- VT retrigger mode
- VT cloud notification flag
- speaker profile
- thermal/safety settings
- Wi-Fi credentials
- admin password

The `VT Trigger SOS` card now has its own:

- `Save VT Settings`
- `Default VT Settings`

The `Tone Test / Selection` card stores the selected tone with:

- `Save Selected Tone`

## Default VT Settings

Factory VT defaults in the delivered firmware:

- enabled: `true`
- mode: `Inching`
- duration: `60 sec`
- retrigger mode: `Extend Timer`
- cloud notification: `Disabled`

VT mode meanings:

- `Inching`: siren remains active while `VT` stays `HIGH`, then stops on `LOW`
- `Timed`: one VT fire starts the siren for the saved duration
- `Toggle`: first VT fire turns siren on, next VT fire turns siren off

## Tone Profiles Included

The delivered firmware includes these ten profiles:

1. `Emergency Evacuation`
2. `Fire Zone Alarm`
3. `Flood Warning`
4. `Landslide / Hazard Alert`
5. `Factory Shift Start / Duty On`
6. `Factory Shift End / Duty Off`
7. `Lunch Break Bell`
8. `Assembly Point Call`
9. `Distress SOS`
10. `All Clear / Test Tone`

Saved tone behavior:

- the saved selected tone is used by `START SOS`
- the saved selected tone is used by the manual push button
- the saved selected tone is used by the VT trigger

## Speaker Profiles

Installer speaker profiles provided:

- `Ahuja SUH-15 Safe`
- `Ahuja SUH-25`

Current safety behavior:

- loudness is fixed for maximum output
- settings change thermal timing and tone behavior, not “volume”
- long-run operation was tuned so the siren no longer chops abruptly every few seconds
- thermal cooling protection still remains active to protect the horn and MOSFET path

## User Controls

Manual button behavior:

- short press: toggle selected tone
- long press: start SOS with the saved tone
- very long press: clear Wi-Fi credentials and reboot

Web UI actions:

- `Save Selected Tone`
- `Test 500Hz`
- `Test 1000Hz`
- `Test 1500Hz`
- `Run Sweep Test`
- `Run Bench Test`
- `START SOS`
- `Stop Siren`
- `Save VT Settings`
- `Default VT Settings`
- `Apply Speaker Profile`
- `Save Settings`

Any setting save now shows a browser alert confirmation.

## Diagnostic Notes

- Fixed-frequency tests `500Hz`, `1000Hz`, and `1500Hz` are short hardware checks
- Named profile tests are intentionally longer so the siren sounds like a full siren phrase instead of a clipped burst
- The status dashboard shows VT input state, VT mode, VT last trigger, last stop reason, and active tone/frequency

## Final Verified Files

Core files for future maintenance:

- entry point: [src/main.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/main.cpp)
- product config: [src/config/ProductConfig.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/config/ProductConfig.h)
- pin config: [src/config/PinConfig.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/config/PinConfig.h)
- tone engine: [src/services/ToneEngine.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/ToneEngine.cpp)
- tone profiles: [src/services/ToneProfiles.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/ToneProfiles.cpp)
- settings storage: [src/services/SettingsManager.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/SettingsManager.cpp)
- VT handling: [src/services/VtTriggerService.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/VtTriggerService.cpp)
- web API: [src/services/WebServerService.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/WebServerService.cpp)
- UI assets: [src/web/WebAssets.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/web/WebAssets.h)
- pin reference: [PIN.md](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/PIN.md)

## Build And Upload

Build:

```powershell
pio run
```

Upload:

```powershell
pio run -e esp32-c3-supermini -t upload --upload-port COM8
```

## Validation Completed

Completed during final delivery work:

- `pio run` completed successfully on the delivered firmware
- final firmware upload to `COM8` completed successfully
- horn output verified after correcting MOSFET load-side ground wiring
- VT trigger verified after correcting VT wire from `GPIO2` to `GPIO3`
- blue LED behavior corrected to track actual siren playback
- VT `Inching` mode, saved tone use, and longer tone preview behavior implemented

## Known Constraints

- this delivered firmware is for `12V` horn use, not `24V` direct horn drive
- `.local` access depends on client and router mDNS behavior
- `gh` was not installed on the delivery machine at the time of packaging, so GitHub PR automation was not part of the firmware itself

## Legacy Comparison Build

The project folder also contains a separate reconstructed legacy comparison build:

- [yesterday_build_legacy](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/yesterday_build_legacy)

This is separate from the delivered final firmware and was kept only for A/B comparison.

