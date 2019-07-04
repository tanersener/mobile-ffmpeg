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

#include <limits.h>
#include <math.h>
#include <stdio.h>

#include "config/aom_dsp_rtcd.h"
#include "config/aom_scale_rtcd.h"

#include "aom_dsp/aom_dsp_common.h"
#include "aom_mem/aom_mem.h"
#include "aom_ports/mem.h"
#include "aom_ports/system_state.h"
#include "aom_scale/aom_scale.h"
#include "aom_scale/yv12config.h"

#include "aom_dsp/variance.h"
#include "av1/common/entropymv.h"
#include "av1/common/quant_common.h"
#include "av1/common/reconinter.h"  // av1_setup_dst_planes()
#include "av1/common/txb_common.h"
#include "av1/encoder/aq_variance.h"
#include "av1/encoder/av1_quantize.h"
#include "av1/encoder/block.h"
#include "av1/encoder/dwt.h"
#include "av1/encoder/encodeframe.h"
#include "av1/encoder/encodemb.h"
#include "av1/encoder/encodemv.h"
#include "av1/encoder/encoder.h"
#include "av1/encoder/encode_strategy.h"
#include "av1/encoder/extend.h"
#include "av1/encoder/firstpass.h"
#include "av1/encoder/mcomp.h"
#include "av1/encoder/rd.h"
#include "av1/encoder/reconinter_enc.h"

#define OUTPUT_FPF 0

#define FIRST_PASS_Q 10.0
#define INTRA_MODE_PENALTY 1024
#define NEW_MV_MODE_PENALTY 32
#define DARK_THRESH 64

#define NCOUNT_INTRA_THRESH 8192
#define NCOUNT_INTRA_FACTOR 3

static void output_stats(FIRSTPASS_STATS *stats,
                         struct aom_codec_pkt_list *pktlist) {
  struct aom_codec_cx_pkt pkt;
  pkt.kind = AOM_CODEC_STATS_PKT;
  pkt.data.twopass_stats.buf = stats;
  pkt.data.twopass_stats.sz = sizeof(FIRSTPASS_STATS);
  aom_codec_pkt_list_add(pktlist, &pkt);

// TEMP debug code
#if OUTPUT_FPF
  {
    FILE *fpfile;
    fpfile = fopen("firstpass.stt", "a");

    fprintf(fpfile,
            "%12.0lf %12.4lf %12.0lf %12.0lf %12.0lf %12.4lf %12.4lf"
            "%12.4lf %12.4lf %12.4lf %12.4lf %12.4lf %12.4lf %12.4lf %12.4lf"
            "%12.4lf %12.4lf %12.0lf %12.0lf %12.0lf %12.4lf %12.4lf\n",
            stats->frame, stats->weight, stats->intra_error, stats->coded_error,
            stats->sr_coded_error, stats->pcnt_inter, stats->pcnt_motion,
            stats->pcnt_second_ref, stats->pcnt_neutral, stats->intra_skip_pct,
            stats->inactive_zone_rows, stats->inactive_zone_cols, stats->MVr,
            stats->mvr_abs, stats->MVc, stats->mvc_abs, stats->MVrv,
            stats->MVcv, stats->mv_in_out_count, stats->new_mv_count,
            stats->count, stats->duration);
    fclose(fpfile);
  }
#endif
}

void av1_twopass_zero_stats(FIRSTPASS_STATS *section) {
  section->frame = 0.0;
  section->weight = 0.0;
  section->intra_error = 0.0;
  section->frame_avg_wavelet_energy = 0.0;
  section->coded_error = 0.0;
  section->sr_coded_error = 0.0;
  section->pcnt_inter = 0.0;
  section->pcnt_motion = 0.0;
  section->pcnt_second_ref = 0.0;
  section->pcnt_neutral = 0.0;
  section->intra_skip_pct = 0.0;
  section->inactive_zone_rows = 0.0;
  section->inactive_zone_cols = 0.0;
  section->MVr = 0.0;
  section->mvr_abs = 0.0;
  section->MVc = 0.0;
  section->mvc_abs = 0.0;
  section->MVrv = 0.0;
  section->MVcv = 0.0;
  section->mv_in_out_count = 0.0;
  section->new_mv_count = 0.0;
  section->count = 0.0;
  section->duration = 1.0;
}

static void accumulate_stats(FIRSTPASS_STATS *section,
                             const FIRSTPASS_STATS *frame) {
  section->frame += frame->frame;
  section->weight += frame->weight;
  section->intra_error += frame->intra_error;
  section->frame_avg_wavelet_energy += frame->frame_avg_wavelet_energy;
  section->coded_error += frame->coded_error;
  section->sr_coded_error += frame->sr_coded_error;
  section->pcnt_inter += frame->pcnt_inter;
  section->pcnt_motion += frame->pcnt_motion;
  section->pcnt_second_ref += frame->pcnt_second_ref;
  section->pcnt_neutral += frame->pcnt_neutral;
  section->intra_skip_pct += frame->intra_skip_pct;
  section->inactive_zone_rows += frame->inactive_zone_rows;
  section->inactive_zone_cols += frame->inactive_zone_cols;
  section->MVr += frame->MVr;
  section->mvr_abs += frame->mvr_abs;
  section->MVc += frame->MVc;
  section->mvc_abs += frame->mvc_abs;
  section->MVrv += frame->MVrv;
  section->MVcv += frame->MVcv;
  section->mv_in_out_count += frame->mv_in_out_count;
  section->new_mv_count += frame->new_mv_count;
  section->count += frame->count;
  section->duration += frame->duration;
}

