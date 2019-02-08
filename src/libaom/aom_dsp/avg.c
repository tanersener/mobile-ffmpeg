/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <stdlib.h>

#include "config/aom_dsp_rtcd.h"
#include "aom_ports/mem.h"

void aom_minmax_8x8_c(const uint8_t *s, int p, const uint8_t *d, int dp,
                      int *min, int *max) {
  int i, j;
  *min = 255;
  *max = 0;
  for (i = 0; i < 8; ++i, s += p, d += dp) {
    for (j = 0; j < 8; ++j) {
      int diff = abs(s[j] - d[j]);
      *min = diff < *min ? diff : *min;
      *max = diff > *max ? diff : *max;
    }
  }
}

unsigned int aom_avg_4x4_c(const uint8_t *s, int p) {
  int i, j;
  int sum = 0;
  for (i = 0; i < 4; ++i, s += p)
    for (j = 0; j < 4; sum += s[j], ++j) {
    }

  return (sum + 8) >> 4;
}

unsigned int aom_avg_8x8_c(const uint8_t *s, int p) {
  int i, j;
  int sum = 0;
  for (i = 0; i < 8; ++i, s += p)
    for (j = 0; j < 8; sum += s[j], ++j) {
    }

  return (sum + 32) >> 6;
}

// src_diff: first pass, 9 bit, dynamic range [-255, 255]
//           second pass, 12 bit, dynamic range [-2040, 2040]
static void hadamard_col8(const int16_t *src_diff, ptrdiff_t src_stride,
                          int16_t *coeff) {
  int16_t b0 = src_diff[0 * src_stride] + src_diff[1 * src_stride];
  int16_t b1 = src_diff[0 * src_stride] - src_diff[1 * src_stride];
  int16_t b2 = src_diff[2 * src_stride] + src_diff[3 * src_stride];
  int16_t b3 = src_diff[2 * src_stride] - src_diff[3 * src_stride];
  int16_t b4 = src_diff[4 * src_stride] + src_diff[5 * src_stride];
  int16_t b5 = src_diff[4 * src_stride] - src_diff[5 * src_stride];
  int16_t b6 = src_diff[6 * src_stride] + src_diff[7 * src_stride];
  int16_t b7 = src_diff[6 * src_stride] - src_diff[7 * src_stride];

  int16_t c0 = b0 + b2;
  int16_t c1 = b1 + b3;
  int16_t c2 = b0 - b2;
  int16_t c3 = b1 - b3;
  int16_t c4 = b4 + b6;
  int16_t c5 = b5 + b7;
  int16_t c6 = b4 - b6;
  int16_t c7 = b5 - b7;

  coeff[0] = c0 + c4;
  coeff[7] = c1 + c5;
  coeff[3] = c2 + c6;
  coeff[4] = c3 + c7;
  coeff[2] = c0 - c4;
  coeff[6] = c1 - c5;
  coeff[1] = c2 - c6;
  coeff[5] = c3 - c7;
}

// The order of the output coeff of the hadamard is not important. For
// optimization purposes the final transpose may be skipped.
void aom_hadamard_8x8_c(const int16_t *src_diff, ptrdiff_t src_stride,
                        tran_low_t *coeff) {
  int idx;
  int16_t buffer[64];
  int16_t buffer2[64];
  int16_t *tmp_buf = &buffer[0];
  for (idx = 0; idx < 8; ++idx) {
    hadamard_col8(src_diff, src_stride, tmp_buf);  // src_diff: 9 bit
                                                   // dynamic range [-255, 255]
    tmp_buf += 8;
    ++src_diff;
  }

  tmp_buf = &buffer[0];
  for (idx = 0; idx < 8; ++idx) {
    hadamard_col8(tmp_buf, 8, buffer2 + 8 * idx);  // tmp_buf: 12 bit
    // dynamic range [-2040, 2040]
    // buffer2: 15 bit
    // dynamic range [-16320, 16320]
    ++tmp_buf;
  }

  for (idx = 0; idx < 64; ++idx) coeff[idx] = (tran_low_t)buffer2[idx];
}

// In place 16x16 2D Hadamard transform
void aom_hadamard_16x16_c(const int16_t *src_diff, ptrdiff_t src_stride,
                          tran_low_t *coeff) {
  int idx;
  for (idx = 0; idx < 4; ++idx) {
    // src_diff: 9 bit, dynamic range [-255, 255]
    const int16_t *src_ptr =
        src_diff + (idx >> 1) * 8 * src_stride + (idx & 0x01) * 8;
    aom_hadamard_8x8_c(src_ptr, src_stride, coeff + idx * 64);
  }

  // coeff: 15 bit, dynamic range [-16320, 16320]
  for (idx = 0; idx < 64; ++idx) {
    tran_low_t a0 = coeff[0];
    tran_low_t a1 = coeff[64];
    tran_low_t a2 = coeff[128];
    tran_low_t a3 = coeff[192];

    tran_low_t b0 = (a0 + a1) >> 1;  // (a0 + a1): 16 bit, [-32640, 32640]
    tran_low_t b1 = (a0 - a1) >> 1;  // b0-b3: 15 bit, dynamic range
    tran_low_t b2 = (a2 + a3) >> 1;  // [-16320, 16320]
    tran_low_t b3 = (a2 - a3) >> 1;

    coeff[0] = b0 + b2;  // 16 bit, [-32640, 32640]
    coeff[64] = b1 + b3;
    coeff[128] = b0 - b2;
    coeff[192] = b1 - b3;

    ++coeff;
  }
}

void aom_hadamard_32x32_c(const int16_t *src_diff, ptrdiff_t src_stride,
                          tran_low_t *coeff) {
  int idx;
  for (idx = 0; idx < 4; ++idx) {
    // src_diff: 9 bit, dynamic range [-255, 255]
    const int16_t *src_ptr =
        src_diff + (idx >> 1) * 16 * src_stride + (idx & 0x01) * 16;
    aom_hadamard_16x16_c(src_ptr, src_stride, coeff + idx * 256);
  }

  // coeff: 15 bit, dynamic range [-16320, 16320]
  for (idx = 0; idx < 256; ++idx) {
    tran_low_t a0 = coeff[0];
    tran_low_t a1 = coeff[256];
    tran_low_t a2 = coeff[512];
    tran_low_t a3 = coeff[768];

    tran_low_t b0 = (a0 + a1) >> 2;  // (a0 + a1): 16 bit, [-32640, 32640]
    tran_low_t b1 = (a0 - a1) >> 2;  // b0-b3: 15 bit, dynamic range
    tran_low_t b2 = (a2 + a3) >> 2;  // [-16320, 16320]
    tran_low_t b3 = (a2 - a3) >> 2;

    coeff[0] = b0 + b2;  // 16 bit, [-32640, 32640]
    coeff[256] = b1 + b3;
    coeff[512] = b0 - b2;
    coeff[768] = b1 - b3;

    ++coeff;
  }
}

// coeff: 16 bits, dynamic range [-32640, 32640].
// length: value range {16, 64, 256, 1024}.
int aom_satd_c(const tran_low_t *coeff, int length) {
  int i;
  int satd = 0;
  for (i = 0; i < length; ++i) satd += abs(coeff[i]);

  // satd: 26 bits, dynamic range [-32640 * 1024, 32640 * 1024]
  return satd;
}
