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

#include <math.h>
#include <string.h>

#include "config/aom_scale_rtcd.h"

#include "aom/aom_integer.h"
#include "aom_ports/system_state.h"
#include "av1/common/cdef.h"
#include "av1/common/onyxc_int.h"
#include "av1/common/reconinter.h"
#include "av1/encoder/encoder.h"

#define REDUCED_PRI_STRENGTHS 8
#define REDUCED_TOTAL_STRENGTHS (REDUCED_PRI_STRENGTHS * CDEF_SEC_STRENGTHS)
#define TOTAL_STRENGTHS (CDEF_PRI_STRENGTHS * CDEF_SEC_STRENGTHS)

static int priconv[REDUCED_PRI_STRENGTHS] = { 0, 1, 2, 3, 5, 7, 10, 13 };

/* Search for the best strength to add as an option, knowing we
   already selected nb_strengths options. */
static uint64_t search_one(int *lev, int nb_strengths,
                           uint64_t mse[][TOTAL_STRENGTHS], int sb_count,
                           int fast) {
  uint64_t tot_mse[TOTAL_STRENGTHS];
  const int total_strengths = fast ? REDUCED_TOTAL_STRENGTHS : TOTAL_STRENGTHS;
  int i, j;
  uint64_t best_tot_mse = (uint64_t)1 << 63;
  int best_id = 0;
  memset(tot_mse, 0, sizeof(tot_mse));
  for (i = 0; i < sb_count; i++) {
    int gi;
    uint64_t best_mse = (uint64_t)1 << 63;
    /* Find best mse among already selected options. */
    for (gi = 0; gi < nb_strengths; gi++) {
      if (mse[i][lev[gi]] < best_mse) {
        best_mse = mse[i][lev[gi]];
      }
    }
    /* Find best mse when adding each possible new option. */
    for (j = 0; j < total_strengths; j++) {
      uint64_t best = best_mse;
      if (mse[i][j] < best) best = mse[i][j];
      tot_mse[j] += best;
    }
  }
  for (j = 0; j < total_strengths; j++) {
    if (tot_mse[j] < best_tot_mse) {
      best_tot_mse = tot_mse[j];
      best_id = j;
    }
  }
  lev[nb_strengths] = best_id;
  return best_tot_mse;
}

/* Search for the best luma+chroma strength to add as an option, knowing we
   already selected nb_strengths options. */
static uint64_t search_one_dual(int *lev0, int *lev1, int nb_strengths,
                                uint64_t (**mse)[TOTAL_STRENGTHS], int sb_count,
                                int fast) {
  uint64_t tot_mse[TOTAL_STRENGTHS][TOTAL_STRENGTHS];
  int i, j;
  uint64_t best_tot_mse = (uint64_t)1 << 63;
  int best_id0 = 0;
  int best_id1 = 0;
  const int total_strengths = fast ? REDUCED_TOTAL_STRENGTHS : TOTAL_STRENGTHS;
  memset(tot_mse, 0, sizeof(tot_mse));
  for (i = 0; i < sb_count; i++) {
    int gi;
    uint64_t best_mse = (uint64_t)1 << 63;
    /* Find best mse among already selected options. */
    for (gi = 0; gi < nb_strengths; gi++) {
      uint64_t curr = mse[0][i][lev0[gi]];
      curr += mse[1][i][lev1[gi]];
      if (curr < best_mse) {
        best_mse = curr;
      }
    }
    /* Find best mse when adding each possible new option. */
    for (j = 0; j < total_strengths; j++) {
      int k;
      for (k = 0; k < total_strengths; k++) {
        uint64_t best = best_mse;
        uint64_t curr = mse[0][i][j];
        curr += mse[1][i][k];
        if (curr < best) best = curr;
        tot_mse[j][k] += best;
      }
    }
  }
  for (j = 0; j < total_strengths; j++) {
    int k;
    for (k = 0; k < total_strengths; k++) {
      if (tot_mse[j][k] < best_tot_mse) {
        best_tot_mse = tot_mse[j][k];
        best_id0 = j;
        best_id1 = k;
      }
    }
  }
  lev0[nb_strengths] = best_id0;
  lev1[nb_strengths] = best_id1;
  return best_tot_mse;
}

