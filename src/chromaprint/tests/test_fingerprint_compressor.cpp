#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include <fstream>
#include "image.h"
#include "classifier.h"
#include "fingerprint_compressor.h"
#include "utils.h"
#include "test_utils.h"

using namespace chromaprint;

TEST(FingerprintCompressor, OneItemOneBit)
{
	FingerprintCompressor compressor;

	uint32_t fingerprint[] = { 1 };
	std::string value = compressor.Compress(std::vector<uint32_t>(fingerprint, fingerprint + 1));

	char expected[] = { 0, 0, 0, 1, 1 };
	CheckString(value, expected, sizeof(expected)/sizeof(expected[0]));
}

TEST(FingerprintCompressor, OneItemThreeBits)
{
	FingerprintCompressor compressor;

	uint32_t fingerprint[] = { 7 };
	std::string value = compressor.Compress(std::vector<uint32_t>(fingerprint, fingerprint + 1));

	char expected[] = { 0, 0, 0, 1, 73, 0 };
	CheckString(value, expected, sizeof(expected)/sizeof(expected[0]));
}

TEST(FingerprintCompressor, OneItemOneBitExcept)
{
	FingerprintCompressor compressor;

	uint32_t fingerprint[] = { 1<<6 };
	std::string value = compressor.Compress(std::vector<uint32_t>(fingerprint, fingerprint + 1));

	char expected[] = { 0, 0, 0, 1, 7, 0 };
	CheckString(value, expected, sizeof(expected)/sizeof(expected[0]));
}

TEST(FingerprintCompressor, OneItemOneBitExcept2)
{
	FingerprintCompressor compressor;

	uint32_t fingerprint[] = { 1<<8 };
	std::string value = compressor.Compress(std::vector<uint32_t>(fingerprint, fingerprint + 1));

	char expected[] = { 0, 0, 0, 1, 7, 2 };
	CheckString(value, expected, sizeof(expected)/sizeof(expected[0]));
}

TEST(FingerprintCompressor, TwoItems)
{
	FingerprintCompressor compressor;

	uint32_t fingerprint[] = { 1, 0 };
	std::string value = compressor.Compress(std::vector<uint32_t>(fingerprint, fingerprint + 2));

	char expected[] = { 0, 0, 0, 2, 65, 0 };
	CheckString(value, expected, sizeof(expected)/sizeof(expected[0]));
}

TEST(FingerprintCompressor, TwoItemsNoChange)
{
	FingerprintCompressor compressor;

	uint32_t fingerprint[] = { 1, 1 };
	std::string value = compressor.Compress(std::vector<uint32_t>(fingerprint, fingerprint + 2));

	char expected[] = { 0, 0, 0, 2, 1, 0 };
	CheckString(value, expected, sizeof(expected)/sizeof(expected[0]));
}
