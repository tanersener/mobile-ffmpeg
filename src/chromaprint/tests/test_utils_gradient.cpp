// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <gtest/gtest.h>
#include "utils/gradient.h"

using namespace chromaprint;

TEST(Gradient, Empty)
{
	std::vector<float> input, output;
	Gradient(input.begin(), input.end(), std::back_inserter(output));
	ASSERT_EQ(0, output.size());
}

TEST(Gradient, OneElement)
{
	std::vector<float> input, output;
	input.push_back(1.0f);
	Gradient(input.begin(), input.end(), std::back_inserter(output));
	ASSERT_EQ(1, output.size());
	ASSERT_FLOAT_EQ(0.0f, output[0]);
}

TEST(Gradient, TwoElements)
{
	std::vector<float> input, output;
	input.push_back(1.0f);
	input.push_back(2.0f);
	Gradient(input.begin(), input.end(), std::back_inserter(output));
	ASSERT_EQ(2, output.size());
	ASSERT_FLOAT_EQ(1.0f, output[0]);
	ASSERT_FLOAT_EQ(1.0f, output[1]);
}

TEST(Gradient, ThreeElements)
{
	std::vector<float> input, output;
	input.push_back(1.0f);
	input.push_back(2.0f);
	input.push_back(4.0f);
	Gradient(input.begin(), input.end(), std::back_inserter(output));
	ASSERT_EQ(3, output.size());
	ASSERT_FLOAT_EQ(1.0f, output[0]);
	ASSERT_FLOAT_EQ(1.5f, output[1]);
	ASSERT_FLOAT_EQ(2.0f, output[2]);
}

TEST(Gradient, FourElements)
{
	std::vector<float> input, output;
	input.push_back(1.0f);
	input.push_back(2.0f);
	input.push_back(4.0f);
	input.push_back(10.0f);
	Gradient(input.begin(), input.end(), std::back_inserter(output));
	ASSERT_EQ(4, output.size());
	ASSERT_FLOAT_EQ(1.0f, output[0]);
	ASSERT_FLOAT_EQ(1.5f, output[1]);
	ASSERT_FLOAT_EQ(4.0f, output[2]);
	ASSERT_FLOAT_EQ(6.0f, output[3]);
}

TEST(Gradient, OverrideInput)
{
	std::vector<float> input;
	input.push_back(1.0f);
	input.push_back(2.0f);
	input.push_back(4.0f);
	input.push_back(10.0f);
	Gradient(input.begin(), input.end(), input.begin());
	ASSERT_EQ(4, input.size());
	ASSERT_FLOAT_EQ(1.0f, input[0]);
	ASSERT_FLOAT_EQ(1.5f, input[1]);
	ASSERT_FLOAT_EQ(4.0f, input[2]);
	ASSERT_FLOAT_EQ(6.0f, input[3]);
}
