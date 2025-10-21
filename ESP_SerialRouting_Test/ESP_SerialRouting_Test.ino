// Basic ESP Serial Command Receiver
// Listens for commands like: !!MASTER:[ARM3]:REQUEST:MAKE_STAR{120,RED,90,6}##
// Responds with: !!MASTER:CONFIRM:MAKE_STAR##

String inputBuffer = "";  // To store incoming serial data
bool receiving = false;   // Whether we’re currently receiving a command

void setup() {
  Serial.begin(115200);  // For debug output to PC
  Serial2.begin(9600);   // Or Serial1 depending on your ESP board (for Teensy link)
  
  Serial.println("ESP ready to receive from Teensy...");
}

void loop() {
  // Read from Teensy (Serial2)
  if (Serial2.available()) {
    char c = Serial2.read();

    if (!receiving) {
      // Start of message
      if (c == '!' && Serial2.peek() == '!') {
        receiving = true;
        inputBuffer = "!!";
        Serial2.read();  // consume the 2nd '!'
      }
    } else {
      inputBuffer += c;

      // End of message
      if (inputBuffer.endsWith("##")) {
        processCommand(inputBuffer);
        inputBuffer = "";
        receiving = false;
      }
    }
  }
}

// Function to process received message
void processCommand(const String &message) {
  Serial.print("Received: ");
  Serial.println(message);

  // Example: !!MASTER:[ARM3]:REQUEST:MAKE_STAR{120,RED,90,6}##
  if (message.indexOf("REQUEST:MAKE_STAR") != -1) {
    String response = "!!MASTER:CONFIRM:MAKE_STAR##";
    Serial2.print(response);
    Serial.print("Sent response: ");
    Serial.println(response);
  } else {
    Serial.println("Unknown command.");
  }
}
