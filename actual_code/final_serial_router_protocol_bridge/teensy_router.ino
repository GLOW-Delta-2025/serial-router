#include <Arduino.h>
#include <ctype.h>

struct MessageParts {
	String addresses[4];
	size_t addressCount = 0;
	String type;
	String command;
	String parameters;
};

struct PendingEntry {
	String commandKey;
	String device;
};

static constexpr uint32_t USB_BAUD = 115200;
static constexpr uint32_t LINK_BAUD = 115200;
static constexpr size_t MAX_PENDING = 16;
static constexpr size_t MAX_BUFFER_CHARS = 384;

static bool TRACE_SERIAL1_RX_BYTES = true;  // Toggle raw ESP->Teensy byte logging
static bool TRACE_USB_RX_BYTES = false;     // Toggle raw MAC->Teensy byte logging

static String macBuffer;
static String espBuffer;
static PendingEntry pending[MAX_PENDING];
static size_t pendingCount = 0;

// -------------------------------------------------------------------------------------------------
// Protocol helpers
// -------------------------------------------------------------------------------------------------

static void trimToken(String& token) {
	token.trim();
}

static String normalizeIdentifier(const String& text) {
	String normalized;
	normalized.reserve(text.length());
	for (size_t i = 0; i < static_cast<size_t>(text.length()); ++i) {
		char c = text.charAt(static_cast<unsigned int>(i));
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
			continue;
		}
		normalized += static_cast<char>(toupper(static_cast<unsigned char>(c)));
	}
	return normalized;
}

static bool parseMessage(const String& raw, MessageParts& out) {
	if (!raw.startsWith("!!") || !raw.endsWith("##")) {
		return false;
	}

	MessageParts result;
	String core = raw.substring(2, raw.length() - 2);
	core.trim();
	String header = core;
	String parameters;

	const int braceStart = core.indexOf('{');
	if (braceStart >= 0) {
		const int braceEnd = core.lastIndexOf('}');
		if (braceEnd < braceStart) {
			return false;
		}
		header = core.substring(0, braceStart);
		parameters = core.substring(braceStart + 1, braceEnd);
		const size_t coreLength = static_cast<size_t>(core.length());
		const size_t closingOffset = static_cast<size_t>(braceEnd + 1);
		if (closingOffset != coreLength) {
			return false; // Trailing characters after closing brace
		}
	}

	String tokens[8];
	int tokenCount = 0;
	int start = 0;
	const size_t headerLength = static_cast<size_t>(header.length());
	for (size_t i = 0; i < headerLength; ++i) {
		if (header.charAt(static_cast<unsigned int>(i)) == ':') {
			tokens[tokenCount++] = header.substring(start, static_cast<int>(i));
			start = static_cast<int>(i) + 1;
			if (tokenCount >= 8) {
				return false;
			}
		}
	}
tokens[tokenCount++] = header.substring(start);

	if (tokenCount < 2) {
		return false;
	}

	for (int i = 0; i < tokenCount; ++i) {
		trimToken(tokens[i]);
	}

	result.type = tokens[tokenCount - 2];
	result.command = tokens[tokenCount - 1];
	result.parameters = parameters;

	const int addrCount = tokenCount - 2;
	if (addrCount > 0) {
		if (addrCount > 4) {
			return false;
		}
		for (int i = 0; i < addrCount; ++i) {
			result.addresses[i] = tokens[i];
		}
		result.addressCount = static_cast<size_t>(addrCount);
	}

	out = result;
	return true;
}

static String buildMessage(const MessageParts& parts) {
	String message = "!!";
	for (size_t i = 0; i < parts.addressCount; ++i) {
		message += parts.addresses[i];
		message += ':';
	}
	message += parts.type;
	message += ':';
	message += parts.command;
	if (parts.parameters.length() > 0) {
		message += '{';
		message += parts.parameters;
		message += '}';
	}
	message += "##";
	return message;
}

