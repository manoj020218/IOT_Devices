# Jenix IOT Devices

This repository keeps device assets by `PID`, so every product can be onboarded, versioned, and published independently.

## Structure

- `devices/<PID>/pid`: product manifest and device capability contract
- `devices/<PID>/firmware/source`: embedded firmware source kept with the device
- `devices/<PID>/firmware/releases/<version>`: built flash artifacts and release metadata
- `devices/<PID>/ui-packages/<packageId>/<version>`: published UI package artifacts for Jenix One
- `devices/<PID>/platform-plugin/source`: device-specific platform integration source

## VPS Mapping

For VPS publishing, map the PID folder into `/root/projects/IOT_one/device-registry` like this:

- `devices/<PID>/pid/*` -> `device-registry/pid/<PID>/`
- `devices/<PID>/firmware/releases/<version>/*` -> `device-registry/firmware/<PID>/<version>/`
- `devices/<PID>/ui-packages/<packageId>/<version>/*` -> `device-registry/ui-packages/<packageId>/<version>/`

This keeps the engineering repo and VPS artifact registry aligned.
