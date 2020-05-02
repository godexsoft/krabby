#pragma once

#include <fmt/format.h>
#include <chrono>
#include <crab/crab.hpp>
#include <ctime>
#include <deque>
#include <iomanip>
#include <sstream>

namespace schwifty::krabby {

static inline std::string generate_key(size_t sz = 16) {
	crab::Random rnd;
	return rnd.printable_string(sz);
}

static inline std::string escape_html(std::string_view html) {
	std::string to_escape{R"(<>%&#:;'")"};
	std::string out{};
	out.reserve(html.length());  // optimistic memory allocation

	std::for_each(std::begin(html), std::end(html), [&](const auto &in) {
		if (std::find(std::begin(to_escape), std::end(to_escape), in) != std::end(to_escape))
			out.append(fmt::format("&#{};", static_cast<int>(in)));
		else
			out.push_back(in);
	});

	return out;
}

static inline std::string xor_sha1_key(std::string key, uint8_t xor_value) {
	const size_t block_size = 64;  // we know the block size because this is sha1 specifically
	uint8_t result[block_size]{};
	const auto key_size = std::min(key.size(), block_size);
	auto i              = 0;

	for (; i < key_size; ++i)
		result[i] = key[i] ^ xor_value;
	for (; i < block_size; ++i)
		result[i] = xor_value;
	return std::string{std::begin(result), std::begin(result) + block_size};
}

static inline std::string hash_sha1(std::string &&data) {
	uint8_t result[crab::sha1::hash_size]{};
	crab::sha1 hash;

	hash.add(data.data(), data.size());
	hash.finalize(result);

	return std::string{std::begin(result), std::begin(result) + crab::sha1::hash_size};
}

static inline std::string string_to_hex(const std::string &input) {
	static const char hex_digits[] = "0123456789abcdef";

	std::string output;
	output.reserve(input.size() * 2);
	for (unsigned char c : input) {
		output.push_back(hex_digits[c >> 4]);
		output.push_back(hex_digits[c & 15]);
	}
	return output;
}

static inline std::string hmac_sha1(std::string &&msg, std::string &&key) {
	auto outer_key = xor_sha1_key(key, 0x5c);
	auto inner_key = xor_sha1_key(key, 0x36);

	auto inner = hash_sha1(inner_key + msg);
	return string_to_hex(hash_sha1(outer_key + inner));
}

}  // namespace schwifty::krabby
