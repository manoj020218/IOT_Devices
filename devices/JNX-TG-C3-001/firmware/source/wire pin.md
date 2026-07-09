# Smart Tank Guard A02W Wiring Reference

Field wiring reference for `JNX-TG-A02YYUW-C3`.
Board family: ESP32-C3 development board.
Sensor: DYP `A02YYUW` UART ultrasonic sensor.
RF interface: PC817 optocoupler outputs used as momentary dry-contact button simulation.

## GPIO Pin Map

| GPIO | Direction | Connect To | Purpose |
|------|-----------|------------|---------|
| 20 | INPUT | DYP `TX` | Sensor UART receive into ESP32-C3 |
| 21 | OUTPUT | DYP `RX` | Sensor UART transmit from ESP32-C3 |
| 1 | OUTPUT | PC817 input for Motor ON channel | RF remote ON pulse |
| 2 | OUTPUT | PC817 input for Motor OFF channel | RF remote OFF pulse |
| 3 | OUTPUT | PC817 input for Alarm channel | RF alarm or overflow pulse |
| 4 | OUTPUT | Status LED or spare | Local status indication |

## Power Wiring

| Source | Connect To | Notes |
|--------|------------|-------|
| `12V DC input` | Buck converter input | Use a stable supply sized for outdoor duty |
| `Buck 5V output` | ESP32-C3 `5V` or `VIN` | Do not feed 12V directly to the ESP32-C3 board |
| `Buck 5V output` | DYP sensor `VCC` | Sensor should be powered from the regulated 5V rail |
| `System GND` | ESP32-C3 `GND`, sensor `GND`, PC817 input return | All grounds must be common |

Recommended: use a clean 5V rail with at least `1A` available so WiFi startup, BLE advertising, and the sensor do not brown out the board.

## Sensor Wiring

| DYP Pin | ESP32-C3 Pin | Notes |
|---------|--------------|-------|
| `VCC` | `5V` | Sensor supply |
| `GND` | `GND` | Common ground |
| `TX` | `GPIO20` | Required; this is the sensor data line into the ESP32-C3 |
| `RX` | `GPIO21` | Recommended; keep connected for compatibility and future sensor commands |

Firmware uses:

```cpp
Serial1.begin(9600, SERIAL_8N1, GPIO20, GPIO21);
```

Important electrical note:
- The ESP32-C3 GPIOs are `3.3V` logic and are not 5V tolerant.
- If the DYP sensor `TX` line measures above `3.3V`, add a divider or level shifter before `GPIO20`.
- Do not assume every A02YYUW module has the same UART voltage. Verify once on the bench.

Recommended divider if sensor TX is 5V TTL:

```text
Sensor TX -> 1k -> GPIO20 -> 2k -> GND
```

## PC817 RF Output Wiring

The ESP32-C3 does not drive the RF remote directly. Each command output drives the LED side of a `PC817`, and the transistor side of that optocoupler is soldered across the corresponding remote button contacts.

### Recommended input-side wiring for active-HIGH outputs

| ESP32-C3 GPIO | Series Resistor | PC817 LED Side | Function |
|---------------|-----------------|----------------|----------|
| `GPIO1` | `680R` to `1k` | Anode of PC817 channel 1 | Motor ON |
| `GPIO2` | `680R` to `1k` | Anode of PC817 channel 2 | Motor OFF |
| `GPIO3` | `680R` to `1k` | Anode of PC817 channel 3 | Alarm |
| `GND` | direct | Cathode of each PC817 LED | LED return |

This matches the default firmware setting:

```text
rf_on_active_high    = 1
rf_off_active_high   = 1
rf_alarm_active_high = 1
```

If the field board is wired as active-LOW instead, keep the same logical channel mapping but set the corresponding config flag to `0`.

### Recommended transistor-side wiring

| Command | PC817 Output Side | Connect Across |
|---------|-------------------|----------------|
| Motor ON | Collector + emitter of channel 1 | RF remote ON button pads |
| Motor OFF | Collector + emitter of channel 2 | RF remote OFF button pads |
| Alarm | Collector + emitter of channel 3 | Alarm input pads or buzzer trigger pads |

Important:
- Treat the PC817 transistor side as a dry contact only.
- Confirm the remote really uses simple momentary button closures before soldering.
- If the remote keypad is matrix-scanned, verify the exact pads with a meter first.
- Never connect ESP32 GPIO directly to the remote battery rail or button matrix.

## Status LED Wiring

Recommended active-HIGH LED wiring:

```text
GPIO4 -> 330R resistor -> LED anode
LED cathode -> GND
```

Default firmware setting:

```text
status_led_enabled     = 1
status_led_active_high = 1
```

## Board-Specific Cautions

- `GPIO20` and `GPIO21` are native USB pins on some ESP32-C3 designs. This project intentionally repurposes them for the sensor UART because that matches the working hardware line.
- Do not inject 5V or strong pull-ups or pull-downs onto `GPIO20` or `GPIO21`.
- Keep wiring short between the sensor and the board. Long unshielded UART runs will create unstable readings.
- Keep the buck converter and RF transmitter noise away from the sensor UART lines.
- Keep the BOOT button accessible for maintenance flashing.

## Bench Check Before Final Assembly

1. Power only the ESP32-C3 and confirm the AP name `JNX-TG-XXXX` appears.
2. Connect the sensor and confirm `/api/status` shows changing `raw_distance_mm`.
3. Trigger each manual action once and confirm each PC817 channel pulses only momentarily.
4. Verify the remote button pads are not being held permanently by any output.
5. Only after that, seal the enclosure.

## Installer Summary

- Sensor `TX` must reach `GPIO20`.
- Sensor `RX` should go to `GPIO21`.
- `GPIO1`, `GPIO2`, and `GPIO3` each drive one separate PC817 input.
- The PC817 transistor sides go across the RF remote button pads, not to the ESP32 power rail.
- Use common ground on the ESP side and a clean regulated `5V` rail.
