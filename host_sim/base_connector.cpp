#include "base_connector.hpp"

#include <algorithm>
#include <sstream>

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

std::string build_message(const MessageParts& parts) {
	std::ostringstream oss;
	oss << "!!";
	if (!parts.addresses.empty()) {
		oss << join(parts.addresses, ':') << ':';
	}
	oss << parts.type << ':' << parts.command;
	if (!parts.parameters.empty()) {
		oss << '{' << parts.parameters << '}';
	}
	oss << "##";
	return oss.str();
}

bool parse_message(const std::string& text, MessageParts& out) {
	if (text.size() < 6U || text.rfind("!!", 0) != 0 || text.substr(text.size() - 2U) != "##") {
		return false;
	}

	const std::string core = text.substr(2U, text.size() - 4U); // strip !! and ##
	std::string header = core;
	std::string parameters;
	const size_t brace_pos = core.find('{');
	if (brace_pos != std::string::npos) {
		const size_t closing = core.rfind('}');
		if (closing == std::string::npos || closing < brace_pos) {
			return false;
		}
		header = core.substr(0, brace_pos);
		parameters = core.substr(brace_pos + 1U, closing - brace_pos - 1U);
		if (closing + 1U != core.size()) {
			return false; // trailing characters after closing brace
		}
	}

	const auto tokens = split(header, ':');
	if (tokens.size() < 2U) {
		return false;
	}

	MessageParts result;
	result.command = tokens.back();
	result.type = tokens[tokens.size() - 2U];
	result.parameters = parameters;
	if (tokens.size() > 2U) {
		result.addresses.assign(tokens.begin(), tokens.end() - 2U);
	} else {
		result.addresses.clear();
	}

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
