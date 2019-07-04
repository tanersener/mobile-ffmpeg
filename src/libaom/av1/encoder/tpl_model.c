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

#include <stdint.h>
#include <float.h>

#include "config/aom_config.h"
#include "config/aom_dsp_rtcd.h"

#include "aom/aom_codec.h"

#include "av1/common/enums.h"
#include "av1/common/onyxc_int.h"
#include "av1/common/reconintra.h"

#include "av1/encoder/encoder.h"
#include "av1/encoder/reconinter_enc.h"

#define MC_FLOW_BSIZE 16
#define MC_FLOW_NUM_PELS (MC_FLOW_BSIZE * MC_FLOW_BSIZE)

static void wht_fwd_txfm(int16_t *src_diff, int bw, tran_low_t *coeff,
                         TX_SIZE tx_size, int is_hbd) {
  if (is_hbd) {
    switch (tx_size) {
      case TX_8X8: aom_highbd_hadamard_8x8(src_diff, bw, coeff); break;
      case TX_16X16: aom_highbd_hadamard_16x16(src_diff, bw, coeff); break;
      case TX_32X32: aom_highbd_hadamard_32x32(src_diff, bw, coeff); break;
      default: assert(0);
    }
  } else {
    switch (tx_size) {
      case TX_8X8: aom_hadamard_8x8(src_diff, bw, coeff); break;
      case TX_16X16: aom_hadamard_16x16(src_diff, bw, coeff); break;
      case TX_32X32: aom_hadamard_32x32(src_diff, bw, coeff); break;
      default: assert(0);
    }
  }
}

static uint32_t motion_estimation(AV1_COMP *cpi, MACROBLOCK *x,
                                  uint8_t *cur_frame_buf,
                                  uint8_t *ref_frame_buf, int stride,
                                  int stride_ref, BLOCK_SIZE bsize, int mi_row,
                                  int mi_col) {
  AV1_COMMON *cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  MV_SPEED_FEATURES *const mv_sf = &cpi->sf.mv;
  const SEARCH_METHODS search_method = NSTEP;
  int step_param;
  int sadpb = x->sadperbit16;
  uint32_t bestsme = UINT_MAX;
  int distortion;
  uint32_t sse;
  int cost_list[5];
  const MvLimits tmp_mv_limits = x->mv_limits;
  search_site_config ss_cfg;

  MV best_ref_mv1 = { 0, 0 };
  MV best_ref_mv1_full; /* full-pixel value of best_ref_mv1 */

  best_ref_mv1_full.col = best_ref_mv1.col >> 3;
  best_ref_mv1_full.row = best_ref_mv1.row >> 3;

  // Setup frame pointers
  x->plane[0].src.buf = cur_frame_buf;
  x->plane[0].src.stride = stride;
  xd->plane[0].pre[0].buf = ref_frame_buf;
  xd->plane[0].pre[0].stride = stride_ref;

  step_param = mv_sf->reduce_first_step_size;
  step_param = AOMMIN(step_param, MAX_MVSEARCH_STEPS - 2);

  av1_set_mv_search_range(&x->mv_limits, &best_ref_mv1);

  av1_init3smotion_compensation(&ss_cfg, stride_ref);
  av1_full_pixel_search(cpi, x, bsize, &best_ref_mv1_full, step_param,
                        search_method, 0, sadpb, cond_cost_list(cpi, cost_list),
                        &best_ref_mv1, INT_MAX, 0, (MI_SIZE * mi_col),
                        (MI_SIZE * mi_row), 0, &ss_cfg);

  /* restore UMV window */
  x->mv_limits = tmp_mv_limits;

  const int pw = block_size_wide[bsize];
  const int ph = block_size_high[bsize];
  bestsme = cpi->find_fractional_mv_step(
      x, cm, mi_row, mi_col, &best_ref_mv1, cpi->common.allow_high_precision_mv,
      x->errorperbit, &cpi->fn_ptr[bsize], 0, mv_sf->subpel_iters_per_step,
      cond_cost_list(cpi, cost_list), NULL, NULL, &distortion, &sse, NULL, NULL,
      0, 0, pw, ph, 1, 1);

  return bestsme;
}

