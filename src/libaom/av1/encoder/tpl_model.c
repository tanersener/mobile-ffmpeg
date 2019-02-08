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

#include "config/aom_config.h"
#include "config/aom_dsp_rtcd.h"

#include "aom/aom_codec.h"

#include "av1/common/onyxc_int.h"
#include "av1/common/reconintra.h"

#include "av1/encoder/encoder.h"
#include "av1/encoder/reconinter_enc.h"

typedef struct GF_PICTURE {
  YV12_BUFFER_CONFIG *frame;
  int ref_frame[7];
} GF_PICTURE;

static void get_quantize_error(MACROBLOCK *x, int plane, tran_low_t *coeff,
                               tran_low_t *qcoeff, tran_low_t *dqcoeff,
                               TX_SIZE tx_size, int64_t *recon_error,
                               int64_t *sse) {
  const struct macroblock_plane *const p = &x->plane[plane];
  const SCAN_ORDER *const scan_order = &av1_default_scan_orders[tx_size];
  uint16_t eob;
  int pix_num = 1 << num_pels_log2_lookup[txsize_to_bsize[tx_size]];
  const int shift = tx_size == TX_32X32 ? 0 : 2;

  av1_quantize_fp_32x32(coeff, pix_num, p->zbin_QTX, p->round_fp_QTX,
                        p->quant_fp_QTX, p->quant_shift_QTX, qcoeff, dqcoeff,
                        p->dequant_QTX, &eob, scan_order->scan,
                        scan_order->iscan);

  *recon_error = av1_block_error(coeff, dqcoeff, pix_num, sse) >> shift;
  *recon_error = AOMMAX(*recon_error, 1);

  *sse = (*sse) >> shift;
  *sse = AOMMAX(*sse, 1);
}

static void wht_fwd_txfm(int16_t *src_diff, int bw, tran_low_t *coeff,
                         TX_SIZE tx_size) {
  switch (tx_size) {
    case TX_8X8: aom_hadamard_8x8(src_diff, bw, coeff); break;
    case TX_16X16: aom_hadamard_16x16(src_diff, bw, coeff); break;
    case TX_32X32: aom_hadamard_32x32(src_diff, bw, coeff); break;
    default: assert(0);
  }
}

