# Echoes of Tomorrow - Serial routing Firmware

This repository contains the embedded firmware for the microcontroller that operate as a serial router in the **Echoes of Tomorrow** GLOW 2025 project.

## Overview
- Written in C++
- Compiled and uploaded to microcontrollers (Teensy 4.1)
- Connects the Mac mini to the microcontrollers via serial

## Project Structure
```
arm/
├── firmware/           # C++ firmware source
├── tests/              # Diagnostic sketches/tests
└── documentation/      # Schematics and setup guides
```

## Setup Instructions
1. Open `firmware/arm.ino` in the Arduino IDE
2. Select the correct board (Teensy 4.1)
3. Upload to the connected board

## Testing
Diagnostic sketches are in `tests/`

## Documentation
See `documentation/` or the [Wiki](https://github.com/GLOW-Delta-2025/master/wiki) for details on architecture, function descriptions, and setup.

## Branches
- `main`: Production-ready code
- `develop`: Active development
- `feature/<name>`, `bugfix/<name>`, `hotfix/<name>`: Use Git Flow

## Commit Convention
```text
<type>: <short description>
```
Example: `fix: LED flickering on pin 6`