static void mode_estimation(AV1_COMP *cpi, MACROBLOCK *x, MACROBLOCKD *xd,
                            struct scale_factors *sf, int frame_idx,
                            int16_t *src_diff, tran_low_t *coeff, int use_satd,
                            int mi_row, int mi_col, BLOCK_SIZE bsize,
                            TX_SIZE tx_size, YV12_BUFFER_CONFIG *ref_frame[],
                            uint8_t *predictor, TplDepStats *tpl_stats) {
  AV1_COMMON *cm = &cpi->common;
  const GF_GROUP *gf_group = &cpi->gf_group;

  const int bw = 4 << mi_size_wide_log2[bsize];
  const int bh = 4 << mi_size_high_log2[bsize];
  const int pix_num = bw * bh;
  const int_interpfilters kernel =
      av1_broadcast_interp_filter(EIGHTTAP_REGULAR);

  int64_t best_intra_cost = INT64_MAX;
  int64_t intra_cost;
  PREDICTION_MODE mode;
  int mb_y_offset = mi_row * MI_SIZE * xd->cur_buf->y_stride + mi_col * MI_SIZE;

  memset(tpl_stats, 0, sizeof(*tpl_stats));

  xd->above_mbmi = NULL;
  xd->left_mbmi = NULL;
  xd->mi[0]->sb_type = bsize;
  xd->mi[0]->motion_mode = SIMPLE_TRANSLATION;

  const int q_cur = gf_group->q_val[frame_idx];
  const int16_t qstep_cur =
      ROUND_POWER_OF_TWO(av1_ac_quant_QTX(q_cur, 0, xd->bd), xd->bd - 8);
  const int qstep_cur_noise =
      use_satd ? ((int)qstep_cur * pix_num + 16) / (4 * 8)
               : ((int)qstep_cur * (int)qstep_cur * pix_num + 384) / (12 * 64);

  // Intra prediction search
  xd->mi[0]->ref_frame[0] = INTRA_FRAME;
  for (mode = DC_PRED; mode <= PAETH_PRED; ++mode) {
    uint8_t *src, *dst;
    int src_stride, dst_stride;

    src = xd->cur_buf->y_buffer + mb_y_offset;
    src_stride = xd->cur_buf->y_stride;

    dst = &predictor[0];
    dst_stride = bw;

    av1_predict_intra_block(
        cm, xd, block_size_wide[bsize], block_size_high[bsize], tx_size, mode,
        0, 0, FILTER_INTRA_MODES, src, src_stride, dst, dst_stride, 0, 0, 0);

    if (use_satd) {
      if (is_cur_buf_hbd(xd)) {
        aom_highbd_subtract_block(bh, bw, src_diff, bw, src, src_stride, dst,
                                  dst_stride, xd->bd);
      } else {
        aom_subtract_block(bh, bw, src_diff, bw, src, src_stride, dst,
                           dst_stride);
      }
      wht_fwd_txfm(src_diff, bw, coeff, tx_size, is_cur_buf_hbd(xd));
      intra_cost = aom_satd(coeff, pix_num);
    } else {
      int64_t sse;
      if (is_cur_buf_hbd(xd)) {
        sse = aom_highbd_sse(xd->cur_buf->y_buffer + mb_y_offset,
                             xd->cur_buf->y_stride, &predictor[0], bw, bw, bh);
      } else {
        sse = aom_sse(xd->cur_buf->y_buffer + mb_y_offset,
                      xd->cur_buf->y_stride, &predictor[0], bw, bw, bh);
      }
      intra_cost = ROUND_POWER_OF_TWO(sse, (xd->bd - 8) * 2);
    }
    intra_cost += qstep_cur_noise;

    if (intra_cost < best_intra_cost) best_intra_cost = intra_cost;
  }

  // Motion compensated prediction
  xd->mi[0]->ref_frame[0] = GOLDEN_FRAME;

  int best_rf_idx = -1;
  int_mv best_mv;
  int64_t inter_cost;
  int64_t best_inter_cost;
  int64_t inter_cost_weighted;
  int64_t best_inter_cost_weighted = INT64_MAX;
  int rf_idx;

  best_mv.as_int = 0;

  for (rf_idx = 0; rf_idx < INTER_REFS_PER_FRAME; ++rf_idx) {
    if (ref_frame[rf_idx] == NULL) continue;

    int q_ref = gf_group->q_val[gf_group->ref_frame_gop_idx[frame_idx][rf_idx]];

    const int16_t qstep_ref =
        ROUND_POWER_OF_TWO(av1_ac_quant_QTX(q_ref, 0, xd->bd), xd->bd - 8);
    const int qstep_ref_noise =
        use_satd
            ? ((int)qstep_ref * pix_num + 16) / (4 * 8)
            : ((int)qstep_ref * (int)qstep_ref * pix_num + 384) / (12 * 64);

    int mb_y_offset_ref =
        mi_row * MI_SIZE * ref_frame[rf_idx]->y_stride + mi_col * MI_SIZE;

    motion_estimation(cpi, x, xd->cur_buf->y_buffer + mb_y_offset,
                      ref_frame[rf_idx]->y_buffer + mb_y_offset_ref,
                      xd->cur_buf->y_stride, ref_frame[rf_idx]->y_stride, bsize,
                      mi_row, mi_col);

    ConvolveParams conv_params = get_conv_params(0, 0, xd->bd);
    WarpTypesAllowed warp_types;
    memset(&warp_types, 0, sizeof(WarpTypesAllowed));

    av1_build_inter_predictor(ref_frame[rf_idx]->y_buffer + mb_y_offset_ref,
                              ref_frame[rf_idx]->y_stride, &predictor[0], bw,
                              &x->best_mv.as_mv, sf, bw, bh, &conv_params,
                              kernel, &warp_types, mi_col * MI_SIZE,
                              mi_row * MI_SIZE, 0, 0, MV_PRECISION_Q3,
                              mi_col * MI_SIZE, mi_row * MI_SIZE, xd, 0);
    if (use_satd) {
      if (is_cur_buf_hbd(xd)) {
        aom_highbd_subtract_block(
            bh, bw, src_diff, bw, xd->cur_buf->y_buffer + mb_y_offset,
            xd->cur_buf->y_stride, &predictor[0], bw, xd->bd);
      } else {
        aom_subtract_block(bh, bw, src_diff, bw,
                           xd->cur_buf->y_buffer + mb_y_offset,
                           xd->cur_buf->y_stride, &predictor[0], bw);
      }
      wht_fwd_txfm(src_diff, bw, coeff, tx_size, is_cur_buf_hbd(xd));

      inter_cost = aom_satd(coeff, pix_num);
    } else {
      int64_t sse;
      if (is_cur_buf_hbd(xd)) {
        sse = aom_highbd_sse(xd->cur_buf->y_buffer + mb_y_offset,
                             xd->cur_buf->y_stride, &predictor[0], bw, bw, bh);
      } else {
        sse = aom_sse(xd->cur_buf->y_buffer + mb_y_offset,
                      xd->cur_buf->y_stride, &predictor[0], bw, bw, bh);
      }
      inter_cost = ROUND_POWER_OF_TWO(sse, (xd->bd - 8) * 2);
    }
    inter_cost_weighted = inter_cost + qstep_ref_noise;

    if (inter_cost_weighted < best_inter_cost_weighted) {
      best_rf_idx = rf_idx;
      best_inter_cost_weighted = inter_cost_weighted;
      best_mv.as_int = x->best_mv.as_int;
    }
  }
  best_intra_cost = AOMMAX(best_intra_cost, 1);
  if (frame_idx == 0)
    best_inter_cost = 0;
  else
    best_inter_cost =
        AOMMIN(best_intra_cost, (int64_t)best_inter_cost_weighted);
  tpl_stats->inter_cost = best_inter_cost << TPL_DEP_COST_SCALE_LOG2;
  tpl_stats->intra_cost = best_intra_cost << TPL_DEP_COST_SCALE_LOG2;

  if (frame_idx) {
    const int idx = gf_group->ref_frame_gop_idx[frame_idx][best_rf_idx];
    tpl_stats->ref_frame_index = idx;
    tpl_stats->ref_disp_frame_index = gf_group->frame_disp_idx[idx];
    tpl_stats->mv.as_int = best_mv.as_int;
  }
}