static uint32_t motion_compensated_prediction(AV1_COMP *cpi, ThreadData *td,
                                              uint8_t *cur_frame_buf,
                                              uint8_t *ref_frame_buf,
                                              int stride, BLOCK_SIZE bsize,
                                              int mi_row, int mi_col) {
  AV1_COMMON *cm = &cpi->common;
  MACROBLOCK *const x = &td->mb;
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

  MV best_ref_mv1 = { 0, 0 };
  MV best_ref_mv1_full; /* full-pixel value of best_ref_mv1 */

  best_ref_mv1_full.col = best_ref_mv1.col >> 3;
  best_ref_mv1_full.row = best_ref_mv1.row >> 3;

  // Setup frame pointers
  x->plane[0].src.buf = cur_frame_buf;
  x->plane[0].src.stride = stride;
  xd->plane[0].pre[0].buf = ref_frame_buf;
  xd->plane[0].pre[0].stride = stride;

  step_param = mv_sf->reduce_first_step_size;
  step_param = AOMMIN(step_param, MAX_MVSEARCH_STEPS - 2);

  av1_set_mv_search_range(&x->mv_limits, &best_ref_mv1);

  av1_full_pixel_search(cpi, x, bsize, &best_ref_mv1_full, step_param,
                        search_method, 0, sadpb, cond_cost_list(cpi, cost_list),
                        &best_ref_mv1, INT_MAX, 0, (MI_SIZE * mi_col),
                        (MI_SIZE * mi_row), 0);

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
                            struct scale_factors *sf, GF_PICTURE *gf_picture,
                            int frame_idx, int16_t *src_diff, tran_low_t *coeff,
                            tran_low_t *qcoeff, tran_low_t *dqcoeff, int mi_row,
                            int mi_col, BLOCK_SIZE bsize, TX_SIZE tx_size,
                            YV12_BUFFER_CONFIG *ref_frame[], uint8_t *predictor,
                            int64_t *recon_error, int64_t *sse,
                            TplDepStats *tpl_stats) {
  AV1_COMMON *cm = &cpi->common;
  ThreadData *td = &cpi->td;

  const int bw = 4 << mi_size_wide_log2[bsize];
  const int bh = 4 << mi_size_high_log2[bsize];
  const int pix_num = bw * bh;
  int best_rf_idx = -1;
  int_mv best_mv;
  int64_t best_inter_cost = INT64_MAX;
  int64_t inter_cost;
  int rf_idx;
  const InterpFilters kernel =
      av1_make_interp_filters(EIGHTTAP_REGULAR, EIGHTTAP_REGULAR);

  int64_t best_intra_cost = INT64_MAX;
  int64_t intra_cost;
  PREDICTION_MODE mode;
  int mb_y_offset = mi_row * MI_SIZE * xd->cur_buf->y_stride + mi_col * MI_SIZE;
  MB_MODE_INFO mi_above, mi_left;

  memset(tpl_stats, 0, sizeof(*tpl_stats));

  xd->mb_to_top_edge = -((mi_row * MI_SIZE) * 8);
  xd->mb_to_bottom_edge = ((cm->mi_rows - 1 - mi_row) * MI_SIZE) * 8;
  xd->mb_to_left_edge = -((mi_col * MI_SIZE) * 8);
  xd->mb_to_right_edge = ((cm->mi_cols - 1 - mi_col) * MI_SIZE) * 8;
  xd->above_mbmi = (mi_row > 0) ? &mi_above : NULL;
  xd->left_mbmi = (mi_col > 0) ? &mi_left : NULL;

  // Intra prediction search
  for (mode = DC_PRED; mode <= PAETH_PRED; ++mode) {
    uint8_t *src, *dst;
    int src_stride, dst_stride;

    src = xd->cur_buf->y_buffer + mb_y_offset;
    src_stride = xd->cur_buf->y_stride;

    dst = &predictor[0];
    dst_stride = bw;

    xd->mi[0]->sb_type = bsize;
    xd->mi[0]->ref_frame[0] = INTRA_FRAME;

    av1_predict_intra_block(
        cm, xd, block_size_wide[bsize], block_size_high[bsize], tx_size, mode,
        0, 0, FILTER_INTRA_MODES, src, src_stride, dst, dst_stride, 0, 0, 0);

    if (is_cur_buf_hbd(xd)) {
      aom_highbd_subtract_block(bh, bw, src_diff, bw, src, src_stride, dst,
                                dst_stride, xd->bd);
    } else {
      aom_subtract_block(bh, bw, src_diff, bw, src, src_stride, dst,
                         dst_stride);
    }

    wht_fwd_txfm(src_diff, bw, coeff, tx_size);

    intra_cost = aom_satd(coeff, pix_num);

    if (intra_cost < best_intra_cost) best_intra_cost = intra_cost;
  }

  // Motion compensated prediction
  best_mv.as_int = 0;

  (void)mb_y_offset;
  // Motion estimation column boundary
  x->mv_limits.col_min = -((mi_col * MI_SIZE) + (17 - 2 * AOM_INTERP_EXTEND));
  x->mv_limits.col_max =
      ((cm->mi_cols - 1 - mi_col) * MI_SIZE) + (17 - 2 * AOM_INTERP_EXTEND);

  for (rf_idx = 0; rf_idx < 7; ++rf_idx) {
    if (ref_frame[rf_idx] == NULL) continue;

    motion_compensated_prediction(cpi, td, xd->cur_buf->y_buffer + mb_y_offset,
                                  ref_frame[rf_idx]->y_buffer + mb_y_offset,
                                  xd->cur_buf->y_stride, bsize, mi_row, mi_col);

    // TODO(jingning): Not yet support high bit-depth in the next three
    // steps.
    ConvolveParams conv_params = get_conv_params(0, 0, xd->bd);
    WarpTypesAllowed warp_types;
    memset(&warp_types, 0, sizeof(WarpTypesAllowed));

    av1_build_inter_predictor(
        ref_frame[rf_idx]->y_buffer + mb_y_offset, ref_frame[rf_idx]->y_stride,
        &predictor[0], bw, &x->best_mv.as_mv, sf, bw, bh, &conv_params, kernel,
        &warp_types, mi_col * MI_SIZE, mi_row * MI_SIZE, 0, 0, MV_PRECISION_Q3,
        mi_col * MI_SIZE, mi_row * MI_SIZE, xd, 0);
    if (is_cur_buf_hbd(xd)) {
      aom_highbd_subtract_block(
          bh, bw, src_diff, bw, xd->cur_buf->y_buffer + mb_y_offset,
          xd->cur_buf->y_stride, &predictor[0], bw, xd->bd);
    } else {
      aom_subtract_block(bh, bw, src_diff, bw,
                         xd->cur_buf->y_buffer + mb_y_offset,
                         xd->cur_buf->y_stride, &predictor[0], bw);
    }
    wht_fwd_txfm(src_diff, bw, coeff, tx_size);

    inter_cost = aom_satd(coeff, pix_num);
    if (inter_cost < best_inter_cost) {
      best_rf_idx = rf_idx;
      best_inter_cost = inter_cost;
      best_mv.as_int = x->best_mv.as_int;
      get_quantize_error(x, 0, coeff, qcoeff, dqcoeff, tx_size, recon_error,
                         sse);
    }
  }
  best_intra_cost = AOMMAX(best_intra_cost, 1);
  best_inter_cost = AOMMIN(best_intra_cost, best_inter_cost);
  tpl_stats->inter_cost = best_inter_cost << TPL_DEP_COST_SCALE_LOG2;
  tpl_stats->intra_cost = best_intra_cost << TPL_DEP_COST_SCALE_LOG2;
  tpl_stats->mc_dep_cost = tpl_stats->intra_cost + tpl_stats->mc_flow;

  tpl_stats->ref_frame_index = gf_picture[frame_idx].ref_frame[best_rf_idx];
  tpl_stats->mv.as_int = best_mv.as_int;
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

static void tpl_model_update_b(TplDepFrame *tpl_frame, TplDepStats *tpl_stats,
                               int mi_row, int mi_col, const BLOCK_SIZE bsize) {
  TplDepFrame *ref_tpl_frame = &tpl_frame[tpl_stats->ref_frame_index];
  TplDepStats *ref_stats = ref_tpl_frame->tpl_stats_ptr;
  MV mv = tpl_stats->mv.as_mv;
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

      int64_t mc_flow = tpl_stats->mc_dep_cost -
                        (tpl_stats->mc_dep_cost * tpl_stats->inter_cost) /
                            tpl_stats->intra_cost;

      int idx, idy;

      for (idy = 0; idy < mi_height; ++idy) {
        for (idx = 0; idx < mi_width; ++idx) {
          TplDepStats *des_stats =
              &ref_stats[(ref_mi_row + idy) * ref_tpl_frame->stride +
                         (ref_mi_col + idx)];

          des_stats->mc_flow += (mc_flow * overlap_area) / pix_num;
          des_stats->mc_ref_cost +=
              ((tpl_stats->intra_cost - tpl_stats->inter_cost) * overlap_area) /
              pix_num;
          assert(overlap_area >= 0);
        }
      }
    }
  }
}

