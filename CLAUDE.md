# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Second Movement is a refactored Movement firmware for [Sensor Watch](https://www.sensorwatch.net), an open-source ARM-based smartwatch (SAM D/L series). Written in C, it uses a modular watch face system with an event-driven architecture.

## Build Commands

**Prerequisites**: GNU Arm Embedded Toolchain, git submodules (`git submodule update --init --recursive`)

```bash
# Hardware build (BOARD and DISPLAY are required)
make BOARD=sensorwatch_red DISPLAY=classic

# Board options: sensorwatch_pro, sensorwatch_green, sensorwatch_red, sensorwatch_blue
# Display options: classic, custom

# Simulator build (requires emscripten)
emmake make BOARD=sensorwatch_red DISPLAY=classic
python3 -m http.server -d build-sim
# Then visit http://localhost:8000/firmware.html

# Install to watch (after double-tap reset)
make install

# Clean
make clean
```

Optional build flags: `TIMESET=minute` (set clock from PC), `NOSLEEP` (disable low energy mode), `TINYUSB_CDC=1` (USB serial, on by default).

Nix users: `nix develop` provides the full toolchain (gnumake, emscripten, gcc-arm-embedded).

## Architecture

### Core Loop (`movement.c` / `movement.h`)
The main event loop dispatches events (button presses, RTC ticks, timeouts, accelerometer) to the active watch face. Manages face lifecycle, settings persistence (RTC backup registers BKUP[0-3]), power management, and USB CDC interface.

### Watch Face Interface
Every watch face is a `watch_face_t` struct with four callbacks:
- `setup()` — allocate and initialize face state
- `activate()` — called when face enters foreground
- `loop()` — handle events and update display (called repeatedly)
- `resign()` — clean up before leaving foreground

Plus an optional `advise()` callback for reporting capabilities (background tasks, alarms, DST handling).

### Key Files
- `movement_config.h` — **which faces are included** in the firmware and default settings (LED colors, 24h mode, timeouts). Edit this to customize your watch.
- `movement_faces.h` — central include for all face declarations
- `watch-faces.mk` — lists all face source files for the build; new faces go above the marker comment
- `Makefile` — top-level build config; includes gossamer's `make.mk` and `rules.mk`

### Directory Layout
- `watch-faces/` — modular faces organized by category: `clock/`, `complication/`, `sensor/`, `settings/`, `demo/`, `io/`
- `watch-library/hardware/` — HAL for real Sensor Watch hardware
- `watch-library/simulator/` — Emscripten-based simulator implementations (mirrors hardware/)
- `watch-library/shared/` — common drivers and display utilities
- `lib/` — application libraries (TOTP, base32/64, chirpy_tx, fesk_tx, sunriset)
- `filesystem/` — littlefs-based flash storage abstraction
- `shell/` — USB CDC serial debug shell
- `gossamer/` — external ARM firmware framework (submodule)
- `tinyusb/`, `littlefs/`, `utz/` — external submodules (USB, filesystem, timezone)

### Adding a Watch Face
1. Create `watch-faces/<category>/<name>_face.c` and `.h`
2. Implement the `watch_face_t` callbacks
3. Add the `.c` file to `watch-faces.mk` (above the marker comment)
4. Include the `.h` in `movement_faces.h`
5. Add the face to the `watch_faces[]` array in `movement_config.h`

## Conventions

- **Commit messages**: Conventional commits (`feat:`, `fix:`, `chore:`), scoped when applicable (e.g., `feat(totp_lfs_face): ...`)
- **Naming**: Snake case for files and symbols. Watch face files follow the `<name>_face.c/.h` pattern.
- **Hardware/simulator parity**: Every HAL function in `watch-library/hardware/` has a corresponding simulator implementation in `watch-library/simulator/`.
