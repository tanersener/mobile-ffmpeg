#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include <fstream>
#include "test_utils.h"
#include "audio_processor.h"
#include "audio_buffer.h"
#include "utils.h"

using namespace chromaprint;

TEST(AudioProcessor, Accessors)
{
	std::vector<short> data = LoadAudioFile("data/test_mono_44100.raw");

	AudioBuffer buffer;
	AudioBuffer buffer2;
	AudioProcessor processor(44100, &buffer);

	EXPECT_EQ(44100, processor.target_sample_rate());
	EXPECT_EQ(&buffer, processor.consumer());

	processor.set_target_sample_rate(11025);
	EXPECT_EQ(11025, processor.target_sample_rate());

	processor.set_consumer(&buffer2);
	EXPECT_EQ(&buffer2, processor.consumer());
}

TEST(AudioProcessor, PassThrough)
{
	std::vector<short> data = LoadAudioFile("data/test_mono_44100.raw");

	AudioBuffer buffer;
	AudioProcessor processor(44100, &buffer);
	processor.Reset(44100, 1);
	processor.Consume(data.data(), data.size());
	processor.Flush();

	ASSERT_EQ(data.size(), buffer.data().size());
	for (size_t i = 0; i < data.size(); i++) {
		ASSERT_EQ(data[i], buffer.data()[i]) << "Signals differ at index " << i;
	}
}

TEST(AudioProcessor, StereoToMono)
{
	std::vector<short> data1 = LoadAudioFile("data/test_stereo_44100.raw");
	std::vector<short> data2 = LoadAudioFile("data/test_mono_44100.raw");

	AudioBuffer buffer;
	AudioProcessor processor(44100, &buffer);
	processor.Reset(44100, 2);
	processor.Consume(data1.data(), data1.size());
	processor.Flush();

	ASSERT_EQ(data2.size(), buffer.data().size());
	for (size_t i = 0; i < data2.size(); i++) {
		ASSERT_EQ(data2[i], buffer.data()[i]) << "Signals differ at index " << i;
	}
}

TEST(AudioProcessor, ResampleMono)
{
	std::vector<short> data1 = LoadAudioFile("data/test_mono_44100.raw");
	std::vector<short> data2 = LoadAudioFile("data/test_mono_11025.raw");

	AudioBuffer buffer;
	AudioProcessor processor(11025, &buffer);
	processor.Reset(44100, 1);
	processor.Consume(data1.data(), data1.size());
	processor.Flush();

	ASSERT_EQ(data2.size(), buffer.data().size());
	for (size_t i = 0; i < data2.size(); i++) {
		ASSERT_EQ(data2[i], buffer.data()[i]) << "Signals differ at index " << i;
	}
}

TEST(AudioProcessor, ResampleMonoNonInteger)
{
	std::vector<short> data1 = LoadAudioFile("data/test_mono_44100.raw");
	std::vector<short> data2 = LoadAudioFile("data/test_mono_8000.raw");

	AudioBuffer buffer;
	AudioProcessor processor(8000, &buffer);
	processor.Reset(44100, 1);
	processor.Consume(data1.data(), data1.size());
	processor.Flush();

	ASSERT_EQ(data2.size(), buffer.data().size());
	for (size_t i = 0; i < data2.size(); i++) {
		ASSERT_NEAR(data2[i], buffer.data()[i], 3) << "Signals differ at index " << i;
	}
}

TEST(AudioProcessor, StereoToMonoAndResample)
{
	std::vector<short> data1 = LoadAudioFile("data/test_stereo_44100.raw");
	std::vector<short> data2 = LoadAudioFile("data/test_mono_11025.raw");

	AudioBuffer buffer;
	AudioProcessor processor(11025, &buffer);
	processor.Reset(44100, 2);
	processor.Consume(data1.data(), data1.size());
	processor.Flush();

	ASSERT_EQ(data2.size(), buffer.data().size());
	for (size_t i = 0; i < data2.size(); i++) {
		ASSERT_EQ(data2[i], buffer.data()[i]) << "Signals differ at index " << i;
	}
}
