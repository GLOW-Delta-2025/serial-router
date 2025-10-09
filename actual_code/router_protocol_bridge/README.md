# Router Protocol Bridge

This directory contains the Teensy and ESP firmware sketches that relay the documented `!!…##` protocol between the MAC host and the downstream controllers. Flash `teensy_router.ino` to the Teensy 4.1, `esp_router.ino` to the ESP32, and connect Teensy Serial1 (pins 0/1) to the ESP32's UART (GPIO26/GPIO25). Use a 115200 baud USB serial monitor on the Teensy to type MAC-style frames and observe routing/debug output.

Then can send commands from communication agreement document to teensy over serial monitor. 
Current implementation can handle requests 101-105 and confirmations 201-205. (Command 104 is intended to come not from mac and doesn't return. Will need some more tests with serial open on ESP as well. For now only one serial monitor for one device is available.)

---

Serial monitor output sample: 

[ROUTER] MAC message: [ARM#]:REQUEST:SEND_STAR
[ROUTER] -> ESP !!MASTER:[ARM#]:REQUEST:SEND_STAR##
[ROUTER] ESP message: MASTER:CONFIRM:SEND_STAR{status=ok}
[ROUTER] -> MAC !![ARM#]:MASTER:CONFIRM:SEND_STAR{status=ok}##
!![ARM#]:MASTER:CONFIRM:SEND_STAR{status=ok}##
[ROUTER] ESP message: MASTER:REQUEST:STAR_ARRIVED{arm=[ARM#],status=arrived}
[ROUTER] -> MAC !![ARM#]:MASTER:REQUEST:STAR_ARRIVED{arm=[ARM#],status=arrived}##
!![ARM#]:MASTER:REQUEST:STAR_ARRIVED{arm=[ARM#],status=arrived}##
[ROUTER] MAC message: [ARM#]:REQUEST:CANCEL_STAR
[ROUTER] -> ESP !!MASTER:[ARM#]:REQUEST:CANCEL_STAR##
[ROUTER] ESP message: MASTER:CONFIRM:CANCEL_STAR{status=ok}
[ROUTER] -> MAC !![ARM#]:MASTER:CONFIRM:CANCEL_STAR{status=ok}##
!![ARM#]:MASTER:CONFIRM:CANCEL_STAR{status=ok}##
[ROUTER] MAC message: MASTER:REQUEST:STAR_ARRIVED{[SPEED],[COLO R], [BRIGHTNESS], [SIZE]}
[ROUTER] -> ESP !!MASTER:MASTER:REQUEST:STAR_ARRIVED{[SPEED],[COLO R], [BRIGHTNESS], [SIZE]}##
[ROUTER] MAC message: CENTER:REQUEST:ADD_STAR{[SPEED],[COLOR], [BRIGHTNESS], [SIZE]}
[ROUTER] -> ESP !!MASTER:CENTER:REQUEST:ADD_STAR{[SPEED],[COLOR], [BRIGHTNESS], [SIZE]}##
[ROUTER] ESP message: MASTER:CONFIRM:ADD_STAR{status=buffered}
[ROUTER] -> MAC !!CENTER:MASTER:CONFIRM:ADD_STAR{status=buffered}##
!!CENTER:MASTER:CONFIRM:ADD_STAR{status=buffered}##
[ROUTER] MAC message: TOP:REQUEST:ADD_STAR{[SPEED],[COLOR], [BRIGHTNESS], [SIZE]}
[ROUTER] -> ESP !!MASTER:TOP:REQUEST:ADD_STAR{[SPEED],[COLOR], [BRIGHTNESS], [SIZE]}##
[ROUTER] ESP message: MASTER:CONFIRM:ADD_STAR{status=buffered}
[ROUTER] -> MAC !!TOP:MASTER:CONFIRM:ADD_STAR{status=buffered}##
!!TOP:MASTER:CONFIRM:ADD_STAR{status=buffered}##
