// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <gtest/gtest.h>
#include "utils/gaussian_filter.h"

using namespace chromaprint;

TEST(ReflectIterator, Iterate) {
	std::vector<int> data { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	ReflectIterator it(data.size());
	for (int i = 0; i < 3; i++) {
		it.MoveBack();
	}
	ASSERT_EQ(3, data[it.pos]);
	ASSERT_EQ(0, it.SafeForwardDistance());
	it.MoveForward();
	ASSERT_EQ(2, data[it.pos]);
	ASSERT_EQ(0, it.SafeForwardDistance());
	it.MoveForward();
	ASSERT_EQ(1, data[it.pos]);
	ASSERT_EQ(0, it.SafeForwardDistance());
	it.MoveForward();
	ASSERT_EQ(1, data[it.pos]);
	ASSERT_EQ(8, it.SafeForwardDistance());
	it.MoveForward();
	ASSERT_EQ(2, data[it.pos]);
//	ASSERT_EQ(3, it.SafeForwardDistance());
}

TEST(BoxFilter, Width1)
{
	std::vector<float> input { 1.0, 2.0, 4.0 };
	std::vector<float> output;
	BoxFilter(input, output, 1);
	ASSERT_EQ(input.size(), output.size());
	ASSERT_FLOAT_EQ(1.0, output[0]);
	ASSERT_FLOAT_EQ(2.0, output[1]);
	ASSERT_FLOAT_EQ(4.0, output[2]);
}

TEST(BoxFilter, Width2)
{
	std::vector<float> input { 1.0, 2.0, 4.0 };
	std::vector<float> output;
	BoxFilter(input, output, 2);
	ASSERT_EQ(input.size(), output.size());
	ASSERT_FLOAT_EQ(1.0, output[0]);
	ASSERT_FLOAT_EQ(1.5, output[1]);
	ASSERT_FLOAT_EQ(3.0, output[2]);
}

TEST(BoxFilter, Width3)
{
	std::vector<float> input { 1.0, 2.0, 4.0 };
	std::vector<float> output;
	BoxFilter(input, output, 3);
	ASSERT_EQ(input.size(), output.size());
	ASSERT_FLOAT_EQ(1.333333333, output[0]);
	ASSERT_FLOAT_EQ(2.333333333, output[1]);
	ASSERT_FLOAT_EQ(3.333333333, output[2]);
}

TEST(BoxFilter, Width4)
{
	std::vector<float> input { 1.0, 2.0, 4.0 };
	std::vector<float> output;
	BoxFilter(input, output, 4);
	ASSERT_EQ(input.size(), output.size());
	ASSERT_FLOAT_EQ(1.5, output[0]);
	ASSERT_FLOAT_EQ(2.0, output[1]);
	ASSERT_FLOAT_EQ(2.75, output[2]);
}

TEST(BoxFilter, Width5)
{
	std::vector<float> input { 1.0, 2.0, 4.0 };
	std::vector<float> output;
	BoxFilter(input, output, 5);
	ASSERT_EQ(input.size(), output.size());
	ASSERT_FLOAT_EQ(2.0, output[0]);
	ASSERT_FLOAT_EQ(2.4, output[1]);
	ASSERT_FLOAT_EQ(2.6, output[2]);
}

TEST(GaussianFilter, Test1)
{
	std::vector<float> input { 1.0, 2.0, 4.0 };
	std::vector<float> output;
	GaussianFilter(input, output, 1.6, 3);
	ASSERT_EQ(input.size(), output.size());
	ASSERT_FLOAT_EQ(1.88888889, output[0]);
	ASSERT_FLOAT_EQ(2.33333333, output[1]);
	ASSERT_FLOAT_EQ(2.77777778, output[2]);
}

TEST(GaussianFilter, Test2)
{
	std::vector<float> input { 1.0, 2.0, 4.0 };
	std::vector<float> output;
	GaussianFilter(input, output, 3.6, 4);
	ASSERT_EQ(input.size(), output.size());
	ASSERT_FLOAT_EQ(2.3322449, output[0]);
	ASSERT_FLOAT_EQ(2.33306122, output[1]);
	ASSERT_FLOAT_EQ(2.33469388, output[2]);
}
