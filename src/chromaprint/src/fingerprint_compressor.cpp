// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <algorithm>
#include "fingerprint_compressor.h"
#include "utils.h"
#include "utils/pack_int3_array.h"
#include "utils/pack_int5_array.h"

namespace chromaprint {

static const int kNormalBits = 3;
static const int kMaxNormalValue = (1 << kNormalBits) - 1;

FingerprintCompressor::FingerprintCompressor()
{
}

void FingerprintCompressor::ProcessSubfingerprint(uint32_t x)
{
	int bit = 1, last_bit = 0;
	while (x != 0) {
		if ((x & 1) != 0) {
			const auto value = bit - last_bit;
			if (value >= kMaxNormalValue) {
				m_normal_bits.push_back(kMaxNormalValue);
				m_exceptional_bits.push_back(value - kMaxNormalValue);
			} else {
				m_normal_bits.push_back(value);
			}
			last_bit = bit;
		}
		x >>= 1;
		bit++;
	}
	m_normal_bits.push_back(0);
}

void FingerprintCompressor::Compress(const std::vector<uint32_t> &data, int algorithm, std::string &output)
{
	const auto size = data.size();

	m_normal_bits.clear();
	m_exceptional_bits.clear();

	if (size > 0) {
		m_normal_bits.reserve(size);
		m_exceptional_bits.reserve(size / 10);
		ProcessSubfingerprint(data[0]);
		for (size_t i = 1; i < size; i++) {
			ProcessSubfingerprint(data[i] ^ data[i - 1]);
		}
	}

	output.resize(4 + GetPackedInt3ArraySize(m_normal_bits.size()) + GetPackedInt5ArraySize(m_exceptional_bits.size()));
	output[0] = algorithm & 255;
	output[1] = (size >> 16) & 255;
	output[2] = (size >>  8) & 255;
	output[3] = (size      ) & 255;

	auto ptr = output.begin() + 4;
	ptr = PackInt3Array(m_normal_bits.begin(), m_normal_bits.end(), ptr);
	ptr = PackInt5Array(m_exceptional_bits.begin(), m_exceptional_bits.end(), ptr);
}

}; // namespace chromaprint

