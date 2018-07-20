// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

// This file was automatically generate using gen_bit_reader.py, do not edit.

#ifndef CHROMAPRINT_UTILS_UNPACK_INT5_ARRAY_H_
#define CHROMAPRINT_UTILS_UNPACK_INT5_ARRAY_H_

#include <algorithm>

namespace chromaprint {

inline size_t GetUnpackedInt5ArraySize(size_t size) {
	return size * 8 / 5;
}

template <typename InputIt, typename OutputIt>
inline OutputIt UnpackInt5Array(const InputIt first, const InputIt last, OutputIt dest) {
	auto size = std::distance(first, last);
	auto src = first;
	while (size >= 5) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		const unsigned char s2 = *src++;
		const unsigned char s3 = *src++;
		const unsigned char s4 = *src++;
		*dest++ = (s0 & 0x1f);
		*dest++ = ((s0 & 0xe0) >> 5) | ((s1 & 0x03) << 3);
		*dest++ = ((s1 & 0x7c) >> 2);
		*dest++ = ((s1 & 0x80) >> 7) | ((s2 & 0x0f) << 1);
		*dest++ = ((s2 & 0xf0) >> 4) | ((s3 & 0x01) << 4);
		*dest++ = ((s3 & 0x3e) >> 1);
		*dest++ = ((s3 & 0xc0) >> 6) | ((s4 & 0x07) << 2);
		*dest++ = ((s4 & 0xf8) >> 3);
		size -= 5;
	}
	if (size == 4) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		const unsigned char s2 = *src++;
		const unsigned char s3 = *src++;
		*dest++ = (s0 & 0x1f);
		*dest++ = ((s0 & 0xe0) >> 5) | ((s1 & 0x03) << 3);
		*dest++ = ((s1 & 0x7c) >> 2);
		*dest++ = ((s1 & 0x80) >> 7) | ((s2 & 0x0f) << 1);
		*dest++ = ((s2 & 0xf0) >> 4) | ((s3 & 0x01) << 4);
		*dest++ = ((s3 & 0x3e) >> 1);
	} else if (size == 3) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		const unsigned char s2 = *src++;
		*dest++ = (s0 & 0x1f);
		*dest++ = ((s0 & 0xe0) >> 5) | ((s1 & 0x03) << 3);
		*dest++ = ((s1 & 0x7c) >> 2);
		*dest++ = ((s1 & 0x80) >> 7) | ((s2 & 0x0f) << 1);
	} else if (size == 2) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		*dest++ = (s0 & 0x1f);
		*dest++ = ((s0 & 0xe0) >> 5) | ((s1 & 0x03) << 3);
		*dest++ = ((s1 & 0x7c) >> 2);
	} else if (size == 1) {
		const unsigned char s0 = *src++;
		*dest++ = (s0 & 0x1f);
	}
	return dest;
}

}; // namespace chromaprint

#endif
