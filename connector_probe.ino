#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdint>
#include <cstddef>

class StubSerial {
public:
	void begin(unsigned long) {}
	void setTX(uint8_t) {}
	void setRX(uint8_t) {}
	size_t write(uint8_t) { return 1U; }
	int available() { return 0; }
	int read() { return 0; }
	void print(const char*) {}
	void print(char) {}
	void println(const char*) {}
	void println(char) {}
	void println(int, int = 10) {}
	void print(int, int = 10) {}
	void print(unsigned long, int = 10) {}
	void println(unsigned long, int = 10) {}
	void flush() {}
	explicit operator bool() const { return true; }
};

static StubSerial Serial;
static StubSerial Serial1;

inline unsigned long millis() { return 0; }
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}

constexpr uint8_t LED_BUILTIN = 13;
constexpr uint8_t OUTPUT = 1;
constexpr uint8_t HIGH = 1;
constexpr uint8_t LOW = 0;
constexpr int BIN = 2;
constexpr int HEX = 16;
#endif

// Teensy 4.1: Serial probe for ESP32 connection over pins 0 (RX1) and 1 (TX1)
// Sends a repeating bit pattern over Serial1 and reports back over USB Serial.
// If no counterpart responds within a timeout window, a single debug message is printed.

constexpr uint32_t BIT_PERIOD_MS = 500;          // Interval between bit toggles
constexpr uint32_t CONNECTION_TIMEOUT_MS = 5000; // Time to wait before flagging no connection
constexpr uint8_t SERIAL1_TX_PIN = 1;             // Teensy TX1 pin number (for reference only)
constexpr uint8_t SERIAL1_RX_PIN = 0;             // Teensy RX1 pin number (for reference only)

bool bitState = false;        // Current bit to send (false -> 0, true -> 1)
uint32_t lastSendMs = 0;      // Millis timestamp for the last transmission
uint32_t lastActivityMs = 0;  // Millis timestamp for the last receive event
bool connectionSeen = false;  // Tracks if we've ever seen inbound data
bool debugReported = false;   // Ensures we only log the "no connection" message once

// Helper to send the current bit over Serial1 and mirror status to the debug console.
void transmitBit(bool bitValue) {
	const char payload = bitValue ? '1' : '0';
	Serial1.write(static_cast<uint8_t>(payload));
	Serial1.flush();

	Serial.print("TX1 sent bit: ");
	Serial.println(payload);

	// Optional visual indicator on the built-in LED for quick sanity checks.
#ifdef LED_BUILTIN
	digitalWrite(LED_BUILTIN, bitValue ? HIGH : LOW);
#endif
}

void setup() {
#ifdef LED_BUILTIN
	pinMode(LED_BUILTIN, OUTPUT);
#endif

	Serial.begin(115200);
	while (!Serial && millis() < 2000) {
		// Wait briefly for the USB serial console to come up (optional).
	}

	Serial.println("Teensy Serial1 bit sender starting up...");
	Serial.print("Serial1 TX pin: ");
	Serial.println(SERIAL1_TX_PIN);
	Serial.print("Serial1 RX pin: ");
	Serial.println(SERIAL1_RX_PIN);

	Serial1.begin(115200);
	Serial1.write('H');
	Serial1.write('I');
	Serial1.flush();
	lastSendMs = millis();
	lastActivityMs = lastSendMs;
}

void loop() {
	const uint32_t now = millis();

	// Periodically toggle and transmit the bit pattern.
	if (now - lastSendMs >= BIT_PERIOD_MS) {
		transmitBit(bitState);
		bitState = !bitState;
		lastSendMs = now;
	}

	// Capture any inbound data from the counterpart and treat that as proof of life.
	while (Serial1.available() > 0) {
		const int incoming = Serial1.read();
		if (incoming < 0) {
			continue;
		}
		lastActivityMs = now;
		connectionSeen = true;
		debugReported = false; // Reset so we can warn again if the link drops later.

		Serial.print("RX1 received byte: '");
		Serial.print(static_cast<char>(incoming & 0xFF));
		Serial.print("' (0x");
		Serial.print(incoming & 0xFF, HEX);
		Serial.println(")");
	}

	// Emit a single debug warning if we've never heard back within the timeout window.
	if (!connectionSeen && !debugReported && (now - lastActivityMs) > CONNECTION_TIMEOUT_MS) {
		Serial.println("Debug: no device detected on Serial1 (pins 0/1). Check ESP32 wiring.");
		debugReported = true;
	}
}