/* Search for the set of strengths that minimizes mse. */
static uint64_t joint_strength_search(int *best_lev, int nb_strengths,
                                      uint64_t mse[][TOTAL_STRENGTHS],
                                      int sb_count, int fast) {
  uint64_t best_tot_mse;
  int i;
  best_tot_mse = (uint64_t)1 << 63;
  /* Greedy search: add one strength options at a time. */
  for (i = 0; i < nb_strengths; i++) {
    best_tot_mse = search_one(best_lev, i, mse, sb_count, fast);
  }
  /* Trying to refine the greedy search by reconsidering each
     already-selected option. */
  if (!fast) {
    for (i = 0; i < 4 * nb_strengths; i++) {
      int j;
      for (j = 0; j < nb_strengths - 1; j++) best_lev[j] = best_lev[j + 1];
      best_tot_mse =
          search_one(best_lev, nb_strengths - 1, mse, sb_count, fast);
    }
  }
  return best_tot_mse;
}

/* Search for the set of luma+chroma strengths that minimizes mse. */
static uint64_t joint_strength_search_dual(int *best_lev0, int *best_lev1,
                                           int nb_strengths,
                                           uint64_t (**mse)[TOTAL_STRENGTHS],
                                           int sb_count, int fast) {
  uint64_t best_tot_mse;
  int i;
  best_tot_mse = (uint64_t)1 << 63;
  /* Greedy search: add one strength options at a time. */
  for (i = 0; i < nb_strengths; i++) {
    best_tot_mse =
        search_one_dual(best_lev0, best_lev1, i, mse, sb_count, fast);
  }
  /* Trying to refine the greedy search by reconsidering each
     already-selected option. */
  for (i = 0; i < 4 * nb_strengths; i++) {
    int j;
    for (j = 0; j < nb_strengths - 1; j++) {
      best_lev0[j] = best_lev0[j + 1];
      best_lev1[j] = best_lev1[j + 1];
    }
    best_tot_mse = search_one_dual(best_lev0, best_lev1, nb_strengths - 1, mse,
                                   sb_count, fast);
  }
  return best_tot_mse;
}

/* FIXME: SSE-optimize this. */
static void copy_sb16_16(uint16_t *dst, int dstride, const uint16_t *src,
                         int src_voffset, int src_hoffset, int sstride,
                         int vsize, int hsize) {
  int r, c;
  const uint16_t *base = &src[src_voffset * sstride + src_hoffset];
  for (r = 0; r < vsize; r++) {
    for (c = 0; c < hsize; c++) {
      dst[r * dstride + c] = base[r * sstride + c];
    }
  }
}

#if CONFIG_DIST_8X8
static INLINE uint64_t dist_8x8_16bit(uint16_t *dst, int dstride, uint16_t *src,
                                      int sstride, int coeff_shift) {
  uint64_t svar = 0;
  uint64_t dvar = 0;
  uint64_t sum_s = 0;
  uint64_t sum_d = 0;
  uint64_t sum_s2 = 0;
  uint64_t sum_d2 = 0;
  uint64_t sum_sd = 0;
  int i, j;
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      sum_s += src[i * sstride + j];
      sum_d += dst[i * dstride + j];
      sum_s2 += src[i * sstride + j] * src[i * sstride + j];
      sum_d2 += dst[i * dstride + j] * dst[i * dstride + j];
      sum_sd += src[i * sstride + j] * dst[i * dstride + j];
    }
  }
  /* Compute the variance -- the calculation cannot go negative. */
  svar = sum_s2 - ((sum_s * sum_s + 32) >> 6);
  dvar = sum_d2 - ((sum_d * sum_d + 32) >> 6);
  return (uint64_t)floor(
      .5 + (sum_d2 + sum_s2 - 2 * sum_sd) * .5 *
               (svar + dvar + (400 << 2 * coeff_shift)) /
               (sqrt((20000 << 4 * coeff_shift) + svar * (double)dvar)));
}
#endif  // CONFIG_DIST_8X8