void av1_init_first_pass(AV1_COMP *cpi) {
  av1_twopass_zero_stats(&cpi->twopass.total_stats);
}

void av1_end_first_pass(AV1_COMP *cpi) {
  output_stats(&cpi->twopass.total_stats, cpi->output_pkt_list);
}

static aom_variance_fn_t get_block_variance_fn(BLOCK_SIZE bsize) {
  switch (bsize) {
    case BLOCK_8X8: return aom_mse8x8;
    case BLOCK_16X8: return aom_mse16x8;
    case BLOCK_8X16: return aom_mse8x16;
    default: return aom_mse16x16;
  }
}

static unsigned int get_prediction_error(BLOCK_SIZE bsize,
                                         const struct buf_2d *src,
                                         const struct buf_2d *ref) {
  unsigned int sse;
  const aom_variance_fn_t fn = get_block_variance_fn(bsize);
  fn(src->buf, src->stride, ref->buf, ref->stride, &sse);
  return sse;
}

static aom_variance_fn_t highbd_get_block_variance_fn(BLOCK_SIZE bsize,
                                                      int bd) {
  switch (bd) {
    default:
      switch (bsize) {
        case BLOCK_8X8: return aom_highbd_8_mse8x8;
        case BLOCK_16X8: return aom_highbd_8_mse16x8;
        case BLOCK_8X16: return aom_highbd_8_mse8x16;
        default: return aom_highbd_8_mse16x16;
      }
      break;
    case 10:
      switch (bsize) {
        case BLOCK_8X8: return aom_highbd_10_mse8x8;
        case BLOCK_16X8: return aom_highbd_10_mse16x8;
        case BLOCK_8X16: return aom_highbd_10_mse8x16;
        default: return aom_highbd_10_mse16x16;
      }
      break;
    case 12:
      switch (bsize) {
        case BLOCK_8X8: return aom_highbd_12_mse8x8;
        case BLOCK_16X8: return aom_highbd_12_mse16x8;
        case BLOCK_8X16: return aom_highbd_12_mse8x16;
        default: return aom_highbd_12_mse16x16;
      }
      break;
  }
}

static unsigned int highbd_get_prediction_error(BLOCK_SIZE bsize,
                                                const struct buf_2d *src,
                                                const struct buf_2d *ref,
                                                int bd) {
  unsigned int sse;
  const aom_variance_fn_t fn = highbd_get_block_variance_fn(bsize, bd);
  fn(src->buf, src->stride, ref->buf, ref->stride, &sse);
  return sse;
}

// Refine the motion search range according to the frame dimension
// for first pass test.
static int get_search_range(const AV1_COMP *cpi) {
  int sr = 0;
  const int dim = AOMMIN(cpi->initial_width, cpi->initial_height);

  while ((dim << sr) < MAX_FULL_PEL_VAL) ++sr;
  return sr;
}

static void first_pass_motion_search(AV1_COMP *cpi, MACROBLOCK *x,
                                     const MV *ref_mv, MV *best_mv,
                                     int *best_motion_err) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MV tmp_mv = kZeroMv;
  MV ref_mv_full = { ref_mv->row >> 3, ref_mv->col >> 3 };
  int num00, tmp_err, n;
  const BLOCK_SIZE bsize = xd->mi[0]->sb_type;
  aom_variance_fn_ptr_t v_fn_ptr = cpi->fn_ptr[bsize];
  const int new_mv_mode_penalty = NEW_MV_MODE_PENALTY;

  int step_param = 3;
  int further_steps = (MAX_MVSEARCH_STEPS - 1) - step_param;
  const int sr = get_search_range(cpi);
  step_param += sr;
  further_steps -= sr;

  // Override the default variance function to use MSE.
  v_fn_ptr.vf = get_block_variance_fn(bsize);
  if (is_cur_buf_hbd(xd)) {
    v_fn_ptr.vf = highbd_get_block_variance_fn(bsize, xd->bd);
  }

  // Center the initial step/diamond search on best mv.
  tmp_err = cpi->diamond_search_sad(x, &cpi->ss_cfg[SS_CFG_SRC], &ref_mv_full,
                                    &tmp_mv, step_param, x->sadperbit16, &num00,
                                    &v_fn_ptr, ref_mv);
  if (tmp_err < INT_MAX)
    tmp_err = av1_get_mvpred_var(x, &tmp_mv, ref_mv, &v_fn_ptr, 1);
  if (tmp_err < INT_MAX - new_mv_mode_penalty) tmp_err += new_mv_mode_penalty;

  if (tmp_err < *best_motion_err) {
    *best_motion_err = tmp_err;
    *best_mv = tmp_mv;
  }

  // Carry out further step/diamond searches as necessary.
  n = num00;
  num00 = 0;

  while (n < further_steps) {
    ++n;

    if (num00) {
      --num00;
    } else {
      tmp_err = cpi->diamond_search_sad(
          x, &cpi->ss_cfg[SS_CFG_SRC], &ref_mv_full, &tmp_mv, step_param + n,
          x->sadperbit16, &num00, &v_fn_ptr, ref_mv);
      if (tmp_err < INT_MAX)
        tmp_err = av1_get_mvpred_var(x, &tmp_mv, ref_mv, &v_fn_ptr, 1);
      if (tmp_err < INT_MAX - new_mv_mode_penalty)
        tmp_err += new_mv_mode_penalty;

      if (tmp_err < *best_motion_err) {
        *best_motion_err = tmp_err;
        *best_mv = tmp_mv;
      }
    }
  }
}

