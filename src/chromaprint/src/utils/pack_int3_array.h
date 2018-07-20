// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

// This file was automatically generate using gen_bit_writer.py, do not edit.

#ifndef CHROMAPRINT_UTILS_PACK_INT3_ARRAY_H_
#define CHROMAPRINT_UTILS_PACK_INT3_ARRAY_H_

#include <algorithm>

namespace chromaprint {

inline size_t GetPackedInt3ArraySize(size_t size) {
	return (size * 3 + 7) / 8;
}

template <typename InputIt, typename OutputIt>
inline OutputIt PackInt3Array(const InputIt first, const InputIt last, OutputIt dest) {
	auto size = std::distance(first, last);
	auto src = first;
	while (size >= 8) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		const unsigned char s2 = *src++;
		const unsigned char s3 = *src++;
		const unsigned char s4 = *src++;
		const unsigned char s5 = *src++;
		const unsigned char s6 = *src++;
		const unsigned char s7 = *src++;
		*dest++ = (unsigned char) ((s0 & 0x07)) | ((s1 & 0x07) << 3) | ((s2 & 0x03) << 6);
		*dest++ = (unsigned char) ((s2 & 0x04) >> 2) | ((s3 & 0x07) << 1) | ((s4 & 0x07) << 4) | ((s5 & 0x01) << 7);
		*dest++ = (unsigned char) ((s5 & 0x06) >> 1) | ((s6 & 0x07) << 2) | ((s7 & 0x07) << 5);
		size -= 8;
	}
	if (size == 7) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		const unsigned char s2 = *src++;
		const unsigned char s3 = *src++;
		const unsigned char s4 = *src++;
		const unsigned char s5 = *src++;
		const unsigned char s6 = *src++;
		*dest++ = (unsigned char) ((s0 & 0x07)) | ((s1 & 0x07) << 3) | ((s2 & 0x03) << 6);
		*dest++ = (unsigned char) ((s2 & 0x04) >> 2) | ((s3 & 0x07) << 1) | ((s4 & 0x07) << 4) | ((s5 & 0x01) << 7);
		*dest++ = (unsigned char) ((s5 & 0x06) >> 1) | ((s6 & 0x07) << 2);
	} else if (size == 6) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		const unsigned char s2 = *src++;
		const unsigned char s3 = *src++;
		const unsigned char s4 = *src++;
		const unsigned char s5 = *src++;
		*dest++ = (unsigned char) ((s0 & 0x07)) | ((s1 & 0x07) << 3) | ((s2 & 0x03) << 6);
		*dest++ = (unsigned char) ((s2 & 0x04) >> 2) | ((s3 & 0x07) << 1) | ((s4 & 0x07) << 4) | ((s5 & 0x01) << 7);
		*dest++ = (unsigned char) ((s5 & 0x06) >> 1);
	} else if (size == 5) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		const unsigned char s2 = *src++;
		const unsigned char s3 = *src++;
		const unsigned char s4 = *src++;
		*dest++ = (unsigned char) ((s0 & 0x07)) | ((s1 & 0x07) << 3) | ((s2 & 0x03) << 6);
		*dest++ = (unsigned char) ((s2 & 0x04) >> 2) | ((s3 & 0x07) << 1) | ((s4 & 0x07) << 4);
	} else if (size == 4) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		const unsigned char s2 = *src++;
		const unsigned char s3 = *src++;
		*dest++ = (unsigned char) ((s0 & 0x07)) | ((s1 & 0x07) << 3) | ((s2 & 0x03) << 6);
		*dest++ = (unsigned char) ((s2 & 0x04) >> 2) | ((s3 & 0x07) << 1);
	} else if (size == 3) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		const unsigned char s2 = *src++;
		*dest++ = (unsigned char) ((s0 & 0x07)) | ((s1 & 0x07) << 3) | ((s2 & 0x03) << 6);
		*dest++ = (unsigned char) ((s2 & 0x04) >> 2);
	} else if (size == 2) {
		const unsigned char s0 = *src++;
		const unsigned char s1 = *src++;
		*dest++ = (unsigned char) ((s0 & 0x07)) | ((s1 & 0x07) << 3);
	} else if (size == 1) {
		const unsigned char s0 = *src++;
		*dest++ = (unsigned char) ((s0 & 0x07));
	}
	 return dest;
}

}; // namespace chromaprint

#endif
