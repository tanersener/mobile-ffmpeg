// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <cmath>
#include <functional>
#include <gtest/gtest.h>
#include "fft.h"
#include "test_utils.h"

namespace chromaprint {

namespace {

struct Collector : public FFTFrameConsumer {
	virtual void Consume(const FFTFrame &frame) override {
		frames.push_back(frame);
	}
	std::vector<FFTFrame> frames;
};

};

TEST(FFTTest, Sine) {
	const size_t nframes = 3;
	const size_t frame_size = 32;
	const size_t overlap = 8;

	const int sample_rate = 1000;
	const double freq = 7 * (sample_rate / 2) / (frame_size / 2);

	std::vector<int16_t> input(frame_size + (nframes - 1) * (frame_size - overlap));
	for (size_t i = 0; i < input.size(); i++) {
		input[i] = INT16_MAX * sin(i * freq * 2.0 * M_PI / sample_rate);
	}

	Collector collector;
	FFT fft(frame_size, overlap, &collector);

	ASSERT_EQ(frame_size, fft.frame_size());
	ASSERT_EQ(overlap, fft.overlap());

	const size_t chunk_size = 100;
	for (size_t i = 0; i < input.size(); i += chunk_size) {
		const auto size = std::min(input.size() - i, chunk_size);
		fft.Consume(input.data() + i, size);
	}

	ASSERT_EQ(nframes, collector.frames.size());

	const double expected_spectrum[] = {
		2.87005e-05,
		0.00011901,
		0.00029869,
		0.000667172,
		0.00166813,
		0.00605612,
		0.228737,
		0.494486,
		0.210444,
		0.00385322,
		0.00194379,
		0.00124616,
		0.000903851,
		0.000715237,
		0.000605707,
		0.000551375,
		0.000534304,
	};

	for (const auto &frame : collector.frames) {
		std::stringstream ss;
		for (size_t i = 0; i < frame.size(); i++) {
			const auto magnitude = sqrt(frame[i]) / frame.size();
			EXPECT_NEAR(expected_spectrum[i], magnitude, 0.001) << "magnitude different at offset " << i;
			ss << "s[" << i << "]=" << magnitude << " ";
		}
//		DEBUG("spectrum " << ss.str());
	}
}

TEST(FFTTest, DC) {
	const size_t nframes = 3;
	const size_t frame_size = 32;
	const size_t overlap = 8;

	std::vector<int16_t> input(frame_size + (nframes - 1) * (frame_size - overlap));
	for (size_t i = 0; i < input.size(); i++) {
		input[i] = INT16_MAX * 0.5;
	}

	Collector collector;
	FFT fft(frame_size, overlap, &collector);

	ASSERT_EQ(frame_size, fft.frame_size());
	ASSERT_EQ(overlap, fft.overlap());

	const size_t chunk_size = 100;
	for (size_t i = 0; i < input.size(); i += chunk_size) {
		const auto size = std::min(input.size() - i, chunk_size);
		fft.Consume(input.data() + i, size);
	}

	ASSERT_EQ(nframes, collector.frames.size());

	const double expected_spectrum[] = {
		0.494691,
		0.219547,
		0.00488079,
		0.00178991,
		0.000939219,
		0.000576082,
		0.000385808,
		0.000272904,
		0.000199905,
		0.000149572,
		0.000112947,
		8.5041e-05,
		6.28312e-05,
		4.4391e-05,
		2.83757e-05,
		1.38507e-05,
		0,
	};


	for (const auto &frame : collector.frames) {
		std::stringstream ss;
		for (size_t i = 0; i < frame.size(); i++) {
			const auto magnitude = sqrt(frame[i]) / frame.size();
			EXPECT_NEAR(expected_spectrum[i], magnitude, 0.001) << "magnitude different at offset " << i;
			ss << "s[" << i << "]=" << magnitude << " ";
		}
//		DEBUG("spectrum " << ss.str());
	}
}

}; // namespace chromaprint
