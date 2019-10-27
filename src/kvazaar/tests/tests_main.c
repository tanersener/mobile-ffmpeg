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

#include "greatest/greatest.h"

#include "test_strategies.h"

GREATEST_MAIN_DEFS();
#if KVZ_BIT_DEPTH == 8
extern SUITE(sad_tests);
extern SUITE(intra_sad_tests);
extern SUITE(satd_tests);
extern SUITE(speed_tests);
extern SUITE(dct_tests);
#endif //KVZ_BIT_DEPTH == 8

extern SUITE(coeff_sum_tests);
extern SUITE(mv_cand_tests);
extern SUITE(inter_recon_bipred_tests);

int main(int argc, char **argv)
{
  GREATEST_MAIN_BEGIN();

  init_test_strategies(1);
#if KVZ_BIT_DEPTH == 8
  RUN_SUITE(sad_tests);
  RUN_SUITE(intra_sad_tests);
  RUN_SUITE(satd_tests);
  RUN_SUITE(dct_tests);

  if (greatest_info.suite_filter &&
      greatest_name_match("speed", greatest_info.suite_filter))
  {
    RUN_SUITE(speed_tests);
  }
#else
  printf("10-bit tests are not yet supported\n");
#endif //KVZ_BIT_DEPTH == 8

  RUN_SUITE(coeff_sum_tests);

  RUN_SUITE(mv_cand_tests);

  // Doesn't work in git
  //RUN_SUITE(inter_recon_bipred_tests);

  GREATEST_MAIN_END();
}