static int round_floor(int ref_pos, int bsize_pix) {
  int round;
  if (ref_pos < 0)
    round = -(1 + (-ref_pos - 1) / bsize_pix);
  else
    round = ref_pos / bsize_pix;

  return round;
}

static int get_overlap_area(int grid_pos_row, int grid_pos_col, int ref_pos_row,
                            int ref_pos_col, int block, BLOCK_SIZE bsize) {
  int width = 0, height = 0;
  int bw = 4 << mi_size_wide_log2[bsize];
  int bh = 4 << mi_size_high_log2[bsize];

  switch (block) {
    case 0:
      width = grid_pos_col + bw - ref_pos_col;
      height = grid_pos_row + bh - ref_pos_row;
      break;
    case 1:
      width = ref_pos_col + bw - grid_pos_col;
      height = grid_pos_row + bh - ref_pos_row;
      break;
    case 2:
      width = grid_pos_col + bw - ref_pos_col;
      height = ref_pos_row + bh - grid_pos_row;
      break;
    case 3:
      width = ref_pos_col + bw - grid_pos_col;
      height = ref_pos_row + bh - grid_pos_row;
      break;
    default: assert(0);
  }

  return width * height;
}

static void tpl_model_update_b(TplDepFrame *tpl_frame,
                               TplDepStats *tpl_stats_ptr, int mi_row,
                               int mi_col, const BLOCK_SIZE bsize) {
  TplDepFrame *ref_tpl_frame = &tpl_frame[tpl_stats_ptr->ref_disp_frame_index];
  TplDepStats *ref_stats_ptr = ref_tpl_frame->tpl_stats_ptr;
  MV mv = tpl_stats_ptr->mv.as_mv;
  int mv_row = mv.row >> 3;
  int mv_col = mv.col >> 3;

  int ref_pos_row = mi_row * MI_SIZE + mv_row;
  int ref_pos_col = mi_col * MI_SIZE + mv_col;

  const int bw = 4 << mi_size_wide_log2[bsize];
  const int bh = 4 << mi_size_high_log2[bsize];
  const int mi_height = mi_size_high[bsize];
  const int mi_width = mi_size_wide[bsize];
  const int pix_num = bw * bh;

  // top-left on grid block location in pixel
  int grid_pos_row_base = round_floor(ref_pos_row, bh) * bh;
  int grid_pos_col_base = round_floor(ref_pos_col, bw) * bw;
  int block;

  for (block = 0; block < 4; ++block) {
    int grid_pos_row = grid_pos_row_base + bh * (block >> 1);
    int grid_pos_col = grid_pos_col_base + bw * (block & 0x01);

    if (grid_pos_row >= 0 && grid_pos_row < ref_tpl_frame->mi_rows * MI_SIZE &&
        grid_pos_col >= 0 && grid_pos_col < ref_tpl_frame->mi_cols * MI_SIZE) {
      int overlap_area = get_overlap_area(
          grid_pos_row, grid_pos_col, ref_pos_row, ref_pos_col, block, bsize);
      int ref_mi_row = round_floor(grid_pos_row, bh) * mi_height;
      int ref_mi_col = round_floor(grid_pos_col, bw) * mi_width;

      int64_t mc_flow =
          tpl_stats_ptr->mc_dep_cost -
          (tpl_stats_ptr->mc_dep_cost * tpl_stats_ptr->inter_cost) /
              tpl_stats_ptr->intra_cost;
      int64_t mc_saved = tpl_stats_ptr->intra_cost - tpl_stats_ptr->inter_cost;
      int idx, idy;
      for (idy = 0; idy < mi_height; ++idy) {
        for (idx = 0; idx < mi_width; ++idx) {
          TplDepStats *des_stats =
              &ref_stats_ptr[(ref_mi_row + idy) * ref_tpl_frame->stride +
                             (ref_mi_col + idx)];
          des_stats->mc_flow += (mc_flow * overlap_area) / pix_num;
          des_stats->mc_count += overlap_area << TPL_DEP_COST_SCALE_LOG2;
          des_stats->mc_saved += (mc_saved * overlap_area) / pix_num;
          assert(overlap_area >= 0);
        }
      }
    }
  }
}

static void tpl_model_update(TplDepFrame *tpl_frame, TplDepStats *tpl_stats_ptr,
                             int mi_row, int mi_col, const BLOCK_SIZE bsize) {
  int idx, idy;
  const int mi_height = mi_size_high[bsize];
  const int mi_width = mi_size_wide[bsize];

  for (idy = 0; idy < mi_height; ++idy) {
    for (idx = 0; idx < mi_width; ++idx) {
      TplDepStats *tpl_ptr =
          &tpl_stats_ptr[(mi_row + idy) * tpl_frame->stride + (mi_col + idx)];
      tpl_model_update_b(tpl_frame, tpl_ptr, mi_row + idy, mi_col + idx,
                         BLOCK_4X4);
    }
  }
}

