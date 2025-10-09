#pragma once

#include <string>
#include <vector>

struct MessageParts {
	std::vector<std::string> addresses; // source/destination chain (eg. {"MASTER","ARM1"})
	std::string type;                    // REQUEST, CONFIRM, etc.
	std::string command;                 // MAKE_STAR, STAR_ARRIVED, ...
	std::string parameters;              // raw parameter block without braces
};

// Build a protocol-compliant message string `!!...##` from structured parts.
std::string build_message(const MessageParts& parts);

// Parse a protocol message into structured components; returns false if malformed.
bool parse_message(const std::string& text, MessageParts& out);

// Provide a short human-readable summary useful for logging.
std::string pretty_message(const MessageParts& parts);