static void tpl_model_update(TplDepFrame *tpl_frame, TplDepStats *tpl_stats,
                             int mi_row, int mi_col, const BLOCK_SIZE bsize) {
  int idx, idy;
  const int mi_height = mi_size_high[bsize];
  const int mi_width = mi_size_wide[bsize];

  for (idy = 0; idy < mi_height; ++idy) {
    for (idx = 0; idx < mi_width; ++idx) {
      TplDepStats *tpl_ptr =
          &tpl_stats[(mi_row + idy) * tpl_frame->stride + (mi_col + idx)];
      tpl_model_update_b(tpl_frame, tpl_ptr, mi_row + idy, mi_col + idx,
                         BLOCK_4X4);
    }
  }
}

static void tpl_model_store(TplDepStats *tpl_stats, int mi_row, int mi_col,
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
    tpl_ptr = &tpl_stats[(mi_row + idy) * stride + mi_col];
    for (idx = 0; idx < mi_width; ++idx) {
      tpl_ptr->intra_cost = intra_cost;
      tpl_ptr->inter_cost = inter_cost;
      tpl_ptr->mc_dep_cost = tpl_ptr->intra_cost + tpl_ptr->mc_flow;
      tpl_ptr->ref_frame_index = src_stats->ref_frame_index;
      tpl_ptr->mv.as_int = src_stats->mv.as_int;
      ++tpl_ptr;
    }
  }
}

