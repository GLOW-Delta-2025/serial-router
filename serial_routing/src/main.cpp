#include <Arduino.h>
#include <string>

// ---------- Forward declarations ----------
void routeFromMac(const std::string &msg);
void routeFromArm(HardwareSerial &armPort, int armNumber);

// ---------- Define serial ports ----------
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
  switch (armNumber) {
    case 1: arm1.print(message.c_str()); break;
    case 2: arm2.print(message.c_str()); break;
    case 3: arm3.print(message.c_str()); break;
    case 4: arm4.print(message.c_str()); break;
    case 5: arm5.print(message.c_str()); break;
    default: break;
  }
}

void sendToCenterpiece(const std::string &message) {
  Centerpiece.print(message.c_str());
}

// ---------- Routing Logic ----------
void routeFromMac(const std::string &msg) {
  if (msg.rfind("!!ARM1", 0) == 0) { sendToArm(1, msg); mac.print("[→ ARM1] "); mac.println(msg.c_str()); }
  else if (msg.rfind("!!ARM2", 0) == 0) { sendToArm(2, msg); mac.print("[→ ARM2] "); mac.println(msg.c_str()); }
  else if (msg.rfind("!!ARM3", 0) == 0) { sendToArm(3, msg); mac.print("[→ ARM3] "); mac.println(msg.c_str()); }
  else if (msg.rfind("!!ARM4", 0) == 0) { sendToArm(4, msg); mac.print("[→ ARM4] "); mac.println(msg.c_str()); }
  else if (msg.rfind("!!ARM5", 0) == 0) { sendToArm(5, msg); mac.print("[→ ARM5] "); mac.println(msg.c_str()); }
  else if (msg.rfind("!!CENTERPIECE", 0) == 0) { sendToCenterpiece(msg); mac.print("[→ CENTERPIECE] "); mac.println(msg.c_str()); }
}


// ---------- Routing from Arms ----------
void routeFromArm(HardwareSerial &armPort, int armNumber) {
  static std::string armBuffers[7];
  std::string &buffer = armBuffers[armNumber];

  while (armPort.available()) {
    char ch = armPort.read();
    buffer += ch;

    // Controleer op volledig bericht
    if (buffer.find("!!") != std::string::npos &&
        buffer.size() >= 2 &&
        buffer.substr(buffer.size() - 2) == "##") {

      // Laat zien van welke arm het komt
      mac.print("[← ARM");
      mac.print(armNumber);
      mac.print("] ");
      mac.println(buffer.c_str());

      // Stuur het pure command terug naar de Mac
      buffer.clear(); // ✅ clear buffer for next message
      armPort.flush(); // ✅ empty any leftover bytes
    }
  }
}

// ---------- Main ----------
void setup() {
  SetupSerialRouting();

  // Wacht tot de USB-serial actief is
  while (!mac) {
    ; // doe niets tot de verbinding klaar is
  }

  mac.println("Serial router ready. Type !![ARMx]COMMAND## en druk op Enter.");
}

void loop() {
  static std::string macBuffer;

  // Lees van Mac
  if (mac.available()) {
    char ch = mac.read();
    macBuffer += ch;

    if (macBuffer.find("!!") != std::string::npos &&
        macBuffer.size() >= 2 &&
        macBuffer.substr(macBuffer.size() - 2) == "##") {

      routeFromMac(macBuffer);
      macBuffer.clear();
    }
  }

  // Lees van armen en centerpiece
  routeFromArm(arm1, 1);
  routeFromArm(arm2, 2);
  routeFromArm(arm3, 3);
  routeFromArm(arm4, 4);
  routeFromArm(arm5, 5);
  routeFromArm(Centerpiece, 6);
}