static void logMessage(const char* prefix, const MessageParts& parts) {
	String detail;
	if (parts.addressCount > 0) {
		detail = parts.addresses[0];
		for (size_t i = 1; i < parts.addressCount; ++i) {
			detail += ':';
			detail += parts.addresses[i];
		}
		detail += ':';
	}
	detail += parts.type;
	detail += ':';
	detail += parts.command;
	if (parts.parameters.length() > 0) {
		detail += '{';
		detail += parts.parameters;
		detail += '}';
	}

	Serial.print(prefix);
	Serial.println(detail);
}

// -------------------------------------------------------------------------------------------------
// Pending command tracking (for routing confirmations back to the correct device)
// -------------------------------------------------------------------------------------------------

static void storePendingCommand(const String& command, const String& device) {
	if (device.length() == 0) {
		return;
	}

	const String key = normalizeIdentifier(command);
	if (key.length() == 0) {
		return;
	}

	for (size_t i = 0; i < pendingCount; ++i) {
		if (pending[i].commandKey == key) {
			pending[i].device = device;
			return;
		}
	}

	if (pendingCount < MAX_PENDING) {
		pending[pendingCount].commandKey = key;
		pending[pendingCount].device = device;
		++pendingCount;
		return;
	}

	// Simple FIFO eviction when the table is full.
	for (size_t i = 1; i < pendingCount; ++i) {
		pending[i - 1] = pending[i];
	}
	pending[pendingCount - 1].commandKey = key;
	pending[pendingCount - 1].device = device;
}

static String resolveDeviceForCommand(const String& command) {
	const String key = normalizeIdentifier(command);
	if (key.length() == 0) {
		return String();
	}
	for (size_t i = 0; i < pendingCount; ++i) {
		if (pending[i].commandKey == key) {
			return pending[i].device;
		}
	}
	return String();
}

// -------------------------------------------------------------------------------------------------
// Forwarding helpers
// -------------------------------------------------------------------------------------------------

static void forwardToEsp(const MessageParts& parts) {
	MessageParts outbound = parts;
	// Insert MASTER at the front of the address chain when routing to the ESP device.
	for (size_t i = outbound.addressCount; i > 0; --i) {
		outbound.addresses[i] = outbound.addresses[i - 1];
	}
	outbound.addresses[0] = "MASTER";
	outbound.addressCount += 1;

	const String encoded = buildMessage(outbound);
	Serial.print("[ROUTER] -> ESP ");
	Serial.println(encoded);
	Serial1.write(encoded.c_str(), encoded.length());
	Serial1.flush();
}

static void forwardToMac(const MessageParts& parts, const String& fallbackDevice) {
	MessageParts outbound = parts;
	String source = resolveDeviceForCommand(parts.command);
	if (source.length() == 0) {
		source = fallbackDevice;
	}
	if (source.length() == 0) {
		source = "DEVICE";
	}

	outbound.addressCount = 2;
	outbound.addresses[0] = source;
	outbound.addresses[1] = "MASTER";

	const String encoded = buildMessage(outbound);
	Serial.print("[ROUTER] -> MAC ");
	Serial.println(encoded);
	Serial.write(encoded.c_str(), encoded.length());
	Serial.write('\n');
}

static void sendAckToEsp(const String& deviceTag, const String& command, const char* status) {
	MessageParts ack;
	ack.addressCount = 1;
	ack.addresses[0] = deviceTag.length() > 0 ? deviceTag : "MASTER";
	ack.type = "CONFIRM";
	ack.command = command;
	ack.parameters = status;

	const String encoded = buildMessage(ack);
	Serial.print("[ROUTER] -> ESP ");
	Serial.println(encoded);
	Serial1.write(encoded.c_str(), encoded.length());
	Serial1.flush();
}

// -------------------------------------------------------------------------------------------------
// Message handlers
// -------------------------------------------------------------------------------------------------

static void handleMacMessage(const String& raw) {
	MessageParts parts;
	if (!parseMessage(raw, parts)) {
		Serial.print("[ROUTER] malformed MAC message: ");
		Serial.println(raw);
		return;
	}

	logMessage("[ROUTER] MAC message: ", parts);

	if (!parts.type.equalsIgnoreCase("REQUEST")) {
		Serial.println("[ROUTER] Ignoring non-REQUEST message from MAC");
		return;
	}

	const String targetDevice = parts.addressCount > 0 ? parts.addresses[0] : String();
	storePendingCommand(parts.command, targetDevice);
	if (parts.command.equalsIgnoreCase("SEND_STAR")) {
		storePendingCommand("STAR_ARRIVED", targetDevice);
	}

	forwardToEsp(parts);
}

