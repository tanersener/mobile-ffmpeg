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
			m_result[i] = (i > 0) ? value ^ m_result[i - 1] : value;
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

std::vector<uint32_t> FingerprintDecompressor::Decompress(const std::string &data, int *algorithm)
{
	if (data.size() < 4) {
		DEBUG("FingerprintDecompressor::Decompress() -- Invalid fingerprint (shorter than 4 bytes)");
		return std::vector<uint32_t>();
	}

	if (algorithm) {
		*algorithm = data[0];
	}

	const size_t num_values =
		((unsigned char)(data[1]) << 16) |
		((unsigned char)(data[2]) <<  8) |
		((unsigned char)(data[3])      );

	size_t offset = 4;
	m_bits.resize(GetUnpackedInt3ArraySize(data.size() - offset));
	UnpackInt3Array(data.begin() + offset, data.end(), m_bits.begin());

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
		DEBUG("FingerprintDecompressor::Decompress() -- Invalid fingerprint (too short, not enough data for normal bits)");
		return std::vector<uint32_t>();
	}

	offset += GetPackedInt3ArraySize(m_bits.size());
	if (data.size() < offset + GetPackedInt5ArraySize(num_exceptional_bits)) {
		DEBUG("FingerprintDecompressor::Decompress() -- Invalid fingerprint (too short, not enough data for exceptional bits)");
		return std::vector<uint32_t>();
	}

	if (num_exceptional_bits) {
		m_exceptional_bits.resize(GetUnpackedInt5ArraySize(GetPackedInt5ArraySize(num_exceptional_bits)));
		UnpackInt5Array(data.begin() + offset, data.end(), m_exceptional_bits.begin());
		for (size_t i = 0, j = 0; i < m_bits.size(); i++) {
			if (m_bits[i] == kMaxNormalValue) {
				m_bits[i] += m_exceptional_bits[j++];
			}
		}
	}

	m_result.assign(num_values, -1);

	UnpackBits();
	return m_result;
}

}; // namespace chromaprint