static INLINE uint64_t mse_8x8_16bit(uint16_t *dst, int dstride, uint16_t *src,
                                     int sstride) {
  uint64_t sum = 0;
  int i, j;
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      int e = dst[i * dstride + j] - src[i * sstride + j];
      sum += e * e;
    }
  }
  return sum;
}

static INLINE uint64_t mse_4x4_16bit(uint16_t *dst, int dstride, uint16_t *src,
                                     int sstride) {
  uint64_t sum = 0;
  int i, j;
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      int e = dst[i * dstride + j] - src[i * sstride + j];
      sum += e * e;
    }
  }
  return sum;
}

/* Compute MSE only on the blocks we filtered. */
static uint64_t compute_cdef_dist(uint16_t *dst, int dstride, uint16_t *src,
                                  cdef_list *dlist, int cdef_count,
                                  BLOCK_SIZE bsize, int coeff_shift, int pli) {
  uint64_t sum = 0;
  int bi, bx, by;
  if (bsize == BLOCK_8X8) {
    for (bi = 0; bi < cdef_count; bi++) {
      by = dlist[bi].by;
      bx = dlist[bi].bx;
      if (pli == 0) {
#if CONFIG_DIST_8X8
        sum += dist_8x8_16bit(&dst[(by << 3) * dstride + (bx << 3)], dstride,
                              &src[bi << (3 + 3)], 8, coeff_shift);
#else

        sum += mse_8x8_16bit(&dst[(by << 3) * dstride + (bx << 3)], dstride,
                             &src[bi << (3 + 3)], 8);
#endif  // CONFIG_DIST_8X8

      } else {
        sum += mse_8x8_16bit(&dst[(by << 3) * dstride + (bx << 3)], dstride,
                             &src[bi << (3 + 3)], 8);
      }
    }
  } else if (bsize == BLOCK_4X8) {
    for (bi = 0; bi < cdef_count; bi++) {
      by = dlist[bi].by;
      bx = dlist[bi].bx;
      sum += mse_4x4_16bit(&dst[(by << 3) * dstride + (bx << 2)], dstride,
                           &src[bi << (3 + 2)], 4);
      sum += mse_4x4_16bit(&dst[((by << 3) + 4) * dstride + (bx << 2)], dstride,
                           &src[(bi << (3 + 2)) + 4 * 4], 4);
    }
  } else if (bsize == BLOCK_8X4) {
    for (bi = 0; bi < cdef_count; bi++) {
      by = dlist[bi].by;
      bx = dlist[bi].bx;
      sum += mse_4x4_16bit(&dst[(by << 2) * dstride + (bx << 3)], dstride,
                           &src[bi << (2 + 3)], 8);
      sum += mse_4x4_16bit(&dst[(by << 2) * dstride + (bx << 3) + 4], dstride,
                           &src[(bi << (2 + 3)) + 4], 8);
    }
  } else {
    assert(bsize == BLOCK_4X4);
    for (bi = 0; bi < cdef_count; bi++) {
      by = dlist[bi].by;
      bx = dlist[bi].bx;
      sum += mse_4x4_16bit(&dst[(by << 2) * dstride + (bx << 2)], dstride,
                           &src[bi << (2 + 2)], 4);
    }
  }
  return sum >> 2 * coeff_shift;
}

static int sb_all_skip(const AV1_COMMON *const cm, int mi_row, int mi_col) {
  const int maxr = AOMMIN(cm->mi_rows - mi_row, MI_SIZE_64X64);
  const int maxc = AOMMIN(cm->mi_cols - mi_col, MI_SIZE_64X64);
  const int stride = cm->mi_stride;
  MB_MODE_INFO **mbmi = cm->mi_grid_visible + mi_row * stride + mi_col;
  for (int r = 0; r < maxr; ++r, mbmi += stride) {
    for (int c = 0; c < maxc; ++c) {
      if (!mbmi[c]->skip) return 0;
    }
  }
  return 1;
}

