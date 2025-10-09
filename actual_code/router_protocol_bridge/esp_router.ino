#include <Arduino.h>
#include <ctype.h>

struct MessageParts {
	String addresses[4];
	size_t addressCount = 0;
	String type;
	String command;
	String parameters;
};

using MessageHandler = void (*)(const String& raw);

static constexpr uint32_t USB_BAUD = 115200;
static constexpr uint32_t LINK_BAUD = 115200;
static constexpr int ESP_RX_PIN = 26;
static constexpr int ESP_TX_PIN = 25;
static constexpr size_t MAX_BUFFER_CHARS = 384;

static String linkBuffer;
static String usbBuffer;

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
	const String core = raw.substring(2, raw.length() - 2);
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
			return false;
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
// Outgoing helpers
// -------------------------------------------------------------------------------------------------

static void sendToTeensy(const MessageParts& parts) {
	const String encoded = buildMessage(parts);
	Serial.print("[ESP] -> Teensy ");
	Serial.println(encoded);
	Serial2.write(encoded.c_str(), encoded.length());
}

static void sendConfirm(const String& command, const String& detail) {
	MessageParts confirm;
	confirm.addressCount = 1;
	confirm.addresses[0] = "MASTER";
	confirm.type = "CONFIRM";
	confirm.command = command;
	confirm.parameters = detail;
	sendToTeensy(confirm);
}

static void sendStarArrived(const String& armTag) {
	MessageParts status;
	status.addressCount = 1;
	status.addresses[0] = "MASTER";
	status.type = "REQUEST";
	status.command = "STAR_ARRIVED";
	status.parameters = "arm=" + armTag + ",status=arrived";
	sendToTeensy(status);
}

// -------------------------------------------------------------------------------------------------
// Message handlers
// -------------------------------------------------------------------------------------------------

static void handleLinkMessage(const String& raw) {
	MessageParts parts;
	if (!parseMessage(raw, parts)) {
		Serial.print("[ESP] Malformed incoming frame: ");
		Serial.println(raw);
		return;
	}

	logMessage("[ESP] Teensy -> ESP ", parts);

	const String deviceTag = parts.addressCount > 0 ? parts.addresses[parts.addressCount - 1] : String();
	const String commandKey = normalizeIdentifier(parts.command);

	if (!parts.type.equalsIgnoreCase("REQUEST")) {
		Serial.println("[ESP] Frame ignored (not a REQUEST)");
		return;
	}

	if (commandKey == "MAKE_STAR") {
		sendConfirm("MAKE_STAR", "status=ok");
	} else if (commandKey == "SEND_STAR") {
		sendConfirm("SEND_STAR", "status=ok");
		sendStarArrived(deviceTag.length() > 0 ? deviceTag : "ARM");
	} else if (commandKey == "CANCEL_STAR") {
		sendConfirm("CANCEL_STAR", "status=ok");
	} else if (commandKey == "ADD_STAR") {
		sendConfirm("ADD_STAR", "status=buffered");
	} else {
		Serial.print("[ESP] No scripted response for command: ");
		Serial.println(parts.command);
	}
}

static void handleUsbMessage(const String& raw) {
	Serial.print("[ESP] Manual TX -> Teensy ");
	Serial.println(raw);
	Serial2.write(raw.c_str(), raw.length());
	Serial2.write('\n');
}

// -------------------------------------------------------------------------------------------------
// Stream buffer processing
// -------------------------------------------------------------------------------------------------

static void processBuffer(String& buffer, MessageHandler handler) {
	while (true) {
		int start = buffer.indexOf("!!");
		if (start < 0) {
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
		handler(message);
	}
}

static void pumpStream(Stream& stream, String& buffer, MessageHandler handler) {
	while (stream.available() > 0) {
		const char ch = static_cast<char>(stream.read());
		if (buffer.length() >= MAX_BUFFER_CHARS) {
			Serial.println("[ESP] Buffer overflow, dropping partial data");
			buffer = "";
		}
		buffer += ch;
	}

	if (buffer.length() > 0) {
		processBuffer(buffer, handler);
	}
}

// -------------------------------------------------------------------------------------------------
// Arduino lifecycle
// -------------------------------------------------------------------------------------------------

void setup() {
	Serial.begin(USB_BAUD);
	while (!Serial && millis() < 2000) {
		delay(10);
	}

	Serial.println();
	Serial.println("[ESP] Protocol responder starting...");
	Serial.println("[ESP] Listening on Serial2 (GPIO26=RX, GPIO25=TX)");
	Serial.println("[ESP] You can also paste protocol frames into this USB console");
	Serial.println("      to forward them directly to the Teensy for debugging.");

	Serial2.begin(LINK_BAUD, SERIAL_8N1, ESP_RX_PIN, ESP_TX_PIN);
}

void loop() {
	pumpStream(Serial2, linkBuffer, handleLinkMessage);
	pumpStream(Serial, usbBuffer, handleUsbMessage);
}
