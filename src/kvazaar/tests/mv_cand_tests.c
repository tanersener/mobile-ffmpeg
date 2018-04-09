/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
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

#include "src/inter.c"

#include <string.h>

#include "greatest/greatest.h"

TEST test_get_spatial_merge_cand(void)
{
  lcu_t lcu;
  memset(&lcu, 0, sizeof(lcu));
  for (int i = 0; i < sizeof(lcu.cu) / sizeof(cu_info_t); i++) {
    lcu.cu[i].type = CU_INTER;
  }

  merge_candidates_t cand = { {0, 0}, {0, 0, 0}, 0, 0 };

  get_spatial_merge_candidates(64 + 32, 64, // x, y
                               32, 24,      // width, height
                               1920, 1080,  // picture size
                               &lcu,
                               &cand);

  ASSERT_EQ(cand.b[0], &lcu.cu[289]);
  ASSERT_EQ(cand.b[1], &lcu.cu[ 16]);
  ASSERT_EQ(cand.b[2], &lcu.cu[  8]);
  ASSERT_EQ(cand.a[0], &lcu.cu[127]);
  ASSERT_EQ(cand.a[1], &lcu.cu[110]);

  PASS();
}

TEST test_is_a0_cand_coded()
{
  // +--+--+
  // |##|  |
  // +--+--+
  // |  |  |
  // +--+--+
  ASSERT_EQ(is_a0_cand_coded(32, 64, 16, 16), true);
  // Same as above with a 2NxN block
  ASSERT_EQ(is_a0_cand_coded(32, 64, 32, 16), true);
  // Same as above with a 2NxnU block
  ASSERT_EQ(is_a0_cand_coded(32, 64, 32, 8), true);
  // Same as above with a 2NxnD block
  ASSERT_EQ(is_a0_cand_coded(32, 64, 32, 24), true);

  // +--+--+
  // |  |##|
  // +--+--+
  // |  |  |
  // +--+--+
  ASSERT_EQ(is_a0_cand_coded(16, 0, 16, 16), false);

  // +--+--+
  // |  |  |
  // +--+--+
  // |  |##|
  // +--+--+
  ASSERT_EQ(is_a0_cand_coded(48, 16, 16, 16), false);
  // Same as above with a Nx2N block
  ASSERT_EQ(is_a0_cand_coded(48, 0, 16, 32), false);
  // Same as above with a nLx2N block
  ASSERT_EQ(is_a0_cand_coded(40, 0, 24, 32), false);
  // Same as above with a nRx2N block
  ASSERT_EQ(is_a0_cand_coded(56, 0, 8, 32), false);

  // +-----+--+--+
  // |     |  |  |
  // |     +--+--+
  // |     |##|  |
  // +-----+--+--+
  // |     |     |
  // |     |     |
  // |     |     |
  // +-----+-----+
  ASSERT_EQ(is_a0_cand_coded(32, 16, 16, 16), false);

  // Same as above with a 2NxnU block
  ASSERT_EQ(is_a0_cand_coded(32, 8, 32, 24), false);
  // Same as above with a 2NxnD block
  ASSERT_EQ(is_a0_cand_coded(32, 24, 32, 8), false);

  // Same as above with a Nx2N block
  ASSERT_EQ(is_a0_cand_coded(32, 0, 16, 32), false);
  // Same as above with a nLx2N block
  ASSERT_EQ(is_a0_cand_coded(32, 0, 8, 32), false);
  // Same as above with a nRx2N block
  ASSERT_EQ(is_a0_cand_coded(32, 0, 24, 32), false);

  // +--+--+-----+
  // |  |  |     |
  // +--+--+     |
  // |##|  |     |
  // +--+--+-----+
  // |     |     |
  // |     |     |
  // |     |     |
  // +-----+-----+
  ASSERT_EQ(is_a0_cand_coded(32, 8, 8, 8), true);

  // Same as above with a 2NxnU block
  ASSERT_EQ(is_a0_cand_coded(32, 4, 16, 12), true);
  // Same as above with a 2NxnD block
  ASSERT_EQ(is_a0_cand_coded(32, 12, 16, 4), true);

  // Same as above with a Nx2N block
  ASSERT_EQ(is_a0_cand_coded(32, 0, 8, 16), true);
  // Same as above with a nLx2N block
  ASSERT_EQ(is_a0_cand_coded(32, 0, 4, 16), true);
  // Same as above with a nRx2N block
  ASSERT_EQ(is_a0_cand_coded(32, 0, 12, 16), true);

  PASS();
}

