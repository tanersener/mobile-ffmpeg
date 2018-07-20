// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <functional>
#include <gtest/gtest.h>
#include "audio/audio_slicer.h"
#include "test_utils.h"

namespace chromaprint {

namespace {

struct Collector {
	template <typename InputIt1, typename InputIt2>
	void operator()(InputIt1 b1, InputIt1 e1, InputIt2 b2, InputIt2 e2) {
		std::vector<int16_t> slice;
		slice.insert(slice.end(), b1, e1);
		slice.insert(slice.end(), b2, e2);
		output.push_back(slice);
	}
	std::vector<std::vector<int16_t>> output;
};

};

TEST(AudioSlicerTest, Process) {
	AudioSlicer<int16_t> slicer(4, 2);
	Collector collector;

	EXPECT_EQ(4, slicer.size());
	EXPECT_EQ(2, slicer.increment());

	const int16_t input[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	slicer.Process(input + 0, input + 1, std::ref(collector));
	slicer.Process(input + 1, input + 3, std::ref(collector));
	slicer.Process(input + 3, input + 6, std::ref(collector));
	slicer.Process(input + 6, input + 9, std::ref(collector));
	slicer.Process(input + 9, input + NELEMS(input), std::ref(collector));

	ASSERT_EQ(4, collector.output.size());
	for (const auto &slice : collector.output) {
		ASSERT_EQ(4, slice.size());
	}

	EXPECT_EQ(0, collector.output[0][0]);
	EXPECT_EQ(1, collector.output[0][1]);
	EXPECT_EQ(2, collector.output[0][2]);
	EXPECT_EQ(3, collector.output[0][3]);

	EXPECT_EQ(2, collector.output[1][0]);
	EXPECT_EQ(3, collector.output[1][1]);
	EXPECT_EQ(4, collector.output[1][2]);
	EXPECT_EQ(5, collector.output[1][3]);

	EXPECT_EQ(4, collector.output[2][0]);
	EXPECT_EQ(5, collector.output[2][1]);
	EXPECT_EQ(6, collector.output[2][2]);
	EXPECT_EQ(7, collector.output[2][3]);

	EXPECT_EQ(6, collector.output[3][0]);
	EXPECT_EQ(7, collector.output[3][1]);
	EXPECT_EQ(8, collector.output[3][2]);
	EXPECT_EQ(9, collector.output[3][3]);
}

}; // namespace chromaprint