static void mc_flow_dispenser(AV1_COMP *cpi, GF_PICTURE *gf_picture,
                              int frame_idx) {
  TplDepFrame *tpl_frame = &cpi->tpl_stats[frame_idx];
  YV12_BUFFER_CONFIG *this_frame = gf_picture[frame_idx].frame;
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

  DECLARE_ALIGNED(16, uint16_t, predictor16[32 * 32 * 3]);
  DECLARE_ALIGNED(16, uint8_t, predictor8[32 * 32 * 3]);
  uint8_t *predictor;
  DECLARE_ALIGNED(16, int16_t, src_diff[32 * 32]);
  DECLARE_ALIGNED(16, tran_low_t, coeff[32 * 32]);
  DECLARE_ALIGNED(16, tran_low_t, qcoeff[32 * 32]);
  DECLARE_ALIGNED(16, tran_low_t, dqcoeff[32 * 32]);

  const BLOCK_SIZE bsize = BLOCK_32X32;
  const TX_SIZE tx_size = max_txsize_lookup[bsize];
  const int mi_height = mi_size_high[bsize];
  const int mi_width = mi_size_wide[bsize];
  int64_t recon_error, sse;

  // Setup scaling factor
  av1_setup_scale_factors_for_frame(
      &sf, this_frame->y_crop_width, this_frame->y_crop_height,
      this_frame->y_crop_width, this_frame->y_crop_height);

  if (is_cur_buf_hbd(xd))
    predictor = CONVERT_TO_BYTEPTR(predictor16);
  else
    predictor = predictor8;

  // Prepare reference frame pointers. If any reference frame slot is
  // unavailable, the pointer will be set to Null.
  for (idx = 0; idx < 7; ++idx) {
    int rf_idx = gf_picture[frame_idx].ref_frame[idx];
    if (rf_idx != -1) ref_frame[idx] = gf_picture[rf_idx].frame;
  }

  xd->mi = cm->mi_grid_visible;
  xd->mi[0] = cm->mi;
  xd->cur_buf = this_frame;

  // Get rd multiplier set up.
  rdmult = (int)av1_compute_rd_mult(cpi, tpl_frame->base_qindex);
  if (rdmult < 1) rdmult = 1;
  set_error_per_bit(&cpi->td.mb, rdmult);
  av1_initialize_me_consts(cpi, &cpi->td.mb, tpl_frame->base_qindex);

  tpl_frame->is_valid = 1;

  cm->base_qindex = tpl_frame->base_qindex;
  av1_frame_init_quantizer(cpi);

  for (mi_row = 0; mi_row < cm->mi_rows; mi_row += mi_height) {
    // Motion estimation row boundary
    x->mv_limits.row_min = -((mi_row * MI_SIZE) + (17 - 2 * AOM_INTERP_EXTEND));
    x->mv_limits.row_max =
        (cm->mi_rows - 1 - mi_row) * MI_SIZE + (17 - 2 * AOM_INTERP_EXTEND);
    for (mi_col = 0; mi_col < cm->mi_cols; mi_col += mi_width) {
      TplDepStats tpl_stats;
      mode_estimation(cpi, x, xd, &sf, gf_picture, frame_idx, src_diff, coeff,
                      qcoeff, dqcoeff, mi_row, mi_col, bsize, tx_size,
                      ref_frame, predictor, &recon_error, &sse, &tpl_stats);

      // Motion flow dependency dispenser.
      tpl_model_store(tpl_frame->tpl_stats_ptr, mi_row, mi_col, bsize,
                      tpl_frame->stride, &tpl_stats);

      tpl_model_update(cpi->tpl_stats, tpl_frame->tpl_stats_ptr, mi_row, mi_col,
                       bsize);
    }
  }
}

