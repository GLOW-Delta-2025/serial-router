// ESP32 Serial Bridge: Auto responder + USB monitor
// - Forwards data to/from Teensy
// - Replies automatically: replaces "REQUEST" with "CONFIRM"
//   e.g. !!MASTER:REQUEST:MAKE_STAR## → !!MASTER:CONFIRM:MAKE_STAR##

#define RXD2 16   // RX from Teensy TX
#define TXD2 17   // TX to Teensy RX
#define SERIAL2_BAUD 9600

String buffer; // buffer for incoming message from Teensy

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  Serial2.begin(SERIAL2_BAUD, SERIAL_8N1, RXD2, TXD2);
  delay(50);

  Serial.println("ESP32 Serial Auto-Responder ready.");
  Serial.println("Will convert any REQUEST → CONFIRM and send back.");
}

void loop() {
  // ----- PC → Teensy passthrough -----
  while (Serial.available()) {
    char c = Serial.read();
    Serial2.write(c);
    Serial.write(c); // echo locally
  }

  // ----- Teensy → PC + Auto Response -----
  while (Serial2.available()) {
    char c = Serial2.read();
    Serial.write(c);  // show everything on PC
    buffer += c;

    // check for end of message
    if (buffer.endsWith("##")) {

      // if message contains REQUEST, respond with CONFIRM
      if (buffer.indexOf("REQUEST") != -1) {
        String reply = buffer;
        reply.replace("REQUEST", "CONFIRM");

        Serial.println();
        Serial.print("[Auto→Teensy] ");
        Serial.println(reply);

        Serial2.print(reply); // send modified message back to Teensy
      }

      buffer = ""; // clear for next message
    }
  }
}