static void tpl_model_store(TplDepStats *tpl_stats_ptr, int mi_row, int mi_col,
                            BLOCK_SIZE bsize, int stride,
                            const TplDepStats *src_stats) {
  const int mi_height = mi_size_high[bsize];
  const int mi_width = mi_size_wide[bsize];
  int idx, idy;

  int64_t intra_cost = src_stats->intra_cost / (mi_height * mi_width);
  int64_t inter_cost = src_stats->inter_cost / (mi_height * mi_width);

  TplDepStats *tpl_ptr;

  intra_cost = AOMMAX(1, intra_cost);
  inter_cost = AOMMAX(1, inter_cost);

  for (idy = 0; idy < mi_height; ++idy) {
    tpl_ptr = &tpl_stats_ptr[(mi_row + idy) * stride + mi_col];
    for (idx = 0; idx < mi_width; ++idx) {
      tpl_ptr->intra_cost = intra_cost;
      tpl_ptr->inter_cost = inter_cost;
      tpl_ptr->mc_dep_cost = tpl_ptr->intra_cost + tpl_ptr->mc_flow;
      tpl_ptr->ref_frame_index = src_stats->ref_frame_index;
      tpl_ptr->ref_disp_frame_index = src_stats->ref_disp_frame_index;
      tpl_ptr->mv.as_int = src_stats->mv.as_int;
      ++tpl_ptr;
    }
  }
}

static YV12_BUFFER_CONFIG *get_framebuf(
    AV1_COMP *cpi, const EncodeFrameInput *const frame_input, int frame_idx) {
  if (frame_idx == 0) {
    RefCntBuffer *ref_buf = get_ref_frame_buf(&cpi->common, GOLDEN_FRAME);
    return &ref_buf->buf;
  } else if (frame_idx == 1) {
    return frame_input ? frame_input->source : NULL;
  } else {
    const GF_GROUP *gf_group = &cpi->gf_group;
    const int frame_disp_idx = gf_group->frame_disp_idx[frame_idx];
    struct lookahead_entry *buf = av1_lookahead_peek(
        cpi->lookahead, frame_disp_idx - cpi->num_gf_group_show_frames);
    return &buf->img;
  }
}

static void mc_flow_dispenser(AV1_COMP *cpi, YV12_BUFFER_CONFIG **gf_picture,
                              int frame_idx) {
  const GF_GROUP *gf_group = &cpi->gf_group;
  if (frame_idx == gf_group->size) return;
  int tpl_idx = gf_group->frame_disp_idx[frame_idx];
  TplDepFrame *tpl_frame = &cpi->tpl_stats[tpl_idx];
  YV12_BUFFER_CONFIG *this_frame = gf_picture[frame_idx];
  YV12_BUFFER_CONFIG *ref_frame[7] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL
  };

  AV1_COMMON *cm = &cpi->common;
  struct scale_factors sf;
  int rdmult, idx;
  ThreadData *td = &cpi->td;
  MACROBLOCK *x = &td->mb;
  MACROBLOCKD *xd = &x->e_mbd;
  int mi_row, mi_col;
#if MC_FLOW_BSIZE == 64
  const BLOCK_SIZE bsize = BLOCK_64X64;
#elif MC_FLOW_BSIZE == 32
  const BLOCK_SIZE bsize = BLOCK_32X32;
#elif MC_FLOW_BSIZE == 16
  const BLOCK_SIZE bsize = BLOCK_16X16;
#elif MC_FLOW_BSIZE == 8
  const BLOCK_SIZE bsize = BLOCK_8X8;
#elif MC_FLOW_BSIZE == 4
  const BLOCK_SIZE bsize = BLOCK_4X4;