static BLOCK_SIZE get_bsize(const AV1_COMMON *cm, int mb_row, int mb_col) {
  if (mi_size_wide[BLOCK_16X16] * mb_col + mi_size_wide[BLOCK_8X8] <
      cm->mi_cols) {
    return mi_size_wide[BLOCK_16X16] * mb_row + mi_size_wide[BLOCK_8X8] <
                   cm->mi_rows
               ? BLOCK_16X16
               : BLOCK_16X8;
  } else {
    return mi_size_wide[BLOCK_16X16] * mb_row + mi_size_wide[BLOCK_8X8] <
                   cm->mi_rows
               ? BLOCK_8X16
               : BLOCK_8X8;
  }
}

static int find_fp_qindex(aom_bit_depth_t bit_depth) {
  return av1_find_qindex(FIRST_PASS_Q, bit_depth, 0, QINDEX_RANGE - 1);
}

static double raw_motion_error_stdev(int *raw_motion_err_list,
                                     int raw_motion_err_counts) {
  int64_t sum_raw_err = 0;
  double raw_err_avg = 0;
  double raw_err_stdev = 0;
  if (raw_motion_err_counts == 0) return 0;

  int i;
  for (i = 0; i < raw_motion_err_counts; i++) {
    sum_raw_err += raw_motion_err_list[i];
  }
  raw_err_avg = (double)sum_raw_err / raw_motion_err_counts;
  for (i = 0; i < raw_motion_err_counts; i++) {
    raw_err_stdev += (raw_motion_err_list[i] - raw_err_avg) *
                     (raw_motion_err_list[i] - raw_err_avg);
  }
  // Calculate the standard deviation for the motion error of all the inter
  // blocks of the 0,0 motion using the last source
  // frame as the reference.
  raw_err_stdev = sqrt(raw_err_stdev / raw_motion_err_counts);
  return raw_err_stdev;
}

