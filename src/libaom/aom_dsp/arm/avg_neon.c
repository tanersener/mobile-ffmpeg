/*
 *  Copyright (c) 2019, Alliance for Open Media. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <arm_neon.h>

#include "config/aom_dsp_rtcd.h"
#include "aom/aom_integer.h"
#include "aom_dsp/arm/sum_neon.h"
#include "av1/common/arm/mem_neon.h"
#include "av1/common/arm/transpose_neon.h"

unsigned int aom_avg_4x4_neon(const uint8_t *a, int a_stride) {
  const uint8x16_t b = load_unaligned_u8q(a, a_stride);
  const uint16x8_t c = vaddl_u8(vget_low_u8(b), vget_high_u8(b));
#if defined(__aarch64__)
  const uint32_t d = vaddlvq_u16(c);
  return d >> 4;
#else
  const uint32x2_t d = horizontal_add_u16x8(c);
  return vget_lane_u32(vrshr_n_u32(d, 4), 0);
#endif
}

unsigned int aom_avg_8x8_neon(const uint8_t *a, int a_stride) {
  uint16x8_t sum;
  uint32x2_t d;
  uint8x8_t b = vld1_u8(a);
  a += a_stride;
  uint8x8_t c = vld1_u8(a);
  a += a_stride;
  sum = vaddl_u8(b, c);

  for (int i = 0; i < 6; ++i) {
    const uint8x8_t e = vld1_u8(a);
    a += a_stride;
    sum = vaddw_u8(sum, e);
  }

  d = horizontal_add_u16x8(sum);

  return vget_lane_u32(vrshr_n_u32(d, 6), 0);
}
