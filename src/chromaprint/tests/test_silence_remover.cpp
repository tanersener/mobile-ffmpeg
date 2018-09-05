#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include <fstream>
#include "test_utils.h"
#include "silence_remover.h"
#include "audio_buffer.h"
#include "utils.h"

using namespace chromaprint;

TEST(SilenceRemover, PassThrough)
{
	short samples[] = { 1000, 2000, 3000, 4000, 5000, 6000 };
	std::vector<short> data(samples, samples + NELEMS(samples));

	AudioBuffer buffer;
	SilenceRemover processor(&buffer);
	processor.Reset(44100, 1);
	processor.Consume(data.data(), data.size());
	processor.Flush();

	ASSERT_EQ(data.size(), buffer.data().size());
	for (size_t i = 0; i < data.size(); i++) {
		ASSERT_EQ(data[i], buffer.data()[i]) << "Signals differ at index " << i;
	}
}

TEST(SilenceRemover, RemoveLeadingSilence)
{
	short samples1[] = { 0, 60, 0, 1000, 2000, 0, 4000, 5000, 0 };
	std::vector<short> data1(samples1, samples1 + NELEMS(samples1));

	short samples2[] = { 1000, 2000, 0, 4000, 5000, 0 };
	std::vector<short> data2(samples2, samples2 + NELEMS(samples2));

	AudioBuffer buffer;
	SilenceRemover processor(&buffer, 100);
	processor.Reset(44100, 1);
	processor.Consume(data1.data(), data1.size());
	processor.Flush();

	ASSERT_EQ(data2.size(), buffer.data().size());
	for (size_t i = 0; i < data2.size(); i++) {
		ASSERT_EQ(data2[i], buffer.data()[i]) << "Signals differ at index " << i;
	}
}
