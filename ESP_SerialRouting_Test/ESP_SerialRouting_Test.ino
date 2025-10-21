// ESP32 Serial Bridge: USB <-> Serial2
// Stuur alle bytes direct tussen USB (Serial) en UART2 (Serial2)

#define RXD2 16  // RX pin voor Serial2 (ESP32 ontvangt hier)
#define TXD2 17  // TX pin voor Serial2 (ESP32 zendt hier)
#define SERIAL2_BAUD 9600

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  Serial.println("ESP32 Serial Bridge actief!");
  Serial2.begin(SERIAL2_BAUD, SERIAL_8N1, RXD2, TXD2);

  delay(50);
  Serial.println("[INFO] Verbinding met Teensy gestart...");
}

// Bufferloze, directe doorsturing
void loop() {
  // USB → Teensy
  while (Serial.available()) {
    char c = Serial.read();
    Serial2.write(c);  // direct doorsturen zonder newline
    Serial.write(c);   // echo terug naar monitor
  }

  // Teensy → USB
  while (Serial2.available()) {
    char c = Serial2.read();
    Serial2.write(c);
    Serial.write(c);   // direct doorsturen naar monitor
  }
}