#else
#error "Invalid block size for tpl model"
#endif  // MC_FLOW_BSIZE == 64
  av1_tile_init(&xd->tile, cm, 0, 0);

  DECLARE_ALIGNED(32, uint16_t, predictor16[MC_FLOW_NUM_PELS * 3]);
  DECLARE_ALIGNED(32, uint8_t, predictor8[MC_FLOW_NUM_PELS * 3]);
  uint8_t *predictor;
  DECLARE_ALIGNED(32, int16_t, src_diff[MC_FLOW_NUM_PELS]);
  DECLARE_ALIGNED(32, tran_low_t, coeff[MC_FLOW_NUM_PELS]);

  const TX_SIZE tx_size = max_txsize_lookup[bsize];
  const int mi_height = mi_size_high[bsize];
  const int mi_width = mi_size_wide[bsize];

  // Setup scaling factor
  av1_setup_scale_factors_for_frame(
      &sf, this_frame->y_crop_width, this_frame->y_crop_height,
      this_frame->y_crop_width, this_frame->y_crop_height);

  xd->cur_buf = this_frame;

  if (is_cur_buf_hbd(xd))
    predictor = CONVERT_TO_BYTEPTR(predictor16);
  else
    predictor = predictor8;

  // Prepare reference frame pointers. If any reference frame slot is
  // unavailable, the pointer will be set to Null.
  for (idx = 0; idx < INTER_REFS_PER_FRAME; ++idx) {
    const int rf_idx = gf_group->ref_frame_gop_idx[frame_idx][idx];
    if (rf_idx != -1) {
      int duplicate = 0;
      for (int idx2 = 0; idx2 < idx; ++idx2) {
        const int rf_idx2 = gf_group->ref_frame_gop_idx[frame_idx][idx2];
        if (rf_idx2 != -1 && gf_picture[rf_idx] == gf_picture[rf_idx2]) {
          duplicate = 1;
          break;
        }
      }
      if (!duplicate) ref_frame[idx] = gf_picture[rf_idx];
    }
  }

  xd->mi = cm->mi_grid_visible;
  xd->mi[0] = cm->mi;
  xd->block_ref_scale_factors[0] = &sf;

  const int base_qindex = gf_group->q_val[frame_idx];
  // Get rd multiplier set up.
  rdmult = (int)av1_compute_rd_mult(cpi, base_qindex);
  if (rdmult < 1) rdmult = 1;
  set_error_per_bit(x, rdmult);
  av1_initialize_me_consts(cpi, x, base_qindex);

  tpl_frame->is_valid = 1;

  cm->base_qindex = base_qindex;
  av1_frame_init_quantizer(cpi);

  for (mi_row = 0; mi_row < cm->mi_rows; mi_row += mi_height) {
    // Motion estimation row boundary
    x->mv_limits.row_min = -((mi_row * MI_SIZE) + (17 - 2 * AOM_INTERP_EXTEND));
    x->mv_limits.row_max = (cm->mi_rows - mi_height - mi_row) * MI_SIZE +
                           (17 - 2 * AOM_INTERP_EXTEND);
    xd->mb_to_top_edge = -((mi_row * MI_SIZE) * 8);
    xd->mb_to_bottom_edge = ((cm->mi_rows - mi_height - mi_row) * MI_SIZE) * 8;
    for (mi_col = 0; mi_col < cm->mi_cols; mi_col += mi_width) {
      TplDepStats tpl_stats;

      // Motion estimation column boundary
      x->mv_limits.col_min =
          -((mi_col * MI_SIZE) + (17 - 2 * AOM_INTERP_EXTEND));
      x->mv_limits.col_max = ((cm->mi_cols - mi_width - mi_col) * MI_SIZE) +
                             (17 - 2 * AOM_INTERP_EXTEND);
      xd->mb_to_left_edge = -((mi_col * MI_SIZE) * 8);
      xd->mb_to_right_edge = ((cm->mi_cols - mi_width - mi_col) * MI_SIZE) * 8;
      mode_estimation(cpi, x, xd, &sf, frame_idx, src_diff, coeff, 1, mi_row,
                      mi_col, bsize, tx_size, ref_frame, predictor, &tpl_stats);

      // Motion flow dependency dispenser.
      tpl_model_store(tpl_frame->tpl_stats_ptr, mi_row, mi_col, bsize,
                      tpl_frame->stride, &tpl_stats);

      if (frame_idx)
        tpl_model_update(cpi->tpl_stats, tpl_frame->tpl_stats_ptr, mi_row,
                         mi_col, bsize);
    }
  }
}

#define REF_IDX(ref) ((ref)-LAST_FRAME)

static void init_gop_frames_for_tpl(AV1_COMP *cpi,
                                    YV12_BUFFER_CONFIG **gf_picture,
                                    GF_GROUP *gf_group, int *tpl_group_frames,
                                    const EncodeFrameInput *const frame_input) {
  AV1_COMMON *cm = &cpi->common;
  const SequenceHeader *const seq_params = &cm->seq_params;
  int frame_idx = 0;
  int frame_disp_idx = 0;
  RefCntBuffer *frame_bufs = cm->buffer_pool->frame_bufs;
  int pframe_qindex = 0;
  int cur_frame_idx = gf_group->index;

  for (int i = 0; i < FRAME_BUFFERS && frame_idx < INTER_REFS_PER_FRAME + 1;
       ++i) {
    if (frame_bufs[i].ref_count == 0) {
      alloc_frame_mvs(cm, &frame_bufs[i]);
      if (aom_realloc_frame_buffer(
              &frame_bufs[i].buf, cm->width, cm->height,
              seq_params->subsampling_x, seq_params->subsampling_y,
              seq_params->use_highbitdepth, cpi->oxcf.border_in_pixels,
              cm->byte_alignment, NULL, NULL, NULL))
        aom_internal_error(&cm->error, AOM_CODEC_MEM_ERROR,
                           "Failed to allocate frame buffer");
      ++frame_idx;
    }
  }

  *tpl_group_frames = 0;

  if (cur_frame_idx > 0) {
    // Initialize Golden reference frame.
    RefCntBuffer *ref_buf = get_ref_frame_buf(cm, GOLDEN_FRAME);
    gf_picture[0] = &ref_buf->buf;
    ++*tpl_group_frames;
  }

  if (cur_frame_idx > 1) {
    // Initialize Alt reference frame.
    RefCntBuffer *ref_buf = get_ref_frame_buf(cm, ALTREF_FRAME);
    gf_picture[1] = &ref_buf->buf;
    ++*tpl_group_frames;
  }

  // Initialize frames in the GF group
  for (frame_idx = cur_frame_idx;
       frame_idx <= AOMMIN(gf_group->size, MAX_LENGTH_TPL_FRAME_STATS - 1);
       ++frame_idx) {
    if (frame_idx == cur_frame_idx) {
      gf_picture[frame_idx] = frame_input->source;
      frame_disp_idx = gf_group->frame_disp_idx[frame_idx];
    } else {
      frame_disp_idx = frame_idx == gf_group->size
                           ? gf_group->frame_disp_idx[1]
                           : gf_group->frame_disp_idx[frame_idx];
      struct lookahead_entry *buf =
          av1_lookahead_peek(cpi->lookahead, frame_disp_idx - 1);

      if (buf == NULL) break;

      gf_picture[frame_idx] = &buf->img;
      if (frame_idx == gf_group->size) {
        gf_group->frame_disp_idx[frame_idx] = frame_disp_idx;
        gf_group->q_val[frame_idx] = gf_group->q_val[1];
        gf_group->update_type[frame_idx] = OVERLAY_UPDATE;
      }
    }

    if (gf_group->update_type[frame_idx] == LF_UPDATE)
      pframe_qindex = gf_group->q_val[frame_idx];

    ++*tpl_group_frames;
  }

  if (cur_frame_idx == 0) return;

  if (frame_idx < MAX_LENGTH_TPL_FRAME_STATS) {
    ++frame_disp_idx;
    int extend_frame_count = 0;
    const int gld_idx_next_gop = gf_group->size;
    const int lst_idx_next_gop =
        gf_group->ref_frame_gop_idx[gld_idx_next_gop][REF_IDX(LAST_FRAME)];
    const int lst2_idx_next_gop =
        gf_group->ref_frame_gop_idx[gld_idx_next_gop][REF_IDX(LAST2_FRAME)];
    const int lst3_idx_next_gop =
        gf_group->ref_frame_gop_idx[gld_idx_next_gop][REF_IDX(LAST3_FRAME)];

    // Extend two frames outside the current gf group.
    for (; frame_idx < MAX_LENGTH_TPL_FRAME_STATS && extend_frame_count < 2;
         ++frame_idx) {
      struct lookahead_entry *buf =
          av1_lookahead_peek(cpi->lookahead, frame_disp_idx - 1);

      if (buf == NULL) break;

      gf_picture[frame_idx] = &buf->img;
      gf_group->q_val[frame_idx] = pframe_qindex;
      gf_group->frame_disp_idx[frame_idx] = frame_disp_idx;
      gf_group->update_type[frame_idx] = LF_UPDATE;

      gf_group->ref_frame_gop_idx[frame_idx][REF_IDX(GOLDEN_FRAME)] =
          gld_idx_next_gop;
      gf_group->ref_frame_gop_idx[frame_idx][REF_IDX(LAST_FRAME)] =
          lst_idx_next_gop;
      gf_group->ref_frame_gop_idx[frame_idx][REF_IDX(LAST2_FRAME)] =
          lst2_idx_next_gop;
      gf_group->ref_frame_gop_idx[frame_idx][REF_IDX(LAST3_FRAME)] =
          lst3_idx_next_gop;

      ++*tpl_group_frames;
      ++extend_frame_count;
      ++frame_disp_idx;
    }
  }

  for (frame_idx = 0; frame_idx < *tpl_group_frames; ++frame_idx) {
    assert(gf_picture[frame_idx] == get_framebuf(cpi, frame_input, frame_idx));
  }
  /*
  for (frame_idx = 0; frame_idx < *tpl_group_frames; ++frame_idx) {
    printf("frame_idx:%d -> %d [ %d ]\n", frame_idx,
           gf_group->frame_disp_idx[frame_idx],
           gf_group->q_val[frame_idx]);
    for (int i = 0; i < INTER_REFS_PER_FRAME; ++i)
      printf("%d, ", gf_group->ref_frame_gop_idx[frame_idx][i]);
    printf(" -> ");
    for (int i = 0; i < INTER_REFS_PER_FRAME; ++i)
      printf(
          "%d, ",
          gf_group->frame_disp_idx[gf_group->ref_frame_gop_idx[frame_idx][i]]);
    printf(" -> ");
    for (int i = 0; i < INTER_REFS_PER_FRAME; ++i)
      printf("%d, ",
             gf_group->q_val[gf_group->ref_frame_gop_idx[frame_idx][i]]);
    printf("\n");
  }
  */
}

