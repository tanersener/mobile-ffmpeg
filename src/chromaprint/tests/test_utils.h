#ifndef CHROMAPRINT_TESTS_UTILS_H_
#define CHROMAPRINT_TESTS_UTILS_H_

#include <stdint.h>
#include <vector>
#include <fstream>
#include <string>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define NELEMS(x) (sizeof(x)/sizeof(x[0]))

inline void CheckString(std::string actual, char *expected, int expected_size)
{
	ASSERT_EQ(expected_size, actual.size());
	for (int i = 0; i < expected_size; i++) {
		EXPECT_EQ(expected[i], actual[i]) << "Different at index " << i;
	}
}

inline void CheckFingerprints(std::vector<uint32_t> actual, uint32_t *expected, int expected_size)
{
	ASSERT_EQ(expected_size, actual.size());
	for (int i = 0; i < expected_size; i++) {
		EXPECT_EQ(expected[i], actual[i]) << "Different at index " << i;
	}
}

inline std::vector<short> LoadAudioFile(const std::string &file_name)
{
	std::string path = TESTS_DIR + file_name;
	std::ifstream file(path.c_str(), std::ifstream::in | std::ifstream::binary);
	uint8_t buf[4096];
	std::vector<int16_t> data;
	while (!file.eof()) {
		file.read((char *) buf, 4096);
		size_t nread = file.gcount();
		for (size_t i = 0; i < nread - 1; i += 2) {
			data.push_back((int16_t) (((uint16_t) buf[i+1] << 8) | ((uint16_t) buf[i])));
		}
	}
	file.close();
	return data;
}

#endif
