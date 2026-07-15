# Pin Reference

Pin usage for the current `Jenix Loud SOS Siren` firmware on the `ESP32-C3 Super Mini`.

## Active Firmware Pins

| Function | ESP32-C3 Pin | Connection |
|---|---:|---|
| PWM siren output | `GPIO4` | Connect to the Robu MOSFET trigger module signal/PWM input |
| VT trigger input | `GPIO3` | Connect the RF receiver `VT` output here; `3.3V HIGH` starts the configured SOS session |
| Manual button input | `GPIO5` | Connect one side of the push button to `GPIO5`, the other side to `GND` |
| Status LED | `GPIO8` | Uses the onboard LED by default for SOS/button indication |
| Common ground | `GND` | Must be common with MOSFET module signal ground and `12V` supply negative |
| Board power | `5V` / USB-C | Power the ESP32-C3 board from a regulated `5V` rail or USB-C during development |

## Important Notes

- `GPIO4` is the siren PWM output used by the tone engine.
- `GPIO4` now outputs an audible-frequency square wave for the horn, approximately `50%` duty at the selected siren frequency.
- `GPIO3` is the verified `VT` trigger input for the RF receiver. The field-tested final wiring uses `GPIO3`, not `GPIO2`.
- `GPIO5` is configured as `INPUT_PULLUP`, so the external button must short the pin to `GND` when pressed.
- `GPIO8` is treated as the built-in LED pin by default because that is common on ESP32-C3 Super Mini pinouts. If your board revision differs, change [PinConfig.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/config/PinConfig.h).
- This board revision is currently configured as `active-low` for the onboard LED. If the LED behavior is inverted on another board, change `kStatusLedActiveLow` in [PinConfig.h](/D:/IOT%20Device/IOT_Platform/Hardware/Loud%20SOS%20Siren/Firmware/src/config/PinConfig.h).

## Status LED Behavior

- Brief pulse when the manual button is physically pressed
- Stays on while the siren is actively playing

## Suggested External Wiring

- `GPIO4` -> MOSFET trigger module signal/PWM input
- RF receiver `VT` output -> `GPIO3`
- `GND` -> MOSFET trigger module signal ground
- RF receiver `GND` -> ESP32-C3 `GND`
- `12V +` -> horn speaker / MOSFET load path as required by the Robu switch module wiring
- `12V -` -> MOSFET load ground and ESP32 common ground
- Regulated `5V` -> ESP32-C3 `5V`
- Regulated `5V GND` -> ESP32-C3 `GND`
- Button between `GPIO5` and `GND`

## Reserved / Avoid Touching For This Build

- `GPIO8`: reserved for status LED
- `GPIO20` / `GPIO21`: leave free unless you intentionally add UART diagnostics
- Boot / reset hardware buttons on the board: do not repurpose in firmware
