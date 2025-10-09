#pragma once

#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

class MockSerial {
public:
	MockSerial() = default;

	void connect(MockSerial* peer_port) { peer = peer_port; }

	void begin(unsigned long, uint32_t = 0, int = -1, int = -1) {}
	void setTX(uint8_t) {}
	void setRX(uint8_t) {}

	size_t write(uint8_t value) {
		if (!peer) {
			return 0U;
		}
		peer->rx_buffer.push_back(value);
		return 1U;
	}

	size_t write(const uint8_t* data, size_t size) {
		if (!peer || !data) {
			return 0U;
		}
		size_t written = 0U;
		for (size_t i = 0; i < size; ++i) {
			peer->rx_buffer.push_back(data[i]);
			++written;
		}
		return written;
	}

	size_t write(const std::string& text) {
		if (!peer) {
			return 0U;
		}
		size_t written = 0U;
		for (unsigned char ch : text) {
			peer->rx_buffer.push_back(static_cast<uint8_t>(ch));
			++written;
		}
		return written;
	}

	int available() const { return static_cast<int>(rx_buffer.size()); }

	int read() {
		if (rx_buffer.empty()) {
			return -1;
		}
		const uint8_t value = rx_buffer.front();
		rx_buffer.pop_front();
		return static_cast<int>(value);
	}

	void flush() {}

	void print(const char* text) { std::cout << text; }
	void print(char value) { std::cout << value; }
	void print(int value, int base = 10) { std::cout << format_value(value, base); }
	void print(unsigned long value, int base = 10) { std::cout << format_value(value, base); }

	void println(const char* text) { std::cout << text << '\n'; }
	void println(char value) { std::cout << value << '\n'; }
	void println(int value, int base = 10) { std::cout << format_value(value, base) << '\n'; }
	void println(unsigned long value, int base = 10) { std::cout << format_value(value, base) << '\n'; }

	explicit operator bool() const { return true; }

private:
	std::deque<uint8_t> rx_buffer{};
	MockSerial* peer = nullptr;

	template <typename T>
	std::string format_value(T value, int base) {
		if (base == 16) {
			std::ostringstream oss;
			oss << std::hex << std::uppercase << static_cast<int>(value);
			return oss.str();
		}
		return std::to_string(value);
	}
};
