# Thermal Notes

This file is for reducing heat on the `ESP32-C3 Super Mini` controller and for diagnosing when the whole dev module becomes warm or hot.

## Immediate Safety Rule

- If the board becomes too hot to touch comfortably for more than a second, stop the siren test and remove power.

## What Was Added In Firmware

- Wi-Fi modem sleep is enabled in [ThermalConfig.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/config/ThermalConfig.h)
- Wi-Fi TX power is reduced to `WIFI_POWER_8_5dBm`
- A thermal boot summary is printed on serial

These changes reduce radio-side heat a little, but they will not solve a wiring or power-path problem.

## Most Common Causes Of A Hot ESP32-C3 Module

- The horn current is accidentally flowing through the ESP32 board instead of only through the MOSFET load path
- The ESP32 board is being fed from the wrong voltage rail
- USB power and external `5V` are back-feeding each other
- The `24V/30V -> 5V` buck converter output is too high
- The MOSFET module signal ground and supply negative are not wired correctly
- The siren duty settings are too aggressive for long runs
- The board is in poor airflow while AP+STA Wi-Fi and the horn are both running continuously

## Wiring Checks

- ESP32 board power input must be `5V`, not `24V`
- Horn must be connected only on the MOSFET module load side
- ESP32 `GND`, MOSFET signal `GND`, and supply negative must be common
- Do not feed the board from USB and an unknown external `5V` source unless the power path is understood
- Measure the buck converter output. It should be close to `5.0V`, not `5.5V+`

## Conservative Runtime Settings

Use the importable file [THERMAL_SAFE_SETTINGS.json](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/THERMAL_SAFE_SETTINGS.json) from the web UI System section if you want a cooler starting point.

That preset lowers:

- normal duty
- boost duty
- boost duration
- max duty
- ON duration

and increases rest time.

## Recommended Bench Procedure

1. Start with no horn connected and verify the ESP32 board itself stays cool.
2. Connect the MOSFET module only and verify the ESP32 still stays cool.
3. Connect the horn and test with `THERMAL_SAFE_SETTINGS.json`.
4. Increase duty only after checking board temperature, horn temperature, and buck converter temperature.

## If The Board Still Gets Hot

- Check the `5V` rail with a meter while the siren is active
- Confirm the horn is not connected to any ESP32 pin or power rail
- Confirm the buck converter can supply the ESP32 cleanly under siren switching noise
- Add more distance between the horn power wiring and the ESP32 board
- Consider using only AP mode during testing instead of AP+STA if router connectivity is not needed

## Files Related To Heat Reduction

- [src/config/ThermalConfig.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/config/ThermalConfig.h)
- [src/services/WifiManagerService.cpp](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/services/WifiManagerService.cpp)
- [THERMAL_SAFE_SETTINGS.json](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/THERMAL_SAFE_SETTINGS.json)