static void pick_cdef_from_qp(AV1_COMMON *const cm) {
  const int bd = cm->seq_params.bit_depth;
  const int q = av1_ac_quant_QTX(cm->base_qindex, 0, bd) >> (bd - 8);
  CdefInfo *const cdef_info = &cm->cdef_info;
  cdef_info->cdef_bits = 0;
  cdef_info->nb_cdef_strengths = 1;
  cdef_info->cdef_damping = 3 + (cm->base_qindex >> 6);

  int predicted_y_f1 = 0;
  int predicted_y_f2 = 0;
  int predicted_uv_f1 = 0;
  int predicted_uv_f2 = 0;
  aom_clear_system_state();
  if (!frame_is_intra_only(cm)) {
    predicted_y_f1 = clamp((int)roundf(-q * q * 0.00000235939456f +
                                       q * 0.0068615186f + 0.02709886f),
                           0, 15);
    predicted_y_f2 = clamp((int)roundf(-q * q * 0.000000576297339f +
                                       q * 0.00139933452f + 0.03831067f),
                           0, 3);
    predicted_uv_f1 = clamp((int)roundf(-q * q * 0.000000709506878f +
                                        q * 0.00346288458f + 0.00887099f),
                            0, 15);
    predicted_uv_f2 = clamp((int)roundf(q * q * 0.000000238740853f +
                                        q * 0.000282235851f + 0.05576307f),
                            0, 3);
  } else {
    predicted_y_f1 = clamp(
        (int)roundf(q * q * 0.00000337319739f + q * 0.0080705937f + 0.0187634f),
        0, 15);
    predicted_y_f2 = clamp((int)roundf(-q * q * -0.00000291673427f +
                                       q * 0.00277986238f + 0.0079405f),
                           0, 3);
    predicted_uv_f1 = clamp((int)roundf(-q * q * 0.0000130790995f +
                                        q * 0.0128924046f - 0.00748388f),
                            0, 15);
    predicted_uv_f2 = clamp((int)roundf(q * q * 0.00000326517829f +
                                        q * 0.000355201832f + 0.00228092f),
                            0, 3);
  }
  cdef_info->cdef_strengths[0] =
      predicted_y_f1 * CDEF_SEC_STRENGTHS + predicted_y_f2;
  cdef_info->cdef_uv_strengths[0] =
      predicted_uv_f1 * CDEF_SEC_STRENGTHS + predicted_uv_f2;

  const int nvfb = (cm->mi_rows + MI_SIZE_64X64 - 1) / MI_SIZE_64X64;
  const int nhfb = (cm->mi_cols + MI_SIZE_64X64 - 1) / MI_SIZE_64X64;
  MB_MODE_INFO **mbmi = cm->mi_grid_visible;
  for (int r = 0; r < nvfb; ++r) {
    for (int c = 0; c < nhfb; ++c) {
      mbmi[MI_SIZE_64X64 * c]->cdef_strength = 0;
    }
    mbmi += MI_SIZE_64X64 * cm->mi_stride;
  }
}