#define UL_INTRA_THRESH 50
#define INVALID_ROW -1
void av1_first_pass(AV1_COMP *cpi, const int64_t ts_duration) {
  int mb_row, mb_col;
  MACROBLOCK *const x = &cpi->td.mb;
  AV1_COMMON *const cm = &cpi->common;
  CurrentFrame *const current_frame = &cm->current_frame;
  const SequenceHeader *const seq_params = &cm->seq_params;
  const int num_planes = av1_num_planes(cm);
  MACROBLOCKD *const xd = &x->e_mbd;
  TileInfo tile;
  struct macroblock_plane *const p = x->plane;
  struct macroblockd_plane *const pd = xd->plane;
  const PICK_MODE_CONTEXT *ctx =
      &cpi->td.pc_root[MAX_MIB_SIZE_LOG2 - MIN_MIB_SIZE_LOG2]->none;
  int i;

  int recon_yoffset, src_yoffset, recon_uvoffset;
  int64_t intra_error = 0;
  int64_t frame_avg_wavelet_energy = 0;
  int64_t coded_error = 0;
  int64_t sr_coded_error = 0;
  int64_t tr_coded_error = 0;

  int sum_mvr = 0, sum_mvc = 0;
  int sum_mvr_abs = 0, sum_mvc_abs = 0;
  int64_t sum_mvrs = 0, sum_mvcs = 0;
  int mvcount = 0;
  int intercount = 0;
  int second_ref_count = 0;
  int third_ref_count = 0;
  const int intrapenalty = INTRA_MODE_PENALTY;
  double neutral_count;
  int intra_skip_count = 0;
  int image_data_start_row = INVALID_ROW;
  int new_mv_count = 0;
  int sum_in_vectors = 0;
  MV lastmv = kZeroMv;
  TWO_PASS *twopass = &cpi->twopass;
  int recon_y_stride, src_y_stride, recon_uv_stride, uv_mb_height;

  const YV12_BUFFER_CONFIG *const lst_yv12 =
      get_ref_frame_yv12_buf(cm, LAST_FRAME);
  const YV12_BUFFER_CONFIG *gld_yv12 = get_ref_frame_yv12_buf(cm, GOLDEN_FRAME);
  const YV12_BUFFER_CONFIG *alt_yv12 = NULL;
  const int alt_offset = 16 - (current_frame->frame_number % 16);
  if (alt_offset < 16) {
    const struct lookahead_entry *const alt_buf =
        av1_lookahead_peek(cpi->lookahead, alt_offset);
    if (alt_buf != NULL) {
      alt_yv12 = &alt_buf->img;
    }
  }
  YV12_BUFFER_CONFIG *const new_yv12 = &cm->cur_frame->buf;
  double intra_factor;
  double brightness_factor;
  const int qindex = find_fp_qindex(seq_params->bit_depth);
  const int mb_scale = mi_size_wide[BLOCK_16X16];

  int *raw_motion_err_list;
  int raw_motion_err_counts = 0;
  CHECK_MEM_ERROR(
      cm, raw_motion_err_list,
      aom_calloc(cm->mb_rows * cm->mb_cols, sizeof(*raw_motion_err_list)));
  // First pass code requires valid last and new frame buffers.
  assert(new_yv12 != NULL);
  assert(frame_is_intra_only(cm) || (lst_yv12 != NULL));

  av1_setup_frame_size(cpi);
  aom_clear_system_state();

  xd->mi = cm->mi_grid_visible;
  xd->mi[0] = cm->mi;
  x->e_mbd.mi[0]->sb_type = BLOCK_16X16;

  intra_factor = 0.0;
  brightness_factor = 0.0;
  neutral_count = 0.0;

  // Do not use periodic key frames.
  cpi->rc.frames_to_key = INT_MAX;

  av1_set_quantizer(cm, qindex);

  av1_setup_block_planes(&x->e_mbd, seq_params->subsampling_x,
                         seq_params->subsampling_y, num_planes);

  av1_setup_src_planes(x, cpi->source, 0, 0, num_planes,
                       x->e_mbd.mi[0]->sb_type);
  av1_setup_dst_planes(xd->plane, seq_params->sb_size, new_yv12, 0, 0, 0,
                       num_planes);

  if (!frame_is_intra_only(cm)) {
    av1_setup_pre_planes(xd, 0, lst_yv12, 0, 0, NULL, num_planes);
  }

  xd->mi = cm->mi_grid_visible;
  xd->mi[0] = cm->mi;

  // Don't store luma on the fist pass since chroma is not computed
  xd->cfl.store_y = 0;
  av1_frame_init_quantizer(cpi);

  for (i = 0; i < num_planes; ++i) {
    p[i].coeff = ctx->coeff[i];
    p[i].qcoeff = ctx->qcoeff[i];
    pd[i].dqcoeff = ctx->dqcoeff[i];
    p[i].eobs = ctx->eobs[i];
    p[i].txb_entropy_ctx = ctx->txb_entropy_ctx[i];
  }

  av1_init_mv_probs(cm);
  av1_initialize_rd_consts(cpi);

  // Tiling is ignored in the first pass.
  av1_tile_init(&tile, cm, 0, 0);
  src_y_stride = cpi->source->y_stride;
  recon_y_stride = new_yv12->y_stride;
  recon_uv_stride = new_yv12->uv_stride;
  uv_mb_height = 16 >> (new_yv12->y_height > new_yv12->uv_height);

  for (mb_row = 0; mb_row < cm->mb_rows; ++mb_row) {
    MV best_ref_mv = kZeroMv;

    // Reset above block coeffs.
    xd->up_available = (mb_row != 0);
    recon_yoffset = (mb_row * recon_y_stride * 16);
    src_yoffset = (mb_row * src_y_stride * 16);
    recon_uvoffset = (mb_row * recon_uv_stride * uv_mb_height);
    int alt_yv12_yoffset =
        (alt_yv12 != NULL) ? mb_row * alt_yv12->y_stride * 16 : -1;

    // Set up limit values for motion vectors to prevent them extending
    // outside the UMV borders.
    x->mv_limits.row_min = -((mb_row * 16) + BORDER_MV_PIXELS_B16);
    x->mv_limits.row_max =
        ((cm->mb_rows - 1 - mb_row) * 16) + BORDER_MV_PIXELS_B16;

    for (mb_col = 0; mb_col < cm->mb_cols; ++mb_col) {
      int this_intra_error;
      const int use_dc_pred = (mb_col || mb_row) && (!mb_col || !mb_row);
      const BLOCK_SIZE bsize = get_bsize(cm, mb_row, mb_col);
      double log_intra;
      int level_sample;

      aom_clear_system_state();

      const int idx_str = xd->mi_stride * mb_row * mb_scale + mb_col * mb_scale;
      xd->mi = cm->mi_grid_visible + idx_str;
      xd->mi[0] = cm->mi + idx_str;
      xd->plane[0].dst.buf = new_yv12->y_buffer + recon_yoffset;
      xd->plane[1].dst.buf = new_yv12->u_buffer + recon_uvoffset;
      xd->plane[2].dst.buf = new_yv12->v_buffer + recon_uvoffset;
      xd->left_available = (mb_col != 0);
      xd->mi[0]->sb_type = bsize;
      xd->mi[0]->ref_frame[0] = INTRA_FRAME;
      set_mi_row_col(xd, &tile, mb_row * mb_scale, mi_size_high[bsize],
                     mb_col * mb_scale, mi_size_wide[bsize], cm->mi_rows,
                     cm->mi_cols);

      set_plane_n4(xd, mi_size_wide[bsize], mi_size_high[bsize], num_planes);

      // Do intra 16x16 prediction.
      xd->mi[0]->segment_id = 0;
      xd->lossless[xd->mi[0]->segment_id] = (qindex == 0);
      xd->mi[0]->mode = DC_PRED;
      xd->mi[0]->tx_size =
          use_dc_pred ? (bsize >= BLOCK_16X16 ? TX_16X16 : TX_8X8) : TX_4X4;
      av1_encode_intra_block_plane(cpi, x, bsize, 0, 0, mb_row * 2, mb_col * 2);
      this_intra_error = aom_get_mb_ss(x->plane[0].src_diff);

      if (this_intra_error < UL_INTRA_THRESH) {
        ++intra_skip_count;
      } else if ((mb_col > 0) && (image_data_start_row == INVALID_ROW)) {
        image_data_start_row = mb_row;
      }

      if (seq_params->use_highbitdepth) {
        switch (seq_params->bit_depth) {
          case AOM_BITS_8: break;
          case AOM_BITS_10: this_intra_error >>= 4; break;
          case AOM_BITS_12: this_intra_error >>= 8; break;
          default:
            assert(0 &&
                   "seq_params->bit_depth should be AOM_BITS_8, "
                   "AOM_BITS_10 or AOM_BITS_12");
            return;
        }
      }

      aom_clear_system_state();
      log_intra = log(this_intra_error + 1.0);
      if (log_intra < 10.0)
        intra_factor += 1.0 + ((10.0 - log_intra) * 0.05);
      else
        intra_factor += 1.0;

      if (seq_params->use_highbitdepth)
        level_sample = CONVERT_TO_SHORTPTR(x->plane[0].src.buf)[0];
      else
        level_sample = x->plane[0].src.buf[0];
      if ((level_sample < DARK_THRESH) && (log_intra < 9.0))
        brightness_factor += 1.0 + (0.01 * (DARK_THRESH - level_sample));
      else
        brightness_factor += 1.0;

      // Intrapenalty below deals with situations where the intra and inter
      // error scores are very low (e.g. a plain black frame).
      // We do not have special cases in first pass for 0,0 and nearest etc so
      // all inter modes carry an overhead cost estimate for the mv.
      // When the error score is very low this causes us to pick all or lots of
      // INTRA modes and throw lots of key frames.
      // This penalty adds a cost matching that of a 0,0 mv to the intra case.
      this_intra_error += intrapenalty;

      // Accumulate the intra error.
      intra_error += (int64_t)this_intra_error;

      const int hbd = is_cur_buf_hbd(xd);
      const int stride = x->plane[0].src.stride;
      uint8_t *buf = x->plane[0].src.buf;
      for (int r8 = 0; r8 < 2; ++r8) {
        for (int c8 = 0; c8 < 2; ++c8) {
          frame_avg_wavelet_energy += av1_haar_ac_sad_8x8_uint8_input(
              buf + c8 * 8 + r8 * 8 * stride, stride, hbd);
        }
      }

      // Set up limit values for motion vectors to prevent them extending
      // outside the UMV borders.
      x->mv_limits.col_min = -((mb_col * 16) + BORDER_MV_PIXELS_B16);
      x->mv_limits.col_max =
          ((cm->mb_cols - 1 - mb_col) * 16) + BORDER_MV_PIXELS_B16;

      if (!frame_is_intra_only(cm)) {  // Do a motion search
        int tmp_err, motion_error, raw_motion_error;
        // Assume 0,0 motion with no mv overhead.
        MV mv = kZeroMv, tmp_mv = kZeroMv;
        struct buf_2d unscaled_last_source_buf_2d;

        xd->plane[0].pre[0].buf = lst_yv12->y_buffer + recon_yoffset;
        if (is_cur_buf_hbd(xd)) {
          motion_error = highbd_get_prediction_error(
              bsize, &x->plane[0].src, &xd->plane[0].pre[0], xd->bd);
        } else {
          motion_error = get_prediction_error(bsize, &x->plane[0].src,
                                              &xd->plane[0].pre[0]);
        }

        // Compute the motion error of the 0,0 motion using the last source
        // frame as the reference. Skip the further motion search on
        // reconstructed frame if this error is small.
        unscaled_last_source_buf_2d.buf =
            cpi->unscaled_last_source->y_buffer + src_yoffset;
        unscaled_last_source_buf_2d.stride =
            cpi->unscaled_last_source->y_stride;
        if (is_cur_buf_hbd(xd)) {
          raw_motion_error = highbd_get_prediction_error(
              bsize, &x->plane[0].src, &unscaled_last_source_buf_2d, xd->bd);
        } else {
          raw_motion_error = get_prediction_error(bsize, &x->plane[0].src,
                                                  &unscaled_last_source_buf_2d);
        }

        // TODO(pengchong): Replace the hard-coded threshold
        if (raw_motion_error > 25) {
          // Test last reference frame using the previous best mv as the
          // starting point (best reference) for the search.
          first_pass_motion_search(cpi, x, &best_ref_mv, &mv, &motion_error);

          // If the current best reference mv is not centered on 0,0 then do a
          // 0,0 based search as well.
          if (!is_zero_mv(&best_ref_mv)) {
            tmp_err = INT_MAX;
            first_pass_motion_search(cpi, x, &kZeroMv, &tmp_mv, &tmp_err);

            if (tmp_err < motion_error) {
              motion_error = tmp_err;
              mv = tmp_mv;
            }
          }

          // Motion search in 2nd reference frame.
          int gf_motion_error;
          if ((current_frame->frame_number > 1) && gld_yv12 != NULL) {
            // Assume 0,0 motion with no mv overhead.
            xd->plane[0].pre[0].buf = gld_yv12->y_buffer + recon_yoffset;
            if (is_cur_buf_hbd(xd)) {
              gf_motion_error = highbd_get_prediction_error(
                  bsize, &x->plane[0].src, &xd->plane[0].pre[0], xd->bd);
            } else {
              gf_motion_error = get_prediction_error(bsize, &x->plane[0].src,
                                                     &xd->plane[0].pre[0]);
            }

            first_pass_motion_search(cpi, x, &kZeroMv, &tmp_mv,
                                     &gf_motion_error);

            if (gf_motion_error < motion_error &&
                gf_motion_error < this_intra_error)
              ++second_ref_count;

            // Reset to last frame as reference buffer.
            xd->plane[0].pre[0].buf = lst_yv12->y_buffer + recon_yoffset;
            xd->plane[1].pre[0].buf = lst_yv12->u_buffer + recon_uvoffset;
            xd->plane[2].pre[0].buf = lst_yv12->v_buffer + recon_uvoffset;

            // In accumulating a score for the 2nd reference frame take the
            // best of the motion predicted score and the intra coded error
            // (just as will be done for) accumulation of "coded_error" for
            // the last frame.
            if (gf_motion_error < this_intra_error)
              sr_coded_error += gf_motion_error;
            else
              sr_coded_error += this_intra_error;
          } else {
            gf_motion_error = motion_error;
            sr_coded_error += motion_error;
          }

          // Motion search in 3rd reference frame.
          if (alt_yv12 != NULL) {
            xd->plane[0].pre[0].buf = alt_yv12->y_buffer + alt_yv12_yoffset;
            xd->plane[0].pre[0].stride = alt_yv12->y_stride;
            int alt_motion_error;
            if (is_cur_buf_hbd(xd)) {
              alt_motion_error = highbd_get_prediction_error(
                  bsize, &x->plane[0].src, &xd->plane[0].pre[0], xd->bd);
            } else {
              alt_motion_error = get_prediction_error(bsize, &x->plane[0].src,
                                                      &xd->plane[0].pre[0]);
            }

            first_pass_motion_search(cpi, x, &kZeroMv, &tmp_mv,
                                     &alt_motion_error);

            if (alt_motion_error < motion_error &&
                alt_motion_error < gf_motion_error &&
                alt_motion_error < this_intra_error)
              ++third_ref_count;

            // Reset to last frame as reference buffer.
            xd->plane[0].pre[0].buf = lst_yv12->y_buffer + recon_yoffset;
            xd->plane[0].pre[0].stride = lst_yv12->y_stride;

            // In accumulating a score for the 3rd reference frame take the
            // best of the motion predicted score and the intra coded error
            // (just as will be done for) accumulation of "coded_error" for
            // the last frame.
            tr_coded_error += AOMMIN(alt_motion_error, this_intra_error);
          } else {
            tr_coded_error += motion_error;
          }
        } else {
          sr_coded_error += motion_error;
          tr_coded_error += motion_error;
        }

        // Start by assuming that intra mode is best.
        best_ref_mv.row = 0;
        best_ref_mv.col = 0;

        if (motion_error <= this_intra_error) {
          aom_clear_system_state();

          // Keep a count of cases where the inter and intra were very close
          // and very low. This helps with scene cut detection for example in
          // cropped clips with black bars at the sides or top and bottom.
          if (((this_intra_error - intrapenalty) * 9 <= motion_error * 10) &&
              (this_intra_error < (2 * intrapenalty))) {
            neutral_count += 1.0;
            // Also track cases where the intra is not much worse than the inter
            // and use this in limiting the GF/arf group length.
          } else if ((this_intra_error > NCOUNT_INTRA_THRESH) &&
                     (this_intra_error <
                      (NCOUNT_INTRA_FACTOR * motion_error))) {
            neutral_count += (double)motion_error /
                             DOUBLE_DIVIDE_CHECK((double)this_intra_error);
          }

          mv.row *= 8;
          mv.col *= 8;
          this_intra_error = motion_error;
          xd->mi[0]->mode = NEWMV;
          xd->mi[0]->mv[0].as_mv = mv;
          xd->mi[0]->tx_size = TX_4X4;
          xd->mi[0]->ref_frame[0] = LAST_FRAME;
          xd->mi[0]->ref_frame[1] = NONE_FRAME;
          av1_enc_build_inter_predictor(cm, xd, mb_row * mb_scale,
                                        mb_col * mb_scale, NULL, bsize,
                                        AOM_PLANE_Y, AOM_PLANE_Y);
          av1_encode_sby_pass1(cm, x, bsize);
          sum_mvr += mv.row;
          sum_mvr_abs += abs(mv.row);
          sum_mvc += mv.col;
          sum_mvc_abs += abs(mv.col);
          sum_mvrs += mv.row * mv.row;
          sum_mvcs += mv.col * mv.col;
          ++intercount;

          best_ref_mv = mv;

          if (!is_zero_mv(&mv)) {
            ++mvcount;

            // Non-zero vector, was it different from the last non zero vector?
            if (!is_equal_mv(&mv, &lastmv)) ++new_mv_count;
            lastmv = mv;

            // Does the row vector point inwards or outwards?
            if (mb_row < cm->mb_rows / 2) {
              if (mv.row > 0)
                --sum_in_vectors;
              else if (mv.row < 0)
                ++sum_in_vectors;
            } else if (mb_row > cm->mb_rows / 2) {
              if (mv.row > 0)
                ++sum_in_vectors;
              else if (mv.row < 0)
                --sum_in_vectors;
            }

            // Does the col vector point inwards or outwards?
            if (mb_col < cm->mb_cols / 2) {
              if (mv.col > 0)
                --sum_in_vectors;
              else if (mv.col < 0)
                ++sum_in_vectors;
            } else if (mb_col > cm->mb_cols / 2) {
              if (mv.col > 0)
                ++sum_in_vectors;
              else if (mv.col < 0)
                --sum_in_vectors;
            }
          }
        }
        raw_motion_err_list[raw_motion_err_counts++] = raw_motion_error;
      } else {
        sr_coded_error += (int64_t)this_intra_error;
        tr_coded_error += (int64_t)this_intra_error;
      }
      coded_error += (int64_t)this_intra_error;

      // Adjust to the next column of MBs.
      x->plane[0].src.buf += 16;
      x->plane[1].src.buf += uv_mb_height;
      x->plane[2].src.buf += uv_mb_height;

      recon_yoffset += 16;
      src_yoffset += 16;
      recon_uvoffset += uv_mb_height;
      alt_yv12_yoffset += 16;
    }
    // Adjust to the next row of MBs.
    x->plane[0].src.buf += 16 * x->plane[0].src.stride - 16 * cm->mb_cols;
    x->plane[1].src.buf +=
        uv_mb_height * x->plane[1].src.stride - uv_mb_height * cm->mb_cols;
    x->plane[2].src.buf +=
        uv_mb_height * x->plane[1].src.stride - uv_mb_height * cm->mb_cols;

    aom_clear_system_state();
  }
  const double raw_err_stdev =
      raw_motion_error_stdev(raw_motion_err_list, raw_motion_err_counts);
  aom_free(raw_motion_err_list);

  // Clamp the image start to rows/2. This number of rows is discarded top
  // and bottom as dead data so rows / 2 means the frame is blank.
  if ((image_data_start_row > cm->mb_rows / 2) ||
      (image_data_start_row == INVALID_ROW)) {
    image_data_start_row = cm->mb_rows / 2;
  }
  // Exclude any image dead zone
  if (image_data_start_row > 0) {
    intra_skip_count =
        AOMMAX(0, intra_skip_count - (image_data_start_row * cm->mb_cols * 2));
  }

  FIRSTPASS_STATS *this_frame_stats =
      &twopass->frame_stats_arr[twopass->frame_stats_next_idx];
  {
    FIRSTPASS_STATS fps;
    // The minimum error here insures some bit allocation to frames even
    // in static regions. The allocation per MB declines for larger formats
    // where the typical "real" energy per MB also falls.
    // Initial estimate here uses sqrt(mbs) to define the min_err, where the
    // number of mbs is proportional to the image area.
    const int num_mbs = (cpi->oxcf.resize_mode != RESIZE_NONE)
                            ? cpi->initial_mbs
                            : cpi->common.MBs;
    const double min_err = 200 * sqrt(num_mbs);

    intra_factor = intra_factor / (double)num_mbs;
    brightness_factor = brightness_factor / (double)num_mbs;
    fps.weight = intra_factor * brightness_factor;

    fps.frame = current_frame->frame_number;
    fps.coded_error = (double)(coded_error >> 8) + min_err;
    fps.sr_coded_error = (double)(sr_coded_error >> 8) + min_err;
    fps.tr_coded_error = (double)(tr_coded_error >> 8) + min_err;
    fps.intra_error = (double)(intra_error >> 8) + min_err;
    fps.frame_avg_wavelet_energy = (double)frame_avg_wavelet_energy;
    fps.count = 1.0;
    fps.pcnt_inter = (double)intercount / num_mbs;
    fps.pcnt_second_ref = (double)second_ref_count / num_mbs;
    fps.pcnt_third_ref = (double)third_ref_count / num_mbs;
    fps.pcnt_neutral = (double)neutral_count / num_mbs;
    fps.intra_skip_pct = (double)intra_skip_count / num_mbs;
    fps.inactive_zone_rows = (double)image_data_start_row;
    fps.inactive_zone_cols = (double)0;  // TODO(paulwilkins): fix
    fps.raw_error_stdev = raw_err_stdev;

    if (mvcount > 0) {
      fps.MVr = (double)sum_mvr / mvcount;
      fps.mvr_abs = (double)sum_mvr_abs / mvcount;
      fps.MVc = (double)sum_mvc / mvcount;
      fps.mvc_abs = (double)sum_mvc_abs / mvcount;
      fps.MVrv =
          ((double)sum_mvrs - ((double)sum_mvr * sum_mvr / mvcount)) / mvcount;
      fps.MVcv =
          ((double)sum_mvcs - ((double)sum_mvc * sum_mvc / mvcount)) / mvcount;
      fps.mv_in_out_count = (double)sum_in_vectors / (mvcount * 2);
      fps.new_mv_count = new_mv_count;
      fps.pcnt_motion = (double)mvcount / num_mbs;
    } else {
      fps.MVr = 0.0;
      fps.mvr_abs = 0.0;
      fps.MVc = 0.0;
      fps.mvc_abs = 0.0;
      fps.MVrv = 0.0;
      fps.MVcv = 0.0;
      fps.mv_in_out_count = 0.0;
      fps.new_mv_count = 0.0;
      fps.pcnt_motion = 0.0;
    }

    // TODO(paulwilkins):  Handle the case when duration is set to 0, or
    // something less than the full time between subsequent values of
    // cpi->source_time_stamp.
    fps.duration = (double)ts_duration;

    // We will store the stats inside the persistent twopass struct (and NOT the
    // local variable 'fps'), and then cpi->output_pkt_list will point to it.
    *this_frame_stats = fps;
    output_stats(this_frame_stats, cpi->output_pkt_list);
    accumulate_stats(&twopass->total_stats, &fps);
    // Update circular index.
    twopass->frame_stats_next_idx =
        (twopass->frame_stats_next_idx + 1) % MAX_LAG_BUFFERS;
  }

  // Copy the previous Last Frame back into gf buffer if the prediction is good
  // enough... but also don't allow it to lag too far.
  if ((twopass->sr_update_lag > 3) ||
      ((current_frame->frame_number > 0) &&
       (this_frame_stats->pcnt_inter > 0.20) &&
       ((this_frame_stats->intra_error /
         DOUBLE_DIVIDE_CHECK(this_frame_stats->coded_error)) > 2.0))) {
    if (gld_yv12 != NULL) {
      assign_frame_buffer_p(
          &cm->ref_frame_map[get_ref_frame_map_idx(cm, GOLDEN_FRAME)],
          cm->ref_frame_map[get_ref_frame_map_idx(cm, LAST_FRAME)]);
    }
    twopass->sr_update_lag = 1;
  } else {
    ++twopass->sr_update_lag;
  }

  aom_extend_frame_borders(new_yv12, num_planes);

  // The frame we just compressed now becomes the last frame.
  assign_frame_buffer_p(
      &cm->ref_frame_map[get_ref_frame_map_idx(cm, LAST_FRAME)], cm->cur_frame);

  // Special case for the first frame. Copy into the GF buffer as a second
  // reference.
  if (current_frame->frame_number == 0 &&
      get_ref_frame_map_idx(cm, GOLDEN_FRAME) != INVALID_IDX) {
    assign_frame_buffer_p(
        &cm->ref_frame_map[get_ref_frame_map_idx(cm, GOLDEN_FRAME)],
        cm->ref_frame_map[get_ref_frame_map_idx(cm, LAST_FRAME)]);
  }

  // Use this to see what the first pass reconstruction looks like.
  if (0) {
    char filename[512];
    FILE *recon_file;
    snprintf(filename, sizeof(filename), "enc%04d.yuv",
             (int)current_frame->frame_number);

    if (current_frame->frame_number == 0)
      recon_file = fopen(filename, "wb");
    else
      recon_file = fopen(filename, "ab");

    (void)fwrite(lst_yv12->buffer_alloc, lst_yv12->frame_size, 1, recon_file);
    fclose(recon_file);
  }

  ++current_frame->frame_number;
}
