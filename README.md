# Serial Router Playground

Firmware, simulation scaffolding, and documentation for routing the GLOW 2025 choreography protocol between the Mac controller, a Teensy 4.1 router, and ESP32-based device endpoints.

## Layout

- `states_diagram/` – Markdown spec for the protocol plus a Python/Graphviz generator that emits DOT/SVG diagrams.
- `actual_code/` – Arduino sketches for Teensy and ESP firmware (`updated_router_protocol_bridge/` holds the latest command-complete pair; 
- `host_sim/` – C++ harness that simulates the MAC⇄Teensy⇄ESP exchange using mock serial ports.

## Repository Structure

```
serial-router/
├─ README.md                         # Project overview and quick-start workflow
├─ actual_code/
│  ├─ one_bit_connector_tx_rx/       # Original Teensy↔ESP link probes and supporting notes
│  └─ updated_router_protocol_bridge/        # Production Teensy router + ESP responder sketches
├─ host_sim/
│  ├─ main.cpp                       # Virtual MAC/Router/ESP harness exercising the protocol
│  ├─ base_connector.*               # Shared helpers for building/parsing protocol frames
│  └─ mock_serial.hpp                # In-memory serial port used by the simulator
└─ states_diagram/
   ├─ teensy_esp_router_states.md    # Authoritative protocol specification (commands, states)
   ├─ diagram_generator.py           # Markdown-to-DOT/SVG renderer for the spec
   └─ output/                        # Generated Graphviz artifacts (DOT/SVG)
```

## Quick Start

1. Generate the protocol diagram: `cd states_diagram && python3 diagram_generator.py`
2. Build the host simulator: `cd host_sim && g++ -std=c++17 -Wall -Wextra -pedantic main.cpp base_connector.cpp -o host_sim_demo`
3. Flash firmware (prefer `actual_code/updated_router_protocol_bridge/` for the full command set):
   - Teensy 4.1: open `actual_code/updated_router_protocol_bridge/teensy_router.ino` in the Arduino IDE and upload with FQBN `teensy:avr:teensy41`.
   - ESP32: open `actual_code/updated_router_protocol_bridge/esp_router.ino`, select the appropriate ESP32 board, and upload (pins GPIO26/25 map to Teensy RX1/TX1).

## Manual Testing

- Open the Teensy USB Serial monitor at 115200 baud and send protocol frames (for example `!!ARM1:REQUEST:MAKE_STAR{speed=3,color=red,brightness=80,size=10}##`).
- Watch the Teensy console for `[ROUTER]` debug lines and the echoed frames that return to the MAC side.
- Optional: connect to the ESP’s USB serial port to view its `[ESP]` logs.

## Contributing Checklist

- Keep command/state names identical to the specification tables (including underscores and capitalization).
- When adding new protocol elements, update `states_diagram/teensy_esp_router_states.md` and regenerate the diagram.
- Extend the host simulator alongside firmware changes so desktop tests cover new flows.
- Run `git status` and review the generated artifacts before committing.
