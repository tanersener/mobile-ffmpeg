#include "audio/ffmpeg_audio_reader.h"
#include <gtest/gtest.h>

#include <config.h>

namespace chromaprint {

TEST(FFmpegAudioReaderTest, ReadRaw) {
	FFmpegAudioReader reader;

	ASSERT_TRUE(reader.SetInputFormat("s16le"));
	ASSERT_TRUE(reader.SetInputChannels(2));
	ASSERT_TRUE(reader.SetInputSampleRate(44100));

	ASSERT_TRUE(reader.Open(TESTS_DIR "/data/test_stereo_44100.raw"));
	ASSERT_TRUE(reader.IsOpen());

	ASSERT_EQ(2, reader.GetChannels());
	ASSERT_EQ(44100, reader.GetSampleRate());

	const int16_t *data;
	size_t size;
	while (reader.Read(&data, &size)) {

	}

/*	ASSERT_TRUE(reader.ReadAll([&](const int16_t *data, size_t size) {
		
	}));*/
}

}; // namespace chromaprint
