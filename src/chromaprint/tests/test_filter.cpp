// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <gtest/gtest.h>
#include "filter.h"
#include "utils/rolling_integral_image.h"
#include "test_utils.h"

namespace chromaprint {

TEST(Filter, Filter0) {
	double data[] = {
		0.0, 1.0,
		2.0, 3.0,
	};
	RollingIntegralImage integral_image(2, data, data + NELEMS(data));

	Filter flt1(0, 0, 1, 1);	
	ASSERT_FLOAT_EQ(0.0, flt1.Apply(integral_image, 0));
	ASSERT_FLOAT_EQ(1.0986123, flt1.Apply(integral_image, 1));
}

}; // namespace chromaprint
