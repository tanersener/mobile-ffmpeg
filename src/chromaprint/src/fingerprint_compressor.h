// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FINGERPRINT_COMPRESSOR_H_
#define CHROMAPRINT_FINGERPRINT_COMPRESSOR_H_

#include <cstdint>
#include <vector>
#include <string>

namespace chromaprint {

class FingerprintCompressor
{
public:
	FingerprintCompressor();

	std::string Compress(const std::vector<uint32_t> &fingerprint, int algorithm = 0) {
		std::string tmp;
		Compress(fingerprint, algorithm, tmp);
		return tmp;
	}

	void Compress(const std::vector<uint32_t> &fingerprint, int algorithm, std::string &output);

private:
	void ProcessSubfingerprint(uint32_t);
	std::vector<unsigned char> m_normal_bits;
	std::vector<unsigned char> m_exceptional_bits;
};

inline std::string CompressFingerprint(const std::vector<uint32_t> &data, int algorithm = 0)
{
	FingerprintCompressor compressor;
	return compressor.Compress(data, algorithm);
}

}; // namespace chromaprint

#endif

