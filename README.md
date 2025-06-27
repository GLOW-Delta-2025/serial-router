# Echoes of Tomorrow - Light Arms Firmware

This repository contains the embedded firmware for the microcontrollers that operate the LED light arms in the **Echoes of Tomorrow** GLOW 2025 project.

## Overview
- Written in C++
- Compiled and uploaded to microcontrollers (e.g., Arduino or compatible boards)
- Receives audio-reactive signals from the central unit

## Project Structure
```
arms/
├── firmware/           # C++ firmware source
├── tests/              # Diagnostic sketches/tests
└── documentation/      # Schematics and setup guides
```

## Setup Instructions
1. Open `firmware/arms.ino` in the Arduino IDE
2. Select the correct board (e.g., Arduino Nano / Teensy 4.0)
3. Upload to the connected board

## Testing
Diagnostic sketches are in `tests/`

## Documentation
See `documentation/` or the [Wiki](https://github.com/echoes-of-tomorrow/central-unit/wiki) for details on architecture, function descriptions, and setup.

## Branches
- `main`: Production-ready code
- `develop`: Active development
- `feature/<name>`, `bugfix/<name>`, `hotfix/<name>`: Use Git Flow

## Commit Convention
```text
<type>: <short description>
```
Example: `fix: LED flickering on pin 6`
