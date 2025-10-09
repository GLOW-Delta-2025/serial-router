#include "base_connector.hpp"
#include "mock_serial.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class VirtualRouter {
public:
	VirtualRouter(MockSerial& mac_port, MockSerial& device_port)
		: mac_serial(mac_port), device_serial(device_port) {}

	void poll() {
		process_serial(mac_serial, mac_buffer, Direction::MacToRouter);
		process_serial(device_serial, device_buffer, Direction::DeviceToRouter);
	}

private:
	enum class Direction { MacToRouter, DeviceToRouter };

	MockSerial& mac_serial;
	MockSerial& device_serial;
	std::string mac_buffer;
	std::string device_buffer;
	std::unordered_map<std::string, std::string> origin_by_command{};

	void process_serial(MockSerial& port, std::string& buffer, Direction direction) {
		while (port.available() > 0) {
			buffer.push_back(static_cast<char>(port.read()));
		}
		drain_buffer(buffer, direction);
	}

	void drain_buffer(std::string& buffer, Direction direction) {
		while (true) {
			auto start = buffer.find("!!");
			if (start == std::string::npos) {
				buffer.clear();
				return;
			}
			if (start > 0) {
				buffer.erase(0, start);
			}
			auto end = buffer.find("##", 2U);
			if (end == std::string::npos) {
				return;
			}
			const size_t end_pos = end + 2U;
			std::string raw = buffer.substr(0, end_pos);
			buffer.erase(0, end_pos);

			MessageParts parts;
			if (!parse_message(raw, parts)) {
				std::cerr << "Router refused malformed message: " << raw << '\n';
				continue;
			}

			if (direction == Direction::MacToRouter) {
				handle_mac(parts);
			} else {
				handle_device(parts);
			}
		}
	}

	void handle_mac(const MessageParts& parts) {
		if (parts.type != "REQUEST") {
			std::cerr << "Router ignoring MAC message (expected REQUEST): "
				  << pretty_message(parts) << '\n';
			return;
		}
		if (parts.addresses.empty()) {
			std::cerr << "Router missing destination in MAC message: "
				  << pretty_message(parts) << '\n';
			return;
		}

		const std::string& destination = parts.addresses.back();

		MessageParts forward = parts;
		forward.addresses.insert(forward.addresses.begin(), "MASTER");

		const std::string outbound = build_message(forward);
		device_serial.write(outbound);

		origin_by_command[parts.command] = destination;
		if (parts.command == "SEND_STAR") {
			origin_by_command["STAR_ARRIVED"] = destination;
		}
	}

	void handle_device(const MessageParts& parts) {
		auto origin_it = origin_by_command.find(parts.command);
		if (origin_it == origin_by_command.end()) {
			std::cerr << "Router lacks origin mapping for " << pretty_message(parts) << '\n';
			return;
		}

		MessageParts forward;
		forward.addresses = {origin_it->second, "MASTER"};
		forward.type = parts.type;
		forward.command = parts.command;
		forward.parameters = parts.parameters;

		const std::string outbound = build_message(forward);
		mac_serial.write(outbound);
	}
};

class VirtualEsp {
public:
	explicit VirtualEsp(MockSerial& device_port) : serial(device_port) {}

	void poll() {
		while (serial.available() > 0) {
			buffer.push_back(static_cast<char>(serial.read()));
		}
		drain();
	}

private:
	MockSerial& serial;
	std::string buffer;

	void drain() {
		while (true) {
			auto start = buffer.find("!!");
			if (start == std::string::npos) {
				buffer.clear();
				return;
			}
			if (start > 0) {
				buffer.erase(0, start);
			}

			auto end = buffer.find("##", 2U);
			if (end == std::string::npos) {
				return;
			}
			const size_t end_pos = end + 2U;
			std::string raw = buffer.substr(0, end_pos);
			buffer.erase(0, end_pos);

			MessageParts parts;
			if (!parse_message(raw, parts)) {
				std::cerr << "ESP could not parse message: " << raw << '\n';
				continue;
			}

			handle(parts);
		}
	}

