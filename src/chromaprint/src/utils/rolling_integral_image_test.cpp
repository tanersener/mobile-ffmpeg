// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <functional>
#include <gtest/gtest.h>
#include "utils/rolling_integral_image.h"
#include "test_utils.h"

namespace chromaprint {

TEST(RollingIntegralImageTest, All) {
	RollingIntegralImage image(4);

	{
		std::vector<double> data { 1, 2, 3 };
		image.AddRow(data);
	}

	ASSERT_EQ(3, image.num_columns());
	ASSERT_EQ(1, image.num_rows());

	ASSERT_DOUBLE_EQ(1, image.Area(0, 0, 1, 1));
	ASSERT_DOUBLE_EQ(2, image.Area(0, 1, 1, 2));
	ASSERT_DOUBLE_EQ(3, image.Area(0, 2, 1, 3));
	ASSERT_DOUBLE_EQ(1 + 2 + 3, image.Area(0, 0, 1, 3));

	{
		std::vector<double> data { 4, 5, 6 };
		image.AddRow(data);
	}

	ASSERT_EQ(3, image.num_columns());
	ASSERT_EQ(2, image.num_rows());

	ASSERT_DOUBLE_EQ(4, image.Area(1, 0, 2, 1));
	ASSERT_DOUBLE_EQ(5, image.Area(1, 1, 2, 2));
	ASSERT_DOUBLE_EQ(6, image.Area(1, 2, 2, 3));
	ASSERT_DOUBLE_EQ(1 + 2 + 3 + 4 + 5 + 6, image.Area(0, 0, 2, 3));

	{
		std::vector<double> data { 7, 8, 9 };
		image.AddRow(data);
	}

	ASSERT_EQ(3, image.num_columns());
	ASSERT_EQ(3, image.num_rows());

	{
		std::vector<double> data { 10, 11, 12 };
		image.AddRow(data);
	}

	ASSERT_EQ(3, image.num_columns());
	ASSERT_EQ(4, image.num_rows());

	ASSERT_DOUBLE_EQ((1 + 2 + 3) + (4 + 5 + 6) + (7 + 8 + 9) + (10 + 11 + 12), image.Area(0, 0, 4, 3));

	{
		std::vector<double> data { 13, 14, 15 };
		image.AddRow(data);
	}

	ASSERT_EQ(3, image.num_columns());
	ASSERT_EQ(5, image.num_rows());

	ASSERT_DOUBLE_EQ(4, image.Area(1, 0, 2, 1));
	ASSERT_DOUBLE_EQ(5, image.Area(1, 1, 2, 2));
	ASSERT_DOUBLE_EQ(6, image.Area(1, 2, 2, 3));
	ASSERT_DOUBLE_EQ(13, image.Area(4, 0, 5, 1));
	ASSERT_DOUBLE_EQ(14, image.Area(4, 1, 5, 2));
	ASSERT_DOUBLE_EQ(15, image.Area(4, 2, 5, 3));
	ASSERT_DOUBLE_EQ((4 + 5 + 6) + (7 + 8 + 9) + (10 + 11 + 12) + (13 + 14 + 15), image.Area(1, 0, 5, 3));

	{
		std::vector<double> data { 16, 17, 18 };
		image.AddRow(data);
	}

	ASSERT_EQ(3, image.num_columns());
	ASSERT_EQ(6, image.num_rows());

	ASSERT_DOUBLE_EQ(7, image.Area(2, 0, 3, 1));
	ASSERT_DOUBLE_EQ(8, image.Area(2, 1, 3, 2));
	ASSERT_DOUBLE_EQ(9, image.Area(2, 2, 3, 3));
	ASSERT_DOUBLE_EQ(16, image.Area(5, 0, 6, 1));
	ASSERT_DOUBLE_EQ(17, image.Area(5, 1, 6, 2));
	ASSERT_DOUBLE_EQ(18, image.Area(5, 2, 6, 3));
	ASSERT_DOUBLE_EQ((7 + 8 + 9) + (10 + 11 + 12) + (13 + 14 + 15) + (16 + 17 + 18), image.Area(2, 0, 6, 3));
}

}; // namespace chromaprint