static void init_tpl_stats(AV1_COMP *cpi) {
  for (int frame_idx = 0; frame_idx < MAX_LENGTH_TPL_FRAME_STATS; ++frame_idx) {
    TplDepFrame *tpl_frame = &cpi->tpl_stats[frame_idx];
    memset(tpl_frame->tpl_stats_ptr, 0,
           tpl_frame->height * tpl_frame->width *
               sizeof(*tpl_frame->tpl_stats_ptr));
    tpl_frame->is_valid = 0;
  }
}

void av1_tpl_setup_stats(AV1_COMP *cpi,
                         const EncodeFrameInput *const frame_input) {
  YV12_BUFFER_CONFIG *gf_picture[MAX_LENGTH_TPL_FRAME_STATS];
  GF_GROUP *gf_group = &cpi->gf_group;

  init_gop_frames_for_tpl(cpi, gf_picture, gf_group, &cpi->tpl_gf_group_frames,
                          frame_input);

  init_tpl_stats(cpi);

  if (cpi->oxcf.enable_tpl_model == 1) {
    // Backward propagation from tpl_group_frames to 1.
    for (int frame_idx = cpi->tpl_gf_group_frames - 1;
         frame_idx >= gf_group->index; --frame_idx) {
      if (gf_group->update_type[frame_idx] == OVERLAY_UPDATE ||
          gf_group->update_type[frame_idx] == INTNL_OVERLAY_UPDATE)
        continue;
      mc_flow_dispenser(cpi, gf_picture, frame_idx);
    }
  }
}