void av1_cdef_search(YV12_BUFFER_CONFIG *frame, const YV12_BUFFER_CONFIG *ref,
                     AV1_COMMON *cm, MACROBLOCKD *xd, int pick_method) {
  if (pick_method == CDEF_PICK_FROM_Q) {
    pick_cdef_from_qp(cm);
    return;
  }

  uint16_t *src[3];
  uint16_t *ref_coeff[3];
  static cdef_list dlist[MI_SIZE_128X128 * MI_SIZE_128X128];
  int dir[CDEF_NBLOCKS][CDEF_NBLOCKS] = { { 0 } };
  int var[CDEF_NBLOCKS][CDEF_NBLOCKS] = { { 0 } };
  const int nvfb = (cm->mi_rows + MI_SIZE_64X64 - 1) / MI_SIZE_64X64;
  const int nhfb = (cm->mi_cols + MI_SIZE_64X64 - 1) / MI_SIZE_64X64;
  int *sb_index = aom_malloc(nvfb * nhfb * sizeof(*sb_index));
  int *selected_strength = aom_malloc(nvfb * nhfb * sizeof(*sb_index));
  const int damping = 3 + (cm->base_qindex >> 6);
  const int fast = pick_method == CDEF_FAST_SEARCH;
  const int total_strengths = fast ? REDUCED_TOTAL_STRENGTHS : TOTAL_STRENGTHS;
  DECLARE_ALIGNED(32, uint16_t, tmp_dst[1 << (MAX_SB_SIZE_LOG2 * 2)]);
  const int num_planes = av1_num_planes(cm);
  av1_setup_dst_planes(xd->plane, cm->seq_params.sb_size, frame, 0, 0, 0,
                       num_planes);
  uint64_t(*mse[2])[TOTAL_STRENGTHS];
  mse[0] = aom_malloc(sizeof(**mse) * nvfb * nhfb);
  mse[1] = aom_malloc(sizeof(**mse) * nvfb * nhfb);

  int stride[3];
  int bsize[3];
  int mi_wide_l2[3];
  int mi_high_l2[3];
  int xdec[3];
  int ydec[3];
  for (int pli = 0; pli < num_planes; pli++) {
    uint8_t *ref_buffer;
    int ref_stride;
    switch (pli) {
      case 0:
        ref_buffer = ref->y_buffer;
        ref_stride = ref->y_stride;
        break;
      case 1:
        ref_buffer = ref->u_buffer;
        ref_stride = ref->uv_stride;
        break;
      case 2:
        ref_buffer = ref->v_buffer;
        ref_stride = ref->uv_stride;
        break;
    }
    src[pli] = aom_memalign(
        32, sizeof(*src) * cm->mi_rows * cm->mi_cols * MI_SIZE * MI_SIZE);
    ref_coeff[pli] = aom_memalign(
        32, sizeof(*ref_coeff) * cm->mi_rows * cm->mi_cols * MI_SIZE * MI_SIZE);
    xdec[pli] = xd->plane[pli].subsampling_x;
    ydec[pli] = xd->plane[pli].subsampling_y;
    bsize[pli] = ydec[pli] ? (xdec[pli] ? BLOCK_4X4 : BLOCK_8X4)
                           : (xdec[pli] ? BLOCK_4X8 : BLOCK_8X8);
    stride[pli] = cm->mi_cols << MI_SIZE_LOG2;
    mi_wide_l2[pli] = MI_SIZE_LOG2 - xd->plane[pli].subsampling_x;
    mi_high_l2[pli] = MI_SIZE_LOG2 - xd->plane[pli].subsampling_y;

    const int frame_height =
        (cm->mi_rows * MI_SIZE) >> xd->plane[pli].subsampling_y;
    const int frame_width =
        (cm->mi_cols * MI_SIZE) >> xd->plane[pli].subsampling_x;
    const int plane_sride = stride[pli];
    const int dst_stride = xd->plane[pli].dst.stride;
    for (int r = 0; r < frame_height; ++r) {
      for (int c = 0; c < frame_width; ++c) {
        if (cm->seq_params.use_highbitdepth) {
          src[pli][r * plane_sride + c] =
              CONVERT_TO_SHORTPTR(xd->plane[pli].dst.buf)[r * dst_stride + c];
          ref_coeff[pli][r * plane_sride + c] =
              CONVERT_TO_SHORTPTR(ref_buffer)[r * ref_stride + c];
        } else {
          src[pli][r * plane_sride + c] =
              xd->plane[pli].dst.buf[r * dst_stride + c];
          ref_coeff[pli][r * plane_sride + c] = ref_buffer[r * ref_stride + c];
        }
      }
    }
  }

  DECLARE_ALIGNED(32, uint16_t, inbuf[CDEF_INBUF_SIZE]);
  uint16_t *const in = inbuf + CDEF_VBORDER * CDEF_BSTRIDE + CDEF_HBORDER;
  const int coeff_shift = AOMMAX(cm->seq_params.bit_depth - 8, 0);
  int sb_count = 0;
  for (int fbr = 0; fbr < nvfb; ++fbr) {
    for (int fbc = 0; fbc < nhfb; ++fbc) {
      // No filtering if the entire filter block is skipped
      if (sb_all_skip(cm, fbr * MI_SIZE_64X64, fbc * MI_SIZE_64X64)) continue;

      const MB_MODE_INFO *const mbmi =
          cm->mi_grid_visible[MI_SIZE_64X64 * fbr * cm->mi_stride +
                              MI_SIZE_64X64 * fbc];
      if (((fbc & 1) &&
           (mbmi->sb_type == BLOCK_128X128 || mbmi->sb_type == BLOCK_128X64)) ||
          ((fbr & 1) &&
           (mbmi->sb_type == BLOCK_128X128 || mbmi->sb_type == BLOCK_64X128)))
        continue;

      int nhb = AOMMIN(MI_SIZE_64X64, cm->mi_cols - MI_SIZE_64X64 * fbc);
      int nvb = AOMMIN(MI_SIZE_64X64, cm->mi_rows - MI_SIZE_64X64 * fbr);
      int hb_step = 1;
      int vb_step = 1;
      BLOCK_SIZE bs;
      if (mbmi->sb_type == BLOCK_128X128 || mbmi->sb_type == BLOCK_128X64 ||
          mbmi->sb_type == BLOCK_64X128) {
        bs = mbmi->sb_type;
        if (bs == BLOCK_128X128 || bs == BLOCK_128X64) {
          nhb = AOMMIN(MI_SIZE_128X128, cm->mi_cols - MI_SIZE_64X64 * fbc);
          hb_step = 2;
        }
        if (bs == BLOCK_128X128 || bs == BLOCK_64X128) {
          nvb = AOMMIN(MI_SIZE_128X128, cm->mi_rows - MI_SIZE_64X64 * fbr);
          vb_step = 2;
        }
      } else {
        bs = BLOCK_64X64;
      }

      const int cdef_count = av1_cdef_compute_sb_list(
          cm, fbr * MI_SIZE_64X64, fbc * MI_SIZE_64X64, dlist, bs);

      const int yoff = CDEF_VBORDER * (fbr != 0);
      const int xoff = CDEF_HBORDER * (fbc != 0);
      int dirinit = 0;
      for (int pli = 0; pli < num_planes; pli++) {
        for (int i = 0; i < CDEF_INBUF_SIZE; i++) inbuf[i] = CDEF_VERY_LARGE;
        /* We avoid filtering the pixels for which some of the pixels to average
           are outside the frame. We could change the filter instead, but it
           would add special cases for any future vectorization. */
        const int ysize = (nvb << mi_high_l2[pli]) +
                          CDEF_VBORDER * (fbr + vb_step < nvfb) + yoff;
        const int xsize = (nhb << mi_wide_l2[pli]) +
                          CDEF_HBORDER * (fbc + hb_step < nhfb) + xoff;
        const int row = fbr * MI_SIZE_64X64 << mi_high_l2[pli];
        const int col = fbc * MI_SIZE_64X64 << mi_wide_l2[pli];
        for (int gi = 0; gi < total_strengths; gi++) {
          int pri_strength = gi / CDEF_SEC_STRENGTHS;
          if (fast) pri_strength = priconv[pri_strength];
          const int sec_strength = gi % CDEF_SEC_STRENGTHS;
          copy_sb16_16(&in[(-yoff * CDEF_BSTRIDE - xoff)], CDEF_BSTRIDE,
                       src[pli], row - yoff, col - xoff, stride[pli], ysize,
                       xsize);
          av1_cdef_filter_fb(
              NULL, tmp_dst, CDEF_BSTRIDE, in, xdec[pli], ydec[pli], dir,
              &dirinit, var, pli, dlist, cdef_count, pri_strength,
              sec_strength + (sec_strength == 3), damping, coeff_shift);
          const uint64_t curr_mse = compute_cdef_dist(
              ref_coeff[pli] + row * stride[pli] + col, stride[pli], tmp_dst,
              dlist, cdef_count, bsize[pli], coeff_shift, pli);
          if (pli < 2)
            mse[pli][sb_count][gi] = curr_mse;
          else
            mse[1][sb_count][gi] += curr_mse;
        }
      }
      sb_index[sb_count++] =
          MI_SIZE_64X64 * fbr * cm->mi_stride + MI_SIZE_64X64 * fbc;
    }
  }

  /* Search for different number of signalling bits. */
  int nb_strength_bits = 0;
  uint64_t best_tot_mse = UINT64_MAX;
  const int quantizer =
      av1_ac_quant_QTX(cm->base_qindex, 0, cm->seq_params.bit_depth) >>
      (cm->seq_params.bit_depth - 8);
  aom_clear_system_state();
  const double lambda = .12 * quantizer * quantizer / 256.;
  CdefInfo *const cdef_info = &cm->cdef_info;
  for (int i = 0; i <= 3; i++) {
    int best_lev0[CDEF_MAX_STRENGTHS];
    int best_lev1[CDEF_MAX_STRENGTHS] = { 0 };
    const int nb_strengths = 1 << i;
    uint64_t tot_mse;
    if (num_planes >= 3) {
      tot_mse = joint_strength_search_dual(best_lev0, best_lev1, nb_strengths,
                                           mse, sb_count, fast);
    } else {
      tot_mse = joint_strength_search(best_lev0, nb_strengths, mse[0], sb_count,
                                      fast);
    }
    /* Count superblock signalling cost. */
    tot_mse += (uint64_t)(sb_count * lambda * i);
    /* Count header signalling cost. */
    tot_mse += (uint64_t)(nb_strengths * lambda * CDEF_STRENGTH_BITS *
                          (num_planes > 1 ? 2 : 1));
    if (tot_mse < best_tot_mse) {
      best_tot_mse = tot_mse;
      nb_strength_bits = i;
      memcpy(cdef_info->cdef_strengths, best_lev0,
             nb_strengths * sizeof(best_lev0[0]));
      if (num_planes > 1) {
        memcpy(cdef_info->cdef_uv_strengths, best_lev1,
               nb_strengths * sizeof(best_lev1[0]));
      }
    }
  }

  cdef_info->cdef_bits = nb_strength_bits;
  cdef_info->nb_cdef_strengths = 1 << nb_strength_bits;
  for (int i = 0; i < sb_count; i++) {
    uint64_t best_mse = UINT64_MAX;
    int best_gi = 0;
    for (int gi = 0; gi < cdef_info->nb_cdef_strengths; gi++) {
      uint64_t curr = mse[0][i][cdef_info->cdef_strengths[gi]];
      if (num_planes > 1) curr += mse[1][i][cdef_info->cdef_uv_strengths[gi]];
      if (curr < best_mse) {
        best_gi = gi;
        best_mse = curr;
      }
    }
    selected_strength[i] = best_gi;
    cm->mi_grid_visible[sb_index[i]]->cdef_strength = best_gi;
  }

  if (fast) {
    for (int j = 0; j < cdef_info->nb_cdef_strengths; j++) {
      const int luma_strength = cdef_info->cdef_strengths[j];
      const int chroma_strength = cdef_info->cdef_uv_strengths[j];
      cdef_info->cdef_strengths[j] =
          priconv[luma_strength / CDEF_SEC_STRENGTHS] * CDEF_SEC_STRENGTHS +
          (luma_strength % CDEF_SEC_STRENGTHS);
      cdef_info->cdef_uv_strengths[j] =
          priconv[chroma_strength / CDEF_SEC_STRENGTHS] * CDEF_SEC_STRENGTHS +
          (chroma_strength % CDEF_SEC_STRENGTHS);
    }
  }

  cdef_info->cdef_damping = damping;

  aom_free(mse[0]);
  aom_free(mse[1]);
  for (int pli = 0; pli < num_planes; pli++) {
    aom_free(src[pli]);
    aom_free(ref_coeff[pli]);
  }
  aom_free(sb_index);
  aom_free(selected_strength);
}
