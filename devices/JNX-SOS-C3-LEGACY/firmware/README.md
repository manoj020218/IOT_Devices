# Jenix Loud SOS Siren Firmware

Firmware for the `ESP32-C3 Super Mini` based `Jenix Loud SOS Siren` controller.

## Hardware

- MCU: ESP32-C3 Super Mini
- Power input: `12V DC`
- Logic supply: regulated `5V` for the ESP32-C3 board
- Switching module: Robu `5V-36V Switch Drive High-power MOSFET Trigger Module`
- Horn load: low-impedance `8 ohm` horn speaker, tested target profiles `Ahuja SUH-15` and `Ahuja SUH-25`
- Factory default speaker profile: `Ahuja SUH-15 Safe`

## Default Pins

- PWM output to MOSFET module signal input: `GPIO4`
- Manual push button to GND: `GPIO5`
- Onboard status LED: `GPIO8` by default, active-low assumption

To change pins, edit [PinConfig.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/config/PinConfig.h).

Additional documentation:

- [PIN.md](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/PIN.md)
- [HANDOFF.md](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/HANDOFF.md)
- [THERMAL.md](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/THERMAL.md)
- [THERMAL_SAFE_SETTINGS.json](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/THERMAL_SAFE_SETTINGS.json)

## Wiring Notes

- The ESP32 `GND`, MOSFET module signal `GND`, and supply negative must be common.
- The horn must be connected only through the MOSFET module load path.
- Do not connect the horn directly to the ESP32.
- The siren output is an audible-frequency square wave on `GPIO4`, not a high-frequency carrier PWM.
- The selected Robu MOSFET trigger module must accept the ESP32 `3.3V` signal input.
- This firmware is now a `12V` horn controller. `24V` direct horn drive is not supported.

## Features

- 4 built-in siren patterns with persisted selection
- Audible-frequency LEDC square-wave generation on ESP32-C3
- Installer-selectable speaker profile:
  - `Ahuja SUH-15 Safe`
  - `Ahuja SUH-25`
- Safety burst and cooling control for low-impedance horn speakers
- Bench diagnostics:
  - fixed `500Hz`, `1000Hz`, `1500Hz`
  - sweep test
  - bench sequence
- Button control:
  - short press: toggle selected tone
  - long press: start SOS pattern
  - very long press: clear Wi-Fi settings and reboot into AP mode
- AP and AP+STA networking with `jenix-sos.local`
- UI hinting and one-time redirect toward the preferred `.local` URL when STA is active
- Embedded responsive web UI
- Web OTA firmware update protected by an admin password
- MQTT and BLE provisioning placeholders for future extension
- Onboard LED indication on button press and SOS playback

## Web Access

If no Wi-Fi is configured, the device starts in AP mode:

- SSID: `JNX-SOS-XXXX`
- Password: `12345678`
- AP IP: `192.168.4.1`

When STA credentials are configured, the device keeps AP mode enabled and also attempts router connection.

## Build

```powershell
pio run
```

## Upload

```powershell
pio run --target upload
```

## Project Layout

```text
src/
  main.cpp
  config/
  models/
  services/
  web/
```