static void get_tpl_forward_stats(AV1_COMP *cpi, MACROBLOCK *x, MACROBLOCKD *xd,
                                  BLOCK_SIZE bsize, int use_satd,
                                  YV12_BUFFER_CONFIG *ref,
                                  YV12_BUFFER_CONFIG *src,
                                  TplDepFrame *ref_tpl_frame) {
  AV1_COMMON *cm = &cpi->common;

  const int bw = 4 << mi_size_wide_log2[bsize];
  const int bh = 4 << mi_size_high_log2[bsize];
  const int mi_height = mi_size_high[bsize];
  const int mi_width = mi_size_wide[bsize];
  const int pix_num = bw * bh;
  const TX_SIZE tx_size = max_txsize_lookup[bsize];

  DECLARE_ALIGNED(32, uint16_t, predictor16[MC_FLOW_NUM_PELS * 3]);
  DECLARE_ALIGNED(32, uint8_t, predictor8[MC_FLOW_NUM_PELS * 3]);
  uint8_t *predictor;
  DECLARE_ALIGNED(32, int16_t, src_diff[MC_FLOW_NUM_PELS]);
  DECLARE_ALIGNED(32, tran_low_t, coeff[MC_FLOW_NUM_PELS]);

  if (is_cur_buf_hbd(xd))
    predictor = CONVERT_TO_BYTEPTR(predictor16);
  else
    predictor = predictor8;

  // Initialize advanced prediction parameters as default values
  struct scale_factors sf;
  av1_setup_scale_factors_for_frame(&sf, ref->y_crop_width, ref->y_crop_height,
                                    src->y_crop_width, src->y_crop_height);
  ConvolveParams conv_params = get_conv_params(0, 0, xd->bd);
  WarpTypesAllowed warp_types;
  memset(&warp_types, 0, sizeof(WarpTypesAllowed));
  const int_interpfilters kernel =
      av1_broadcast_interp_filter(EIGHTTAP_REGULAR);
  xd->above_mbmi = NULL;
  xd->left_mbmi = NULL;
  xd->mi[0]->sb_type = bsize;
  xd->mi[0]->motion_mode = SIMPLE_TRANSLATION;
  xd->block_ref_scale_factors[0] = &sf;

  for (int mi_row = 0; mi_row < cm->mi_rows; mi_row += mi_height) {
    // Motion estimation row boundary
    x->mv_limits.row_min = -((mi_row * MI_SIZE) + (17 - 2 * AOM_INTERP_EXTEND));
    x->mv_limits.row_max = (cm->mi_rows - mi_height - mi_row) * MI_SIZE +
                           (17 - 2 * AOM_INTERP_EXTEND);
    xd->mb_to_top_edge = -((mi_row * MI_SIZE) * 8);
    xd->mb_to_bottom_edge = ((cm->mi_rows - mi_height - mi_row) * MI_SIZE) * 8;

    for (int mi_col = 0; mi_col < cm->mi_cols; mi_col += mi_width) {
      int64_t inter_cost, intra_cost;
      x->mv_limits.col_min =
          -((mi_col * MI_SIZE) + (17 - 2 * AOM_INTERP_EXTEND));
      x->mv_limits.col_max = ((cm->mi_cols - mi_width - mi_col) * MI_SIZE) +
                             (17 - 2 * AOM_INTERP_EXTEND);
      xd->mb_to_left_edge = -((mi_col * MI_SIZE) * 8);
      xd->mb_to_right_edge = ((cm->mi_cols - mi_width - mi_col) * MI_SIZE) * 8;

      // Intra mode
      xd->mi[0]->ref_frame[0] = INTRA_FRAME;
      int64_t best_intra_cost = INT64_MAX;
      for (PREDICTION_MODE mode = DC_PRED; mode <= PAETH_PRED; ++mode) {
        uint8_t *src_buf =
            src->y_buffer + mi_row * MI_SIZE * src->y_stride + mi_col * MI_SIZE;
        const int src_stride = src->y_stride;

        uint8_t *dst_buf = &predictor[0];
        const int dst_stride = bw;

        av1_predict_intra_block(cm, xd, bw, bh, tx_size, mode, 0, 0,
                                FILTER_INTRA_MODES, src_buf, src_stride,
                                dst_buf, dst_stride, 0, 0, 0);

        if (use_satd) {
          if (is_cur_buf_hbd(xd)) {
            aom_highbd_subtract_block(bh, bw, src_diff, bw, src_buf, src_stride,
                                      dst_buf, dst_stride, xd->bd);
          } else {
            aom_subtract_block(bh, bw, src_diff, bw, src_buf, src_stride,
                               dst_buf, dst_stride);
          }
          wht_fwd_txfm(src_diff, bw, coeff, tx_size, is_cur_buf_hbd(xd));

          intra_cost = aom_satd(coeff, pix_num);
        } else {
          int64_t sse;
          if (is_cur_buf_hbd(xd)) {
            sse = aom_highbd_sse(src_buf, src_stride, dst_buf, dst_stride, bw,
                                 bh);
          } else {
            sse = aom_sse(src_buf, src_stride, dst_buf, dst_stride, bw, bh);
          }
          intra_cost = ROUND_POWER_OF_TWO(sse, (xd->bd - 8) * 2);
        }
        if (intra_cost < best_intra_cost) best_intra_cost = intra_cost;
      }

      // Inter mode
      // Motion estimation column boundary
      xd->mi[0]->ref_frame[0] = GOLDEN_FRAME;

      const int mb_y_offset =
          mi_row * MI_SIZE * src->y_stride + mi_col * MI_SIZE;
      const int mb_y_offset_ref =
          mi_row * MI_SIZE * ref->y_stride + mi_col * MI_SIZE;
      motion_estimation(cpi, x, src->y_buffer + mb_y_offset,
                        ref->y_buffer + mb_y_offset_ref, src->y_stride,
                        ref->y_stride, bsize, mi_row, mi_col);

      av1_build_inter_predictor(
          ref->y_buffer + mb_y_offset_ref, ref->y_stride, &predictor[0], bw,
          &x->best_mv.as_mv, &sf, bw, bh, &conv_params, kernel, &warp_types,
          mi_col * MI_SIZE, mi_row * MI_SIZE, 0, 0, MV_PRECISION_Q3,
          mi_col * MI_SIZE, mi_row * MI_SIZE, xd, 0);
      if (use_satd) {
        if (is_cur_buf_hbd(xd)) {
          aom_highbd_subtract_block(bh, bw, src_diff, bw,
                                    src->y_buffer + mb_y_offset, src->y_stride,
                                    &predictor[0], bw, xd->bd);
        } else {
          aom_subtract_block(bh, bw, src_diff, bw, src->y_buffer + mb_y_offset,
                             src->y_stride, &predictor[0], bw);
        }
        wht_fwd_txfm(src_diff, bw, coeff, tx_size, is_cur_buf_hbd(xd));
        inter_cost = aom_satd(coeff, pix_num);
      } else {
        int64_t sse;
        if (is_cur_buf_hbd(xd)) {
          sse = aom_highbd_sse(src->y_buffer + mb_y_offset, src->y_stride,
                               &predictor[0], bw, bw, bh);
        } else {
          sse = aom_sse(src->y_buffer + mb_y_offset, src->y_stride,
                        &predictor[0], bw, bw, bh);
        }
        inter_cost = ROUND_POWER_OF_TWO(sse, (xd->bd - 8) * 2);
      }

      // Finalize stats
      best_intra_cost = AOMMAX(best_intra_cost, 1);
      inter_cost = AOMMIN(best_intra_cost, inter_cost);

      // Project stats to reference block
      TplDepStats *ref_stats_ptr = ref_tpl_frame->tpl_stats_ptr;
      const MV mv = x->best_mv.as_mv;
      const int mv_row = mv.row >> 3;
      const int mv_col = mv.col >> 3;
      const int ref_pos_row = mi_row * MI_SIZE + mv_row;
      const int ref_pos_col = mi_col * MI_SIZE + mv_col;
      const int grid_pos_row_base = round_floor(ref_pos_row, bh) * bh;
      const int grid_pos_col_base = round_floor(ref_pos_col, bw) * bw;

      for (int block = 0; block < 4; ++block) {
        const int grid_pos_row = grid_pos_row_base + bh * (block >> 1);
        const int grid_pos_col = grid_pos_col_base + bw * (block & 0x01);

        if (grid_pos_row >= 0 &&
            grid_pos_row < ref_tpl_frame->mi_rows * MI_SIZE &&
            grid_pos_col >= 0 &&
            grid_pos_col < ref_tpl_frame->mi_cols * MI_SIZE) {
          const int overlap_area =
              get_overlap_area(grid_pos_row, grid_pos_col, ref_pos_row,
                               ref_pos_col, block, bsize);
          const int ref_mi_row = round_floor(grid_pos_row, bh) * mi_height;
          const int ref_mi_col = round_floor(grid_pos_col, bw) * mi_width;

          const int64_t mc_saved = (best_intra_cost - inter_cost)
                                   << TPL_DEP_COST_SCALE_LOG2;
          for (int idy = 0; idy < mi_height; ++idy) {
            for (int idx = 0; idx < mi_width; ++idx) {
              TplDepStats *des_stats =
                  &ref_stats_ptr[(ref_mi_row + idy) * ref_tpl_frame->stride +
                                 (ref_mi_col + idx)];
              des_stats->mc_count += overlap_area << TPL_DEP_COST_SCALE_LOG2;
              des_stats->mc_saved += (mc_saved * overlap_area) / pix_num;
              assert(overlap_area >= 0);
            }
          }
        }
      }
    }
  }
}

