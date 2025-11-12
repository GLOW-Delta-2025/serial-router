# Host-Side Protocol Simulator

This simulator wires a virtual MAC, Teensy router, and ESP device together using in-process mock serial ports so you can exercise the command flow without hardware.

- Protocol framing/parsing is provided by the shared CommandLibary (`CmdLib.h`).
- Topology: MAC ⇄ (USB CDC) ⇄ Teensy Router ⇄ (UART) ⇄ ESP — all emulated via `MockSerial`.
- Scenarios: MAKE_STAR, SEND_STAR (with STAR_ARRIVED), CANCEL_STAR, ADD_STAR.

For how this supports acceptance testing and KPI coverage (timing, resource use, performance), see `tests/acceptance/README.md`.

## Build

From this folder:

```bash
# Relative include path to the CommandLibary (inside the workspace)
g++ -std=c++17 -Wall -Wextra -pedantic \
	-I../../CommandLibary/CommandLibary \
	main.cpp base_connector.cpp -o host_sim_demo

# Or with absolute paths (edit for your machine)
g++ -std=c++17 -Wall -Wextra -pedantic \
	-I/Users/red/Documents/GLOW_2025/CommandLibary/CommandLibary \
	/Users/red/Documents/GLOW_2025/serial-router/host_sim/main.cpp \
	/Users/red/Documents/GLOW_2025/serial-router/host_sim/base_connector.cpp \
	-o /Users/red/Documents/GLOW_2025/serial-router/host_sim/host_sim_demo
```

## Run

```bash
./host_sim_demo
```

### Expected output (abridged)

```
MAC sending !!ARM1:REQUEST:MAKE_STAR{...}##
ESP received MASTER:ARM1:REQUEST:MAKE_STAR{...}
ESP sending !!MASTER:CONFIRM:MAKE_STAR##
MAC received ARM1:MASTER:CONFIRM:MAKE_STAR -> !!ARM1:MASTER:CONFIRM:MAKE_STAR##
...
ESP sending !!MASTER:REQUEST:STAR_ARRIVED{arm=ARM1,...}##
MAC received ARM1:MASTER:REQUEST:STAR_ARRIVED{...} -> !!ARM1:MASTER:REQUEST:STAR_ARRIVED{...}##
```

Note: Named parameter order may differ between lines due to map iteration order; semantics are preserved.

## Extend the simulation

- Edit `main.cpp` and extend the `send_from_mac(...)` calls to add new devices/commands.
- `VirtualRouter` and `VirtualEsp` provide a simple model you can adapt for error injection, noise, and partial frames.

## Troubleshooting

- Header not found: ensure the `-I` include path points to `CommandLibary/CommandLibary` where `CmdLib.h` resides.
- Requires a C++17 compiler (Clang or GCC) and standard library.
- If you need deterministic parameter ordering for log comparisons, we can add key-sorted serialization in the simulator.

---

This is primarily for development: iterate logic here first, then translate into `.ino` files for Arduino IDE, flash devices, and run hardware tests. If you have two USB serial ports and monitors, you can skip the simulator; this project assumes a single-port macOS setup.