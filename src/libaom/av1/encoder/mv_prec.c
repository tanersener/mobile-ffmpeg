/*
 * Copyright (c) 2019, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */
#include "av1/encoder/mv_prec.h"

void av1_pick_and_set_high_precision_mv(AV1_COMP *cpi, int qindex) {
  int use_hp = qindex < HIGH_PRECISION_MV_QTHRESH;

  if (cpi->sf.hl_sf.high_precision_mv_usage == QTR_ONLY) {
    use_hp = 0;
  }

  av1_set_high_precision_mv(cpi, use_hp,
                            cpi->common.cur_frame_force_integer_mv);
}
