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
See `documentation/` or the [Wiki](https://github.com/GLOW-Delta-2025/central-unit/wiki) for details on architecture, function descriptions, and setup.

## Teensy ↔ ESP32 Connector Probe

This reference setup pairs a Teensy 4.1 with an ESP32-WROVER over a UART link so you can validate wiring and baud configuration before dropping the code into the main firmware.

### Hardware

- Teensy 4.1 (Arduino Teensy core installed)
- ESP32-WROVER (or similar ESP32 module with exposed GPIO25/26)
- Jumper wires for TX/RX crossover and common ground
- Optional: shared 3.3 V supply (or power each board separately via USB—**always** share ground)

#### Wiring

| Teensy 4.1 | ESP32 |
| --- | --- |
| Pin 1 (TX1) | GPIO26 (`ESP_RX_PIN`) |
| Pin 0 (RX1) | GPIO25 (`ESP_TX_PIN`) |
| GND | GND |
| 3.3 V (optional) | 3.3 V |

> ⚠️ When both boards are USB-powered, leave 3.3 V disconnected and only share ground. Avoid tying USB 5 V rails together.

### Software Setup

#### Arduino IDE board packages

1. **Install Teensy support**
	- IDE 1.8.x: install *Teensyduino* from PJRC, then select “Teensy 4.1”.
	- IDE 2.x: open **Boards Manager**, search “Teensy”, and install the “Teensy Boards” package. (PJRC download link available on their site.)
2. **Install ESP32 support**
	- Boards Manager URL: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
	- Search “ESP32” in Boards Manager, install the latest 3.x package, then select “ESP32 Wrover Module”.

#### Libraries

Both sketches rely only on the Arduino cores—no additional libraries required.

### Flashing the Sketches

#### Teensy 4.1 (`connector_probe.ino`)

1. Open `connector_probe.ino` in the Arduino IDE.
2. Select **Tools → Board → Teensy 4.1**.
3. (Optional) Set **USB Type** to “Serial” for host logging.
4. Build and upload. The Teensy Loader should appear; tap the reset button if it doesn’t auto-program.
5. Open **Serial Monitor** at **115200 baud** to watch TX/RX logs.

#### ESP32 (`esp_connector`)

1. Open the `esp_connector` sketch (rename to `esp_connector.ino` or place it in a same-named folder if prompted).
2. Select **Tools → Board → ESP32 Wrover Module**.
3. Choose the correct serial port. If upload fails with “wrong boot mode,” hold **BOOT**, tap **EN/RST**, release **BOOT** as the IDE shows “Connecting…”.
4. Upload, then open **Serial Monitor** at **115200 baud** to view the startup banner and echoed bytes.

### Bring-up Checklist

1. Flash both boards with the sketches above.
2. Wire the UART crossover (Teensy TX1 ↔ ESP GPIO26, Teensy RX1 ↔ ESP GPIO25) and common ground.
3. Power both boards and open their serial monitors at 115200 baud.
4. Verify:
	- Teensy prints `TX1 sent bit:` every 500 ms and logs incoming bytes (`ESP`, `'?'` heartbeats, echoed bits).
	- ESP32 logs every byte it sees and echoes the character back.

If the Teensy only logs outbound bits, re-check wiring and confirm the ESP32 console shows the startup banner. GPIO16/17 on WROVER modules can emit boot chatter; using GPIO26/25 avoids that noise.

**Expected Serial Sample**

```
TX1 sent bit: 1
RX1 received byte: '1' (0x31)
TX1 sent bit: 0
RX1 received byte: '0' (0x30)
RX1 received byte: '?' (0x3F)
```

### Troubleshooting

- **No ESP output**: Ensure the Arduino IDE selected the correct board/port and that the ESP32 powers up (look for the reset banner over USB).
- **Upload fails**: Enter download mode manually (hold BOOT, tap EN) or disconnect Teensy TX/RX during flashing.
- **Garbled characters**: Confirm both sketches use 115200 baud and that you’re not mixing 5 V logic (both boards run at 3.3 V).
- **Link silence**: The ESP32 sends a `'?'` heartbeat every 2 seconds; if the Teensy stops seeing it, inspect cabling or power.

Adjust `ESP_RX_PIN`/`ESP_TX_PIN` in the ESP32 sketch if you rewire to alternate pins.

## Branches
- `main`: Production-ready code
- `develop`: Active development
- `feature/<name>`, `bugfix/<name>`, `hotfix/<name>`: Use Git Flow

## Commit Convention
```text
<type>: <short description>
```
Example: `fix: LED flickering on pin 6`
