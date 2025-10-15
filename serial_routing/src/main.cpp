#include <Arduino.h>
#include <string>

// Forward declarations
void routeFromMac(const std::string &msg);
void routeFromArm(HardwareSerial &armPort);

// Define serial ports
#define mac Serial
#define arm1 Serial1
#define arm2 Serial2
#define arm3 Serial3
#define arm4 Serial4
#define arm5 Serial5
#define Centerpiece Serial6

// ---------- Setup ----------
void SetupSerialRouting() {
  mac.begin(9600);
  arm1.begin(9600);
  arm2.begin(9600);
  arm3.begin(9600);
  arm4.begin(9600);
  arm5.begin(9600);
  Centerpiece.begin(9600);
}

// ---------- Send Helpers ----------
void sendToArm(int armNumber, const std::string &message) {
  std::string routedMsg = "!!MASTER:[ARM" + std::to_string(armNumber) + "]:" + message.substr(2);
  switch (armNumber) {
    case 1: arm1.println(routedMsg.c_str()); break;
    case 2: arm2.println(routedMsg.c_str()); break;
    case 3: arm3.println(routedMsg.c_str()); break;
    case 4: arm4.println(routedMsg.c_str()); break;
    case 5: arm5.println(routedMsg.c_str()); break;
    default: break;
  }
}

void sendToCenterpiece(const std::string &message) {
  std::string routedMsg = "!!MASTER:[CENTERPIECE]:" + message.substr(2);
  Centerpiece.println(routedMsg.c_str());
}

// ---------- Routing Logic ----------
void routeFromMac(const std::string &msg) {
  if (msg.rfind("!![ARM1]", 0) == 0) sendToArm(1, msg);
  else if (msg.rfind("!![ARM2]", 0) == 0) sendToArm(2, msg);
  else if (msg.rfind("!![ARM3]", 0) == 0) sendToArm(3, msg);
  else if (msg.rfind("!![ARM4]", 0) == 0) sendToArm(4, msg);
  else if (msg.rfind("!![ARM5]", 0) == 0) sendToArm(5, msg);
  else if (msg.rfind("!![CENTERPIECE]", 0) == 0) sendToCenterpiece(msg);
}

void routeFromArm(HardwareSerial &armPort) {
  if (armPort.available()) {
    std::string response;
    char ch;
    while (armPort.available()) {
      ch = armPort.read();
      response += ch;
      if (response.size() >= 2 && response.substr(response.size() - 2) == "##") break; // message delimiter
    }
    if (!response.empty()) mac.println(response.c_str());
  }
}

// ---------- Main ----------
void setup() {
  SetupSerialRouting();
}

void loop() {
  // Check Mac input
  if (mac.available()) {
    std::string msg;
    char ch;
    while (mac.available()) {
      ch = mac.read();
      msg += ch;
      if (msg.size() >= 2 && msg.substr(msg.size() - 2) == "##") break; // message delimiter
    }
    if (!msg.empty()) routeFromMac(msg);
  }

  // Check all arm responses
  routeFromArm(arm1);
  routeFromArm(arm2);
  routeFromArm(arm3);
  routeFromArm(arm4);
  routeFromArm(arm5);
  routeFromArm(Centerpiece);
  delay(1000); // Small delay to avoid overwhelming the loop
}