	void handle(const MessageParts& parts) {
		if (parts.type != "REQUEST") {
			std::cerr << "ESP expected REQUEST but got " << pretty_message(parts) << '\n';
			return;
		}
		if (parts.addresses.size() < 2U) {
			std::cerr << "ESP requires source and destination tags: "
				  << pretty_message(parts) << '\n';
			return;
		}

		const std::string& device_tag = parts.addresses.back();

		std::cout << "ESP received " << pretty_message(parts) << '\n';

		if (parts.command == "MAKE_STAR") {
			send_confirmation("MAKE_STAR");
		} else if (parts.command == "SEND_STAR") {
			send_confirmation("SEND_STAR");
			send_star_arrived(device_tag);
		} else if (parts.command == "CANCEL_STAR") {
			send_confirmation("CANCEL_STAR");
		} else if (parts.command == "ADD_STAR") {
			send_confirmation("ADD_STAR");
		} else {
			std::cerr << "ESP does not handle " << parts.command << '\n';
		}
	}

	void send_confirmation(const std::string& command) {
		MessageParts response;
		response.addresses = {"MASTER"};
		response.type = "CONFIRM";
		response.command = command;
		response.parameters.clear();

		const std::string raw = build_message(response);
		std::cout << "ESP sending " << raw << '\n';
		serial.write(raw);
	}

	void send_star_arrived(const std::string& arm_tag) {
		MessageParts status;
		status.addresses = {"MASTER"};
		status.type = "REQUEST";
		status.command = "STAR_ARRIVED";
		status.parameters = "arm=" + arm_tag + ",speed=3,color=red,brightness=80,size=10";

		const std::string raw = build_message(status);
		std::cout << "ESP sending " << raw << '\n';
		serial.write(raw);
	}
};

std::vector<std::string> collect_messages(MockSerial& port) {
	std::string buffer;
	while (port.available() > 0) {
		buffer.push_back(static_cast<char>(port.read()));
	}

	std::vector<std::string> messages;
	while (true) {
		auto start = buffer.find("!!");
		if (start == std::string::npos) {
			break;
		}
		if (start > 0) {
			buffer.erase(0, start);
		}
		auto end = buffer.find("##", 2U);
		if (end == std::string::npos) {
			break;
		}
		const size_t end_pos = end + 2U;
		messages.push_back(buffer.substr(0, end_pos));
		buffer.erase(0, end_pos);
	}

	return messages;
}

void pump(VirtualRouter& router, VirtualEsp& esp, int iterations = 4) {
	for (int i = 0; i < iterations; ++i) {
		router.poll();
		esp.poll();
	}
}

int main() {
	MockSerial mac_port;
	MockSerial router_mac_port;

	mac_port.connect(&router_mac_port);
	router_mac_port.connect(&mac_port);

	MockSerial router_device_port;
	MockSerial esp_port;

	router_device_port.connect(&esp_port);
	esp_port.connect(&router_device_port);

	VirtualRouter router(router_mac_port, router_device_port);
	VirtualEsp esp(esp_port);

	const auto send_from_mac = [&](const std::string& destination, const std::string& command,
		const std::string& parameters) {
		MessageParts request;
		request.addresses = {destination};
		request.type = "REQUEST";
		request.command = command;
		request.parameters = parameters;

		const std::string raw = build_message(request);
		std::cout << "MAC sending " << raw << '\n';
		mac_port.write(raw);

		pump(router, esp, 6);

		auto responses = collect_messages(mac_port);
		for (const auto& response : responses) {
			MessageParts parts;
			if (parse_message(response, parts)) {
				std::cout << "MAC received " << pretty_message(parts) << " -> " << response
					  << '\n';
			} else {
				std::cout << "MAC received " << response << '\n';
			}
		}
	};

	send_from_mac("ARM1", "MAKE_STAR", "speed=3,color=red,brightness=80,size=10");
	send_from_mac("ARM1", "SEND_STAR", "");
	send_from_mac("ARM1", "CANCEL_STAR", "");
	send_from_mac("CENTER", "ADD_STAR", "star=constellation,speed=3,color=red,brightness=80,size=10");

	return 0;
}
