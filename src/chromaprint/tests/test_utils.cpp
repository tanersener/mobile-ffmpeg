#include <gtest/gtest.h>
#include <algorithm>
#include <limits>
#include "utils.h"

using namespace chromaprint;

TEST(Utils, PrepareHammingWindow) {
	double window_ex[10] = { 0.08, 0.187619556165, 0.460121838273, 0.77, 0.972258605562, 0.972258605562, 0.77, 0.460121838273, 0.187619556165, 0.08};
	double window[10];
	PrepareHammingWindow(window, window + 10);
	for (int i = 0; i < 10; i++) {
		EXPECT_FLOAT_EQ(window_ex[i], window[i]);
	}
}

TEST(Utils, ApplyWindow) {
	double window_ex[10] = { 0.08, 0.187619556165, 0.460121838273, 0.77, 0.972258605562, 0.972258605562, 0.77, 0.460121838273, 0.187619556165, 0.08};
	double window[10];
	int16_t input[10];
	double output[10];
	PrepareHammingWindow(window, window + 10, 1.0 / INT16_MAX);
	std::fill(input, input + 10, INT16_MAX);
	auto window_ptr = window + 0;
	auto output_ptr = output + 0;
	ApplyWindow(input, input + 10, window_ptr, output_ptr);
	ASSERT_EQ(window + 10, window_ptr);
	ASSERT_EQ(output + 10, output_ptr);
	for (int i = 0; i < 10; i++) {
		EXPECT_FLOAT_EQ(window_ex[i], output[i]);
	}
}

TEST(Utils, Sum) {
	double data[] = { 0.1, 0.2, 0.4, 1.0 };
	EXPECT_FLOAT_EQ(1.7, Sum(data, data + 4));
}

TEST(Utils, EuclideanNorm) {
	double data[] = { 0.1, 0.2, 0.4, 1.0 };
	EXPECT_FLOAT_EQ(1.1, EuclideanNorm(data, data + 4));
}

TEST(Utils, NormalizeVector) {
	double data[] = { 0.1, 0.2, 0.4, 1.0 };
	double normalized_data[] = { 0.090909, 0.181818, 0.363636, 0.909091 };
	NormalizeVector(data, data + 4, EuclideanNorm<double *>, 0.01);
	for (int i = 0; i < 4; i++) {
		EXPECT_NEAR(normalized_data[i], data[i], 1e-5) << "Wrong data at index " << i;
	}
}

TEST(Utils, NormalizeVectorNearZero) {
	double data[] = { 0.0, 0.001, 0.002, 0.003 };
	NormalizeVector(data, data + 4, EuclideanNorm<double *>, 0.01);
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(0.0, data[i]) << "Wrong data at index " << i;
	}
}

TEST(Utils, NormalizeVectorZero) {
	double data[] = { 0.0, 0.0, 0.0, 0.0 };
	NormalizeVector(data, data + 4, EuclideanNorm<double *>, 0.01);
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(0.0, data[i]) << "Wrong data at index " << i;
	}
}

TEST(Utils, IsNaN) {
    EXPECT_FALSE(IsNaN(0.0));
    EXPECT_TRUE(IsNaN(sqrt(-1.0)));
}

TEST(Utils, CountSetBits32) {
    EXPECT_EQ(0, CountSetBits(0x00U));
    EXPECT_EQ(8, CountSetBits(0xFFU));
    EXPECT_EQ(16, CountSetBits(0xFFFFU));
    EXPECT_EQ(24, CountSetBits(0xFFFFFFU));
    EXPECT_EQ(32, CountSetBits(0xFFFFFFFFU));
    EXPECT_EQ(4, CountSetBits(0x01010101U));
}

TEST(Utils, CountSetBits64) {
    EXPECT_EQ(0, CountSetBits(0x00U));
    EXPECT_EQ(8, CountSetBits(0xFFU));
    EXPECT_EQ(16, CountSetBits(0xFFFFU));
    EXPECT_EQ(24, CountSetBits(0xFFFFFFU));
    EXPECT_EQ(32, CountSetBits(0xFFFFFFFFU));
    EXPECT_EQ(40, CountSetBits(0xFFFFFFFFFFU));
    EXPECT_EQ(48, CountSetBits(0xFFFFFFFFFFFFU));
    EXPECT_EQ(56, CountSetBits(0xFFFFFFFFFFFFFFU));
    EXPECT_EQ(64, CountSetBits(0xFFFFFFFFFFFFFFFFU));
    EXPECT_EQ(8, CountSetBits(0x0101010101010101U));
}
