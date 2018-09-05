// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FINGERPRINT_DECOMPRESSOR_H_
#define CHROMAPRINT_FINGERPRINT_DECOMPRESSOR_H_

#include <cstdint>
#include <vector>
#include <string>

namespace chromaprint {

class FingerprintDecompressor
{
public:
	FingerprintDecompressor();
	std::vector<uint32_t> Decompress(const std::string &fingerprint, int *algorithm = 0);

private:
	void UnpackBits();
	std::vector<uint32_t> m_result;
	std::vector<unsigned char> m_bits;
	std::vector<unsigned char> m_exceptional_bits;
};

inline std::vector<uint32_t> DecompressFingerprint(const std::string &data, int *algorithm = 0)
{
	FingerprintDecompressor decompressor;
	return decompressor.Decompress(data, algorithm);
}

}; // namespace chromaprint

#endif
