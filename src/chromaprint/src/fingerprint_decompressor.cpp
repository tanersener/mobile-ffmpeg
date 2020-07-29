// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include "fingerprint_decompressor.h"
#include "debug.h"
#include "utils/pack_int3_array.h"
#include "utils/pack_int5_array.h"
#include "utils/unpack_int3_array.h"
#include "utils/unpack_int5_array.h"
#include "utils.h"

namespace chromaprint {

static const int kMaxNormalValue = 7;
static const int kNormalBits = 3;
static const int kExceptionBits = 5;

FingerprintDecompressor::FingerprintDecompressor()
{
}

void FingerprintDecompressor::UnpackBits()
{
	int i = 0, last_bit = 0, value = 0;
	for (size_t j = 0; j < m_bits.size(); j++) {
		int bit = m_bits[j];
		if (bit == 0) {
			m_output[i] = (i > 0) ? value ^ m_output[i - 1] : value;
			value = 0;
			last_bit = 0;
			i++;
			continue;
		}
		bit += last_bit;
		last_bit = bit;
		value |= 1 << (bit - 1);
	}
}

bool FingerprintDecompressor::Decompress(const std::string &input)
{
	if (input.size() < 4) {
		DEBUG("FingerprintDecompressor::Decompress() -- Invalid fingerprint (shorter than 4 bytes)");
		return false;
	}

	m_algorithm = input[0];

	const size_t num_values =
		((size_t)((unsigned char)(input[1])) << 16) |
		((size_t)((unsigned char)(input[2])) <<  8) |
		((size_t)((unsigned char)(input[3]))      );

	size_t offset = 4;
	m_bits.resize(GetUnpackedInt3ArraySize(input.size() - offset));
	UnpackInt3Array(input.begin() + offset, input.end(), m_bits.begin());

	size_t found_values = 0, num_exceptional_bits = 0;
	for (size_t i = 0; i < m_bits.size(); i++) {
		if (m_bits[i] == 0) {
			found_values += 1;
			if (found_values == num_values) {
				m_bits.resize(i + 1);
				break;
			}
		} else if (m_bits[i] == kMaxNormalValue) {
			num_exceptional_bits += 1;
		}
	}

	if (found_values != num_values) {
		DEBUG("FingerprintDecompressor::Decompress() -- Invalid fingerprint (too short, not enough input for normal bits)");
		return false;
	}

	offset += GetPackedInt3ArraySize(m_bits.size());
	if (input.size() < offset + GetPackedInt5ArraySize(num_exceptional_bits)) {
		DEBUG("FingerprintDecompressor::Decompress() -- Invalid fingerprint (too short, not enough input for exceptional bits)");
		return false;
	}

	if (num_exceptional_bits) {
		m_exceptional_bits.resize(GetUnpackedInt5ArraySize(GetPackedInt5ArraySize(num_exceptional_bits)));
		UnpackInt5Array(input.begin() + offset, input.end(), m_exceptional_bits.begin());
		for (size_t i = 0, j = 0; i < m_bits.size(); i++) {
			if (m_bits[i] == kMaxNormalValue) {
				m_bits[i] += m_exceptional_bits[j++];
			}
		}
	}

	m_output.assign(num_values, -1);

	UnpackBits();
	return true;
}

}; // namespace chromaprint