static void init_gop_frames(AV1_COMP *cpi, GF_PICTURE *gf_picture,
                            const GF_GROUP *gf_group, int *tpl_group_frames,
                            const EncodeFrameInput *const frame_input) {
  AV1_COMMON *cm = &cpi->common;
  const SequenceHeader *const seq_params = &cm->seq_params;
  int frame_idx = 0;
  int i;
  int gld_index = -1;
  int alt_index = -1;
  int lst_index = -1;
  int extend_frame_count = 0;
  int pframe_qindex = cpi->tpl_stats[2].base_qindex;

  RefCntBuffer *frame_bufs = cm->buffer_pool->frame_bufs;
  int recon_frame_index[INTER_REFS_PER_FRAME + 1] = { -1, -1, -1, -1,
                                                      -1, -1, -1, -1 };

  // TODO(jingning): To be used later for gf frame type parsing.
  (void)gf_group;

  for (i = 0; i < FRAME_BUFFERS && frame_idx < INTER_REFS_PER_FRAME + 1; ++i) {
    if (frame_bufs[i].ref_count == 0) {
      alloc_frame_mvs(cm, &frame_bufs[i]);
      if (aom_realloc_frame_buffer(
              &frame_bufs[i].buf, cm->width, cm->height,
              seq_params->subsampling_x, seq_params->subsampling_y,
              seq_params->use_highbitdepth, cpi->oxcf.border_in_pixels,
              cm->byte_alignment, NULL, NULL, NULL))
        aom_internal_error(&cm->error, AOM_CODEC_MEM_ERROR,
                           "Failed to allocate frame buffer");

      recon_frame_index[frame_idx] = i;
      ++frame_idx;
    }
  }

  for (i = 0; i < INTER_REFS_PER_FRAME + 1; ++i) {
    assert(recon_frame_index[i] >= 0);
    cpi->tpl_recon_frames[i] = &frame_bufs[recon_frame_index[i]].buf;
  }

  *tpl_group_frames = 0;

  // Initialize Golden reference frame.
  gf_picture[0].frame = NULL;
  RefCntBuffer *ref_buf = get_ref_frame_buf(cm, GOLDEN_FRAME);
  if (ref_buf) gf_picture[0].frame = &ref_buf->buf;
  for (i = 0; i < 7; ++i) gf_picture[0].ref_frame[i] = -1;
  gld_index = 0;
  ++*tpl_group_frames;

  // Initialize ARF frame
  gf_picture[1].frame = frame_input->source;
  gf_picture[1].ref_frame[0] = gld_index;
  gf_picture[1].ref_frame[1] = lst_index;
  gf_picture[1].ref_frame[2] = alt_index;
  // TODO(yuec) Need o  figure out full AV1 reference model
  for (i = 3; i < 7; ++i) gf_picture[1].ref_frame[i] = -1;
  alt_index = 1;
  ++*tpl_group_frames;

  // Initialize P frames
  for (frame_idx = 2; frame_idx < MAX_LAG_BUFFERS; ++frame_idx) {
    struct lookahead_entry *buf =
        av1_lookahead_peek(cpi->lookahead, frame_idx - 2);

    if (buf == NULL) break;

    gf_picture[frame_idx].frame = &buf->img;
    gf_picture[frame_idx].ref_frame[0] = gld_index;
    gf_picture[frame_idx].ref_frame[1] = lst_index;
    gf_picture[frame_idx].ref_frame[2] = alt_index;
    for (i = 3; i < 7; ++i) gf_picture[frame_idx].ref_frame[i] = -1;

    ++*tpl_group_frames;
    lst_index = frame_idx;

    if (frame_idx == cpi->rc.baseline_gf_interval + 1) break;
  }

  gld_index = frame_idx;
  lst_index = AOMMAX(0, frame_idx - 1);
  alt_index = -1;
  ++frame_idx;

  // Extend two frames outside the current gf group.
  for (; frame_idx < MAX_LAG_BUFFERS && extend_frame_count < 2; ++frame_idx) {
    struct lookahead_entry *buf =
        av1_lookahead_peek(cpi->lookahead, frame_idx - 2);

    if (buf == NULL) break;

    cpi->tpl_stats[frame_idx].base_qindex = pframe_qindex;

    gf_picture[frame_idx].frame = &buf->img;
    gf_picture[frame_idx].ref_frame[0] = gld_index;
    gf_picture[frame_idx].ref_frame[1] = lst_index;
    gf_picture[frame_idx].ref_frame[2] = alt_index;
    for (i = 3; i < 7; ++i) gf_picture[frame_idx].ref_frame[i] = -1;
    lst_index = frame_idx;
    ++*tpl_group_frames;
    ++extend_frame_count;
  }
}

static void init_tpl_stats(AV1_COMP *cpi) {
  int frame_idx;
  for (frame_idx = 0; frame_idx < MAX_LAG_BUFFERS; ++frame_idx) {
    TplDepFrame *tpl_frame = &cpi->tpl_stats[frame_idx];
    memset(tpl_frame->tpl_stats_ptr, 0,
           tpl_frame->height * tpl_frame->width *
               sizeof(*tpl_frame->tpl_stats_ptr));
    tpl_frame->is_valid = 0;
  }
}

void av1_tpl_setup_stats(AV1_COMP *cpi,
                         const EncodeFrameInput *const frame_input) {
  GF_PICTURE gf_picture[MAX_LAG_BUFFERS];
  const GF_GROUP *gf_group = &cpi->twopass.gf_group;
  int tpl_group_frames = 0;
  int frame_idx;

  init_gop_frames(cpi, gf_picture, gf_group, &tpl_group_frames, frame_input);

  init_tpl_stats(cpi);

  // Backward propagation from tpl_group_frames to 1.
  for (frame_idx = tpl_group_frames - 1; frame_idx > 0; --frame_idx)
    mc_flow_dispenser(cpi, gf_picture, frame_idx);
}