TEST test_is_b0_cand_coded()
{
  // +--+--+
  // |##|  |
  // +--+--+
  // |  |  |
  // +--+--+
  ASSERT_EQ(is_b0_cand_coded(32, 64, 16, 16), true);
  // Same as above with a Nx2N block
  ASSERT_EQ(is_b0_cand_coded(32, 64, 16, 32), true);
  // Same as above with a nLx2N block
  ASSERT_EQ(is_b0_cand_coded(32, 64, 24, 32), true);
  // Same as above with a nRx2N block
  ASSERT_EQ(is_b0_cand_coded(32, 64, 8, 32), true);

  // +--+--+
  // |  |  |
  // +--+--+
  // |##|  |
  // +--+--+
  ASSERT_EQ(is_b0_cand_coded(32, 16, 16, 16), true);

  // +--+--+
  // |  |  |
  // +--+--+
  // |  |##|
  // +--+--+
  ASSERT_EQ(is_b0_cand_coded(48, 16, 16, 16), false);
  // Same as above with a 2NxN block
  ASSERT_EQ(is_b0_cand_coded(32, 16, 32, 16), false);
  // Same as above with a 2NxnU block
  ASSERT_EQ(is_b0_cand_coded(32, 8, 32, 24), false);
  // Same as above with a 2NxnD block
  ASSERT_EQ(is_b0_cand_coded(32, 24, 32, 8), false);

  // +-----+-----+
  // |     |     |
  // |     |     |
  // |     |     |
  // +-----+--+--+
  // |     |  |##|
  // |     +--+--+
  // |     |  |  |
  // +-----+--+--+
  ASSERT_EQ(is_b0_cand_coded(48, 32, 16, 16), false);

  // Same as above with a 2NxnU block
  ASSERT_EQ(is_b0_cand_coded(32, 32, 32, 8), false);
  // Same as above with a 2NxnD block
  ASSERT_EQ(is_b0_cand_coded(32, 32, 32, 24), false);

  // Same as above with a nLx2N block
  ASSERT_EQ(is_b0_cand_coded(56, 32, 8, 32), false);
  // Same as above with a nRx2N block
  ASSERT_EQ(is_b0_cand_coded(40, 32, 24, 32), false);

  // +--+--+-----+
  // |  |##|     |
  // +--+--+     |
  // |  |  |     |
  // +--+--+-----+
  // |     |     |
  // |     |     |
  // |     |     |
  // +-----+-----+
  ASSERT_EQ(is_b0_cand_coded(16, 0, 16, 16), true);

  // Same as above with a 2NxnU block
  ASSERT_EQ(is_b0_cand_coded(0, 0, 32, 8), true);
  // Same as above with a 2NxnD block
  ASSERT_EQ(is_b0_cand_coded(0, 0, 32, 24), true);

  // Same as above with a nLx2N block
  ASSERT_EQ(is_b0_cand_coded(8, 0, 24, 32), true);
  // Same as above with a nRx2N block
  ASSERT_EQ(is_b0_cand_coded(24, 0, 8, 32), true);

  PASS();
}

SUITE(mv_cand_tests) {
  RUN_TEST(test_get_spatial_merge_cand);
  RUN_TEST(test_is_a0_cand_coded);
  RUN_TEST(test_is_b0_cand_coded);
}
