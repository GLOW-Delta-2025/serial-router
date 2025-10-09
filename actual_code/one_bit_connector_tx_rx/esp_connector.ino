#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdint>
#include <cstddef>

class StubSerial {
public:
	void begin(unsigned long, uint32_t = 0, int = -1, int = -1) {}
	size_t write(uint8_t) { return 1U; }
	void flush() {}
	int available() { return 0; }
	int read() { return -1; }
	void print(const char*) {}
	void print(char) {}
	void print(int, int = 10) {}
	void print(unsigned long, int = 10) {}
	void println(const char*) {}
	void println(char) {}
	void println(int, int = 10) {}
	void println(unsigned long, int = 10) {}
	explicit operator bool() const { return true; }
};

static StubSerial Serial;
static StubSerial Serial2;

inline unsigned long millis() { return 0; }
inline void delay(unsigned long) {}

constexpr uint32_t SERIAL_8N1 = 0;
constexpr int BIN = 2;
constexpr int HEX = 16;
#endif

// ESP32 side of the Teensy <-> ESP connector probe.
// Listens for the toggling bit stream sent by the Teensy over a hardware UART,
// echoes the received byte back to confirm the link, and reports status on the
// USB serial console.

constexpr uint32_t LINK_BAUD_RATE = 115200;
constexpr uint32_t INACTIVITY_TIMEOUT_MS = 4000; // Warn if no bytes arrive within this window.
constexpr uint32_t HEARTBEAT_INTERVAL_MS = 2000;

// Default UART2 pins on ESP32 (GPIO26 -> Teensy RX, GPIO25 -> Teensy TX).
// These avoid the WROVER PSRAM pins (16/17) which can spew boot chatter.
constexpr int ESP_RX_PIN = 26;
constexpr int ESP_TX_PIN = 25;

uint32_t lastInboundMs = 0;
bool teensySeen = false;
bool warnedNoSignal = false;
uint32_t lastHeartbeatMs = 0;

void setup() {
	Serial.begin(115200);
	while (!Serial && millis() < 2000) {
		delay(10);
	}

	Serial.println("ESP32 Serial link listener starting up...");
	Serial.print("Serial2 RX pin: ");
	Serial.println(ESP_RX_PIN);
	Serial.print("Serial2 TX pin: ");
	Serial.println(ESP_TX_PIN);

	Serial2.begin(LINK_BAUD_RATE, SERIAL_8N1, ESP_RX_PIN, ESP_TX_PIN);
	lastInboundMs = millis();
	lastHeartbeatMs = lastInboundMs;

	Serial2.write(static_cast<uint8_t>('E'));
	Serial2.write(static_cast<uint8_t>('S'));
	Serial2.write(static_cast<uint8_t>('P'));
	Serial2.flush();
}

void loop() {
	const uint32_t now = millis();

	while (Serial2.available() > 0) {
		const int incoming = Serial2.read();
		if (incoming < 0) {
			continue;
		}

		lastInboundMs = now;
		teensySeen = true;
		warnedNoSignal = false;

		Serial.print("RX2 byte: '");
		Serial.print(static_cast<char>(incoming & 0xFF));
		Serial.print("' (0x");
		Serial.print(incoming & 0xFF, HEX);
		Serial.println(")");

		const uint8_t echoed = static_cast<uint8_t>(incoming & 0xFF);
		Serial2.write(echoed);
		Serial.print("TX2 echoed bit: ");
		Serial.println(static_cast<char>(echoed));
	}

	if (!teensySeen && !warnedNoSignal && (now - lastInboundMs) > INACTIVITY_TIMEOUT_MS) {
		Serial.println("Debug: no signal detected from Teensy on Serial2. Check wiring, grounds, and baud rate.");
		warnedNoSignal = true;
	}

	if ((now - lastHeartbeatMs) > HEARTBEAT_INTERVAL_MS) {
		Serial2.write(static_cast<uint8_t>('?'));
		Serial2.flush();
		lastHeartbeatMs = now;
	}
}
