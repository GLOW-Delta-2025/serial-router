#include "base_connector.hpp"

#include <algorithm>
#include <sstream>
#include <map>

// Use the shared CommandLib for protocol building/parsing
// Include path expected via compiler flag, e.g. -I../../CommandLibary/CommandLibary
#include "CmdLib.h"

namespace {
	std::vector<std::string> split(const std::string& text, char delimiter) {
		std::vector<std::string> parts;
		std::string current;
		for (char ch : text) {
			if (ch == delimiter) {
				parts.push_back(current);
				current.clear();
			} else {
				current.push_back(ch);
			}
		}
		parts.push_back(current);
		return parts;
	}

	std::string join(const std::vector<std::string>& parts, char delimiter) {
		std::ostringstream oss;
		for (size_t i = 0; i < parts.size(); ++i) {
			if (i > 0) {
				oss << delimiter;
			}
			oss << parts[i];
		}
		return oss.str();
	}
}

// Helper: parse "k=v,flag,key=val" into a map to interop with cmdlib
static std::map<std::string, std::string> parse_named_params(const std::string& params) {
	std::map<std::string, std::string> out;
	if (params.empty()) return out;
	const auto tokens = split(params, ',');
	for (const auto& t : tokens) {
		auto eq = t.find('=');
		if (eq == std::string::npos) {
			// flag-only
			out[ t ] = "";
		} else {
			out[ t.substr(0, eq) ] = t.substr(eq + 1);
		}
	}
	return out;
}

std::string build_message(const MessageParts& parts) {
	cmdlib::Command cmd;
	for (const auto& h : parts.addresses) cmd.headers.push_back(h);
	cmd.msgKind = parts.type;
	cmd.command = parts.command;
	for (const auto& kv : parse_named_params(parts.parameters)) {
		cmd.setNamed(kv.first, kv.second);
	}
	return cmd.toString();
}

bool parse_message(const std::string& text, MessageParts& out) {
	cmdlib::Command cmd;
	std::string error;
	if (!cmdlib::parse(text, cmd, error)) {
		return false;
	}
	MessageParts result;
	result.addresses = cmd.headers;
	result.type = cmd.msgKind;
	result.command = cmd.command;
	// Rebuild parameter string from named map (order not guaranteed but content-equivalent)
	std::vector<std::string> kvs;
	kvs.reserve(cmd.namedParams.size());
	for (const auto& kv : cmd.namedParams) {
		if (kv.second.empty()) kvs.push_back(kv.first);
		else kvs.push_back(kv.first + "=" + kv.second);
	}
	result.parameters = kvs.empty() ? std::string() : join(kvs, ',');
	out = std::move(result);
	return true;
}

std::string pretty_message(const MessageParts& parts) {
	std::ostringstream oss;
	if (!parts.addresses.empty()) {
		oss << join(parts.addresses, ':') << ':';
	}
	oss << parts.type << ':' << parts.command;
	if (!parts.parameters.empty()) {
		oss << '{' << parts.parameters << '}';
	}
	return oss.str();
}
