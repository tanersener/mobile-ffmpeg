// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_BASE64_H_
#define CHROMAPRINT_BASE64_H_

#include <string>

namespace chromaprint {

static const char kBase64Chars[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static const char kBase64CharsReversed[256] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 62, 0, 0,
	52,	53, 54, 55, 56, 57, 58, 59,
	60, 61, 0, 0, 0, 0, 0, 0,

	0, 0, 1, 2, 3, 4, 5, 6,
	7, 8, 9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19,	20, 21, 22,
	23, 24, 25, 0, 0, 0, 0, 63,
	0, 26, 27, 28, 29, 30, 31, 32,
	33,	34, 35, 36, 37,	38, 39, 40,
	41,	42, 43, 44, 45, 46, 47, 48,
	49,	50, 51, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};

inline size_t GetBase64EncodedSize(size_t size)
{
	return (size * 4 + 2) / 3;
}

inline size_t GetBase64DecodedSize(size_t size)
{
	return size * 3 / 4;
}

template <typename InputIt, typename OutputIt>
inline OutputIt Base64Encode(InputIt first, InputIt last, OutputIt dest, bool terminate = false)
{
	auto src = first;
	auto size = std::distance(first, last);
	while (size >= 3) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		const unsigned char s2 = *src++;
		*dest++ = kBase64Chars[(s0 >> 2) & 63];
		*dest++ = kBase64Chars[((s0 << 4) | (s1 >> 4)) & 63];
		*dest++ = kBase64Chars[((s1 << 2) | (s2 >> 6)) & 63];
		*dest++ = kBase64Chars[s2 & 63];
		size -= 3;
	}
	if (size == 2) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		*dest++ = kBase64Chars[(s0 >> 2) & 63];
		*dest++ = kBase64Chars[((s0 << 4) | (s1 >> 4)) & 63];
		*dest++ = kBase64Chars[(s1 << 2) & 63];
	} else if (size == 1) {
		const unsigned char s0 = *src++;
		*dest++ = kBase64Chars[(s0 >> 2) & 63];
		*dest++ = kBase64Chars[(s0 << 4) & 63];
	}
	if (terminate) {
		*dest++ = '\0';
	}
	return dest;
}

template <typename InputIt, typename OutputIt>
inline OutputIt Base64Decode(InputIt first, InputIt last, OutputIt dest)
{
	auto src = first;
	auto size = std::distance(first, last);
	while (size >= 4) {
		const unsigned char b0 = kBase64CharsReversed[*src++ & 255];
		const unsigned char b1 = kBase64CharsReversed[*src++ & 255];
		const unsigned char b2 = kBase64CharsReversed[*src++ & 255];
		const unsigned char b3 = kBase64CharsReversed[*src++ & 255];
		*dest++ = (b0 << 2) | (b1 >> 4);
		*dest++ = ((b1 << 4) & 255) | (b2 >> 2);
		*dest++ = ((b2 << 6) & 255) | b3;
		size -= 4;
	}
	if (size == 3) {
		const unsigned char b0 = kBase64CharsReversed[*src++ & 255];
		const unsigned char b1 = kBase64CharsReversed[*src++ & 255];
		const unsigned char b2 = kBase64CharsReversed[*src++ & 255];
		*dest++ = (b0 << 2) | (b1 >> 4);
		*dest++ = ((b1 << 4) & 255) | (b2 >> 2);
	} else if (size == 2) {
		const unsigned char b0 = kBase64CharsReversed[*src++ & 255];
		const unsigned char b1 = kBase64CharsReversed[*src++ & 255];
		*dest++ = (b0 << 2) | (b1 >> 4);
	}
	return dest;
}

void Base64Encode(const std::string &src, std::string &dest);
std::string Base64Encode(const std::string &src);

void Base64Decode(const std::string &encoded, std::string &dest);
std::string Base64Decode(const std::string &encoded);

}; // namespace chromaprint

#endif

