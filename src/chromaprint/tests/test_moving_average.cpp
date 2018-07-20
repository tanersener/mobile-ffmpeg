#include <gtest/gtest.h>
#include <algorithm>
#include "moving_average.h"

using namespace chromaprint;

TEST(MovingAverage, Empty) {
	MovingAverage<int> avg(2);
	
	EXPECT_EQ(0, avg.GetAverage());

	avg.AddValue(100);
	EXPECT_EQ(100, avg.GetAverage());

	avg.AddValue(50);
	EXPECT_EQ(75, avg.GetAverage());

	avg.AddValue(0);
	EXPECT_EQ(25, avg.GetAverage());

	avg.AddValue(1000);
	EXPECT_EQ(500, avg.GetAverage());
}