static void handleEspMessage(const String& raw) {
	MessageParts parts;
	if (!parseMessage(raw, parts)) {
		Serial.print("[ROUTER] malformed ESP message: ");
		Serial.println(raw);
		return;
	}

	logMessage("[ROUTER] ESP message: ", parts);

	String fallbackDevice;
	if (parts.addressCount > 0) {
		fallbackDevice = parts.addresses[0];
	}

	const String commandKey = normalizeIdentifier(parts.command);

	forwardToMac(parts, fallbackDevice);

	if (commandKey == "STAR_ARRIVED" && parts.type.equalsIgnoreCase("REQUEST")) {
		sendAckToEsp(fallbackDevice, "STAR_ARRIVED", "status=received");
	} else if (commandKey == "CLIMAX_READY" && parts.type.equalsIgnoreCase("REQUEST")) {
		sendAckToEsp(fallbackDevice, "CLIMAX_READY", "status=forwarded");
	} else if (commandKey == "COMM_ERROR") {
		sendAckToEsp(fallbackDevice, "COMM_ERROR", "status=acknowledged");
	}
}

// -------------------------------------------------------------------------------------------------
// Stream buffer processing
// -------------------------------------------------------------------------------------------------

static void processBuffer(String& buffer, bool fromMac) {
	while (true) {
		int start = buffer.indexOf("!!");
		if (start < 0) {
			// Drop stale chatter to avoid unbounded growth.
			if (buffer.length() > MAX_BUFFER_CHARS) {
				buffer.remove(0);
			}
			return;
		}
		if (start > 0) {
			buffer.remove(0, start);
		}

		int end = buffer.indexOf("##", 2);
		if (end < 0) {
			return;
		}

		const String message = buffer.substring(0, end + 2);
		buffer.remove(0, end + 2);

		if (fromMac) {
			handleMacMessage(message);
		} else {
			handleEspMessage(message);
		}
	}
}

static void pumpStream(Stream& stream, String& buffer, bool fromMac, bool traceBytes) {
	while (stream.available() > 0) {
		const char ch = static_cast<char>(stream.read());
		if (traceBytes) {
			Serial.print(fromMac ? "[ROUTER] USB RX 0x" : "[ROUTER] RX1 0x");
			const uint8_t v = static_cast<uint8_t>(ch);
			if (v < 16U) {
				Serial.print('0');
			}
			Serial.print(v, HEX);
			Serial.print(" '");
			if (ch >= 32 && ch <= 126) {
				Serial.print(ch);
			} else {
				Serial.print('.');
			}
			Serial.println('\'');
		}
		if (buffer.length() >= MAX_BUFFER_CHARS) {
			Serial.println("[ROUTER] Buffer overflow, clearing partial message");
			buffer = "";
		}
		buffer += ch;
	}

	if (buffer.length() > 0) {
		processBuffer(buffer, fromMac);
	}
}

// -------------------------------------------------------------------------------------------------
// Arduino entry points
// -------------------------------------------------------------------------------------------------

void setup() {
	Serial.begin(USB_BAUD);
	while (!Serial && millis() < 2000) {
		// Wait for the USB console to attach (optional grace period).
	}

	Serial.println();
	Serial.println("[ROUTER] Teensy protocol bridge starting up...");
	Serial.println("[ROUTER] Type complete protocol frames such as");
	Serial.println("          !!ARM1:REQUEST:MAKE_STAR{speed=3,color=red,brightness=80,size=10}##");
	Serial.println("          and press ENTER to forward them to the ESP.");

	Serial1.begin(LINK_BAUD);
	Serial.println("[ROUTER] Serial1 link ready at 115200 baud");
}

void loop() {
	pumpStream(Serial, macBuffer, true, TRACE_USB_RX_BYTES);
	pumpStream(Serial1, espBuffer, false, TRACE_SERIAL1_RX_BYTES);
}
