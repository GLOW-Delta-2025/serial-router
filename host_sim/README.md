# Host-Side Protocol Simulator

`main.cpp` wires a virtual MAC, Teensy router, and ESP device together using the mock serial ports so you can exercise the command flow without hardware. Build with `g++ -std=c++17 -Wall -Wextra -pedantic main.cpp base_connector.cpp -o host_sim_demo` and run `./host_sim_demo` to replay the scripted sequence covering IDs 101–105 and confirmations 201–205.


--

This one is for development purposes. First develop logic in this simulation. Then translate into .ino files for Arduino IDE. And only after that flash into devices and test there. 

Can avoid it if you have two working USB ports and can open two serial monitors. 

I only have one on a mac. Due to USB-C connector issues no option to connect both at the same time. 