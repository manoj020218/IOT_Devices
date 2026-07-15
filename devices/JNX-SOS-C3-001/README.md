# JNX-SOS-C3-001

Final delivery package for the `Jenix Loud SOS Siren` on `ESP32-C3 Super Mini`.

## Contents

- `firmware/source`
  - complete PlatformIO source package
  - siren source, web UI, VT trigger logic, handoff notes, and thermal documents
- `firmware/releases/1.2.0/firmware.bin`
  - tested firmware image for version `1.2.0`

## Highlights

- `12V` horn siren firmware for low-impedance `8 ohm` horn speakers
- verified `GPIO3` VT trigger input
- verified `GPIO4` siren output to the Robu MOSFET trigger module
- ten saved tone profiles
- VT modes:
  - `Inching`
  - `Timed`
  - `Toggle`
- local web UI, OTA update, saved NVS settings, and client handoff documentation

## Key Documents

- `firmware/source/README.md`
- `firmware/source/HANDOFF.md`
- `firmware/source/PIN.md`
- `firmware/source/THERMAL.md`

