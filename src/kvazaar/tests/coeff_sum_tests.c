/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2017 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * Kvazaar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "greatest/greatest.h"

#include "test_strategies.h"

#include <string.h>

static coeff_t coeff_test_data[64 * 64];
static uint32_t expected_test_result;

static void setup()
{
  // Fill test data.
  coeff_t value = INT16_MIN;
  for (int i = 0; i < 64 * 64; i++) {
    coeff_test_data[i] = value;
    value += 16;
  }

  // Calculate expected result using the formula for an arithmetic sum.
  expected_test_result =
    2048 * (16 - INT16_MIN) / 2 +
    2048 * 2047 * 16 / 2;
}

TEST test_coeff_abs_sum()
{
  uint32_t sum = kvz_coeff_abs_sum(coeff_test_data, 64 * 64);
  ASSERT_EQ(sum, expected_test_result);
  PASS();
}

SUITE(coeff_sum_tests)
{
  setup();

  for (volatile int i = 0; i < strategies.count; ++i) {
    if (strcmp(strategies.strategies[i].type, "coeff_abs_sum") != 0) {
      continue;
    }

    kvz_coeff_abs_sum = strategies.strategies[i].fptr;
    RUN_TEST(test_coeff_abs_sum);
  }
}
