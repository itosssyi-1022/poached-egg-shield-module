# Poached Eggs firmware investigation report

## Summary

- This repository builds **ZMK** firmware for the **Poached Eggs** split keyboard.
- Both halves target the **Seeed Studio XIAO BLE** (`seeeduino_xiao_ble`), which is an **nRF52840**-based MCU.
- The left half is configured as the **split central** and the right half is the **split peripheral**.
- The most likely reason a newly generated `.keymap` build appears to have **no effect at all** is that the repository enables **ZMK Studio** on the left half, and ZMK persistent settings can keep using a previously saved runtime keymap even after reflashing new firmware.
- This repository did not provide a GitHub Actions build for **`settings_reset`**, so there was no built-in way to clear the persisted settings from the generated artifacts.

## Repository findings

### Hardware / MCU

- Build target board: `seeeduino_xiao_ble`
  - Source: `/home/runner/work/poached-egg-shield-module/poached-egg-shield-module/build.yaml`
- Shield metadata requires `seeed_xiao`
  - Source: `/home/runner/work/poached-egg-shield-module/poached-egg-shield-module/config/boards/shields/poached_eggs/poached_eggs.zmk.yml`

### Split roles

- `CONFIG_ZMK_SPLIT_ROLE_CENTRAL` is enabled only for the left shield.
- `CONFIG_ZMK_SPLIT` is enabled for both left and right shields.
  - Source: `/home/runner/work/poached-egg-shield-module/poached-egg-shield-module/config/boards/shields/poached_eggs/Kconfig.defconfig`

This means:

- **Left half**: talks to the host and evaluates the keymap.
- **Right half**: sends matrix events to the left half.

So if the user checks only host-visible behavior, the left half is the important side to verify first.

### Studio / persistent settings

- The left half enables:
  - `CONFIG_ZMK_STUDIO=y`
  - `CONFIG_ZMK_STUDIO_LOCKING=y`
  - Source: `/home/runner/work/poached-egg-shield-module/poached-egg-shield-module/config/boards/shields/poached_eggs/poached_eggs_left.conf`
- The GitHub Actions build for the left half also adds the `studio-rpc-usb-uart` snippet.
  - Source: `/home/runner/work/poached-egg-shield-module/poached-egg-shield-module/build.yaml`
- The keymap contains a `&studio_unlock` binding on the Bluetooth control layer.
  - Source: `/home/runner/work/poached-egg-shield-module/poached-egg-shield-module/config/poached_eggs.keymap`

These settings strongly indicate that the normal left-half firmware is intended to be used with ZMK Studio.

## Root cause analysis

The most plausible failure mode is:

1. A prior runtime keymap or other persisted ZMK settings already exist in flash.
2. The user edits `/config/poached_eggs.keymap`, pushes changes, and downloads the newly built firmware from GitHub Actions.
3. The user reflashes both halves.
4. On boot, ZMK restores the persisted settings and continues using the stored runtime keymap/state instead of the newly compiled default keymap.
5. The user observes that the `.keymap` changes were not reflected.

This matches the repository because the left firmware explicitly enables ZMK Studio and persistent runtime editing support.

## Why the problem is easy to misdiagnose

- The right half is not the split central, so flashing only the right half cannot explain host-side keymap behavior by itself.
- The generated left and right firmware files can both be flashed successfully, but a successful flash does **not** imply that persisted settings were cleared.
- The repository previously generated only the normal left/right firmware, not a reset image.

## Change made in this repository

`/home/runner/work/poached-egg-shield-module/poached-egg-shield-module/build.yaml` now also builds:

- `seeeduino_xiao_ble + settings_reset`

This adds a GitHub Actions artifact that can be flashed to clear ZMK persistent settings before reflashing the normal Poached Eggs firmware.

## Recommended recovery procedure

1. Run GitHub Actions and download these artifacts:
   - `poached_eggs_left`
   - `poached_eggs_right`
   - `settings_reset`
2. Flash `settings_reset` to the **left** half.
3. Flash `settings_reset` to the **right** half.
4. Reflash the normal firmware:
   - left half: `poached_eggs_left`
   - right half: `poached_eggs_right`
5. If Bluetooth was previously paired, remove the old host pairing and pair again.
6. Verify the updated layout from the **left/central** side behavior first.

## Validation notes

- I inspected the repository build and shield configuration directly.
- A full local ZMK build could not be executed in this environment because `west` is not installed here.
- The change is intentionally minimal: it only adds the missing reset artifact build and records the investigation results.
