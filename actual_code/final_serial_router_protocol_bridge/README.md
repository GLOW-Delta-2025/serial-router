# Router Protocol Bridge

This directory contains the Teensy and ESP firmware sketches that relay the documented `!!…##` protocol between the MAC host and the downstream controllers. Flash `teensy_router.ino` to the Teensy 4.1, `esp_router.ino` to the ESP32, and connect Teensy Serial1 (pins 0/1) to the ESP32's UART (GPIO26/GPIO25). Use a 115200 baud USB serial monitor on the Teensy to type MAC-style frames and observe routing/debug output.

The bridge currently supports:
- MAC requests 101-111 and broadcast requests 301-304, with corresponding confirmations 201-211 and 401-404 routed back to the host.
- ESP-side scripted confirmations for the same commands, including simulated payload follow-ups (`STAR_ARRIVED` after `SEND_STAR`, `CLIMAX_READY` after `BUILDUP_CLIMAX_CENTER`).
- Router acknowledgements for device-originated notifications 104 `STAR_ARRIVED`, 107 `CLIMAX_READY`, and 500 `COMM_ERROR` so manual tests can see the round-trip without additional tooling.
- Frame parsing tolerant of stray whitespace before the closing `##` markers, matching recent firmware behavior.

---