void av1_tpl_setup_forward_stats(AV1_COMP *cpi) {
  ThreadData *td = &cpi->td;
  MACROBLOCK *x = &td->mb;
  MACROBLOCKD *xd = &x->e_mbd;
#if MC_FLOW_BSIZE == 64
  const BLOCK_SIZE bsize = BLOCK_64X64;
#elif MC_FLOW_BSIZE == 32
  const BLOCK_SIZE bsize = BLOCK_32X32;
#elif MC_FLOW_BSIZE == 16
  const BLOCK_SIZE bsize = BLOCK_16X16;
#elif MC_FLOW_BSIZE == 8
  const BLOCK_SIZE bsize = BLOCK_8X8;
#elif MC_FLOW_BSIZE == 4
  const BLOCK_SIZE bsize = BLOCK_4X4;
#else
#error "Invalid block size for tpl model"
#endif  // MC_FLOW_BSIZE == 64

  const GF_GROUP *gf_group = &cpi->gf_group;
  assert(IMPLIES(gf_group->size > 0, gf_group->index < gf_group->size));
  const int tpl_cur_idx = gf_group->frame_disp_idx[gf_group->index];
  TplDepFrame *tpl_frame = &cpi->tpl_stats[tpl_cur_idx];
  memset(
      tpl_frame->tpl_stats_ptr, 0,
      tpl_frame->height * tpl_frame->width * sizeof(*tpl_frame->tpl_stats_ptr));
  tpl_frame->is_valid = 0;
  int tpl_used_mask[MAX_LENGTH_TPL_FRAME_STATS] = { 0 };
  for (int idx = gf_group->index + 1; idx < cpi->tpl_gf_group_frames; ++idx) {
    const int tpl_future_idx = gf_group->frame_disp_idx[idx];

    if (gf_group->update_type[idx] == OVERLAY_UPDATE ||
        gf_group->update_type[idx] == INTNL_OVERLAY_UPDATE)
      continue;
    if (tpl_future_idx == tpl_cur_idx) continue;
    if (tpl_used_mask[tpl_future_idx]) continue;

    for (int ridx = 0; ridx < INTER_REFS_PER_FRAME; ++ridx) {
      const int ref_idx = gf_group->ref_frame_gop_idx[idx][ridx];
      const int tpl_ref_idx = gf_group->frame_disp_idx[ref_idx];
      if (tpl_ref_idx == tpl_cur_idx) {
        // Do tpl stats computation between current buffer and the one at
        // gf_group index given by idx (and with disp index given by
        // tpl_future_idx).
        assert(idx >= 2);
        YV12_BUFFER_CONFIG *cur_buf = &cpi->common.cur_frame->buf;
        YV12_BUFFER_CONFIG *future_buf = get_framebuf(cpi, NULL, idx);
        get_tpl_forward_stats(cpi, x, xd, bsize, 0, cur_buf, future_buf,
                              tpl_frame);
        tpl_frame->is_valid = 1;
        tpl_used_mask[tpl_future_idx] = 1;
      }
    }
  }
}
