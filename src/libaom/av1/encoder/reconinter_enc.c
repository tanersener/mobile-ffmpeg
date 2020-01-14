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

#include <assert.h>
#include <stdio.h>
#include <limits.h>

#include "config/aom_config.h"
#include "config/aom_dsp_rtcd.h"
#include "config/aom_scale_rtcd.h"

#include "aom/aom_integer.h"
#include "aom_dsp/blend.h"

#include "av1/common/blockd.h"
#include "av1/common/mvref_common.h"
#include "av1/common/reconinter.h"
#include "av1/common/reconintra.h"
#include "av1/common/onyxc_int.h"
#include "av1/common/obmc.h"
#include "av1/encoder/reconinter_enc.h"

static INLINE void build_inter_predictors(const AV1_COMMON *cm, MACROBLOCKD *xd,
                                          int plane, const MB_MODE_INFO *mi,
                                          int bw, int bh, int mi_x, int mi_y) {
  struct macroblockd_plane *const pd = &xd->plane[plane];
  int is_compound = has_second_ref(mi);
  int ref;
  const int is_intrabc = is_intrabc_block(mi);
  assert(IMPLIES(is_intrabc, !is_compound));
  int is_global[2] = { 0, 0 };
  for (ref = 0; ref < 1 + is_compound; ++ref) {
    const WarpedMotionParams *const wm = &xd->global_motion[mi->ref_frame[ref]];
    is_global[ref] = is_global_mv_block(mi, wm->wmtype);
  }

  const BLOCK_SIZE bsize = mi->sb_type;
  const int ss_x = pd->subsampling_x;
  const int ss_y = pd->subsampling_y;
  int sub8x8_inter = (block_size_wide[bsize] < 8 && ss_x) ||
                     (block_size_high[bsize] < 8 && ss_y);

  if (is_intrabc) sub8x8_inter = 0;

  // For sub8x8 chroma blocks, we may be covering more than one luma block's
  // worth of pixels. Thus (mi_x, mi_y) may not be the correct coordinates for
  // the top-left corner of the prediction source - the correct top-left corner
  // is at (pre_x, pre_y).
  const int row_start = (block_size_high[bsize] == 4) && ss_y ? -1 : 0;
  const int col_start = (block_size_wide[bsize] == 4) && ss_x ? -1 : 0;
  const int pre_x = (mi_x + MI_SIZE * col_start) >> ss_x;
  const int pre_y = (mi_y + MI_SIZE * row_start) >> ss_y;

  if (sub8x8_inter) {
    for (int row = row_start; row <= 0 && sub8x8_inter; ++row) {
      for (int col = col_start; col <= 0; ++col) {
        const MB_MODE_INFO *this_mbmi = xd->mi[row * xd->mi_stride + col];
        if (!is_inter_block(this_mbmi)) sub8x8_inter = 0;
        if (is_intrabc_block(this_mbmi)) sub8x8_inter = 0;
      }
    }
  }

  if (sub8x8_inter) {
    // block size
    const int b4_w = block_size_wide[bsize] >> ss_x;
    const int b4_h = block_size_high[bsize] >> ss_y;
    const BLOCK_SIZE plane_bsize = get_plane_block_size(bsize, ss_x, ss_y);
    const int b8_w = block_size_wide[plane_bsize];
    const int b8_h = block_size_high[plane_bsize];
    assert(!is_compound);

    int row = row_start;
    for (int y = 0; y < b8_h; y += b4_h) {
      int col = col_start;
      for (int x = 0; x < b8_w; x += b4_w) {
        MB_MODE_INFO *this_mbmi = xd->mi[row * xd->mi_stride + col];
        int tmp_dst_stride = 8;
        assert(bw < 8 || bh < 8);
        struct buf_2d *const dst_buf = &pd->dst;
        uint8_t *dst = dst_buf->buf + dst_buf->stride * y + x;
        ref = 0;
        const RefCntBuffer *ref_buf =
            get_ref_frame_buf(cm, this_mbmi->ref_frame[ref]);
        const struct scale_factors *ref_scale_factors =
            get_ref_scale_factors_const(cm, this_mbmi->ref_frame[ref]);

        const struct scale_factors *const sf =
            is_intrabc ? &cm->sf_identity : ref_scale_factors;
        struct buf_2d pre_buf = {
          NULL,
          (plane == 1) ? ref_buf->buf.u_buffer : ref_buf->buf.v_buffer,
          ref_buf->buf.uv_crop_width,
          ref_buf->buf.uv_crop_height,
          ref_buf->buf.uv_stride,
        };

        if (is_intrabc) pre_buf = *dst_buf;

        const MV mv = this_mbmi->mv[ref].as_mv;
        InterPredParams inter_pred_params;
        av1_init_inter_params(&inter_pred_params, b4_w, b4_h, pre_y + y,
                              pre_x + x, pd->subsampling_x, pd->subsampling_y,
                              xd->bd, is_cur_buf_hbd(xd), mi->use_intrabc, sf,
                              &pre_buf, this_mbmi->interp_filters);

        inter_pred_params.conv_params = get_conv_params_no_round(
            ref, plane, xd->tmp_conv_dst, tmp_dst_stride, 0, xd->bd);
        inter_pred_params.conv_params.use_dist_wtd_comp_avg = 0;

        av1_build_inter_predictor(dst, dst_buf->stride, &mv,
                                  &inter_pred_params);
        ++col;
      }
      ++row;
    }

    return;
  }

  {
    InterPredParams inter_pred_params;

    struct buf_2d *const dst_buf = &pd->dst;
    uint8_t *const dst = dst_buf->buf;
    for (ref = 0; ref < 1 + is_compound; ++ref) {
      const struct scale_factors *const sf =
          is_intrabc ? &cm->sf_identity : xd->block_ref_scale_factors[ref];
      struct buf_2d pre_buf = is_intrabc ? *dst_buf : pd->pre[ref];
      const MV mv = mi->mv[ref].as_mv;

      WarpTypesAllowed warp_types;
      warp_types.global_warp_allowed = is_global[ref];
      warp_types.local_warp_allowed = mi->motion_mode == WARPED_CAUSAL;

      av1_init_inter_params(&inter_pred_params, bw, bh, pre_y, pre_x,
                            pd->subsampling_x, pd->subsampling_y, xd->bd,
                            is_cur_buf_hbd(xd), mi->use_intrabc, sf, &pre_buf,
                            mi->interp_filters);

      if (is_compound) av1_init_comp_mode(&inter_pred_params);

      inter_pred_params.conv_params = get_conv_params_no_round(
          ref, plane, xd->tmp_conv_dst, MAX_SB_SIZE, is_compound, xd->bd);

      av1_dist_wtd_comp_weight_assign(
          cm, mi, 0, &inter_pred_params.conv_params.fwd_offset,
          &inter_pred_params.conv_params.bck_offset,
          &inter_pred_params.conv_params.use_dist_wtd_comp_avg, is_compound);

      av1_init_warp_params(&inter_pred_params, &warp_types, ref, xd, mi);

      if (is_masked_compound_type(mi->interinter_comp.type)) {
        av1_init_mask_comp(&inter_pred_params, mi->sb_type,
                           &mi->interinter_comp);
        // Assigne physical buffer
        inter_pred_params.mask_comp.seg_mask = xd->seg_mask;
      }

      av1_build_inter_predictor(dst, dst_buf->stride, &mv, &inter_pred_params);
    }
  }
}

static void build_inter_predictors_for_plane(const AV1_COMMON *cm,
                                             MACROBLOCKD *xd, int mi_row,
                                             int mi_col, const BUFFER_SET *ctx,
                                             BLOCK_SIZE bsize, int plane_idx) {
  const struct macroblockd_plane *pd = &xd->plane[plane_idx];
  if (!is_chroma_reference(mi_row, mi_col, bsize, pd->subsampling_x,
                           pd->subsampling_y))
    return;

  const int mi_x = mi_col * MI_SIZE;
  const int mi_y = mi_row * MI_SIZE;
  build_inter_predictors(cm, xd, plane_idx, xd->mi[0], pd->width, pd->height,
                         mi_x, mi_y);

  if (is_interintra_pred(xd->mi[0])) {
    BUFFER_SET default_ctx = { { NULL, NULL, NULL }, { 0, 0, 0 } };
    if (!ctx) {
      default_ctx.plane[plane_idx] = xd->plane[plane_idx].dst.buf;
      default_ctx.stride[plane_idx] = xd->plane[plane_idx].dst.stride;
      ctx = &default_ctx;
    }
    av1_build_interintra_predictors_sbp(cm, xd, xd->plane[plane_idx].dst.buf,
                                        xd->plane[plane_idx].dst.stride, ctx,
                                        plane_idx, bsize);
  }
}

void av1_enc_build_inter_predictor(const AV1_COMMON *cm, MACROBLOCKD *xd,
                                   int mi_row, int mi_col,
                                   const BUFFER_SET *ctx, BLOCK_SIZE bsize,
                                   int plane_from, int plane_to) {
  for (int plane_idx = plane_from; plane_idx <= plane_to; ++plane_idx) {
    build_inter_predictors_for_plane(cm, xd, mi_row, mi_col, ctx, bsize,
                                     plane_idx);
  }
}

void av1_build_inter_predictor(uint8_t *dst, int dst_stride, const MV *src_mv,
                               InterPredParams *inter_pred_params) {
  SubpelParams subpel_params;
  const struct scale_factors *sf = inter_pred_params->scale_factors;

  struct buf_2d *pre_buf = &inter_pred_params->ref_frame_buf;
  int ssx = inter_pred_params->subsampling_x;
  int ssy = inter_pred_params->subsampling_y;
  int orig_pos_y = inter_pred_params->pix_row << SUBPEL_BITS;
  orig_pos_y += src_mv->row * (1 << (1 - ssy));
  int orig_pos_x = inter_pred_params->pix_col << SUBPEL_BITS;
  orig_pos_x += src_mv->col * (1 << (1 - ssx));
  int pos_y = sf->scale_value_y(orig_pos_y, sf);
  int pos_x = sf->scale_value_x(orig_pos_x, sf);
  pos_x += SCALE_EXTRA_OFF;
  pos_y += SCALE_EXTRA_OFF;

  const int top = -AOM_LEFT_TOP_MARGIN_SCALED(ssy);
  const int left = -AOM_LEFT_TOP_MARGIN_SCALED(ssx);
  const int bottom = (pre_buf->height + AOM_INTERP_EXTEND) << SCALE_SUBPEL_BITS;
  const int right = (pre_buf->width + AOM_INTERP_EXTEND) << SCALE_SUBPEL_BITS;
  pos_y = clamp(pos_y, top, bottom);
  pos_x = clamp(pos_x, left, right);

  uint8_t *src = pre_buf->buf0 +
                 (pos_y >> SCALE_SUBPEL_BITS) * pre_buf->stride +
                 (pos_x >> SCALE_SUBPEL_BITS);
  subpel_params.subpel_x = pos_x & SCALE_SUBPEL_MASK;
  subpel_params.subpel_y = pos_y & SCALE_SUBPEL_MASK;
  subpel_params.xs = sf->x_step_q4;
  subpel_params.ys = sf->y_step_q4;

  if (inter_pred_params->comp_mode == UNIFORM_SINGLE ||
      inter_pred_params->comp_mode == UNIFORM_COMP)
    av1_make_inter_predictor(src, pre_buf->stride, dst, dst_stride,
                             inter_pred_params, &subpel_params);
  else
    av1_make_masked_inter_predictor(src, pre_buf->stride, dst, dst_stride,
                                    inter_pred_params, &subpel_params);
}

static INLINE void build_obmc_prediction(MACROBLOCKD *xd, int rel_mi_row,
                                         int rel_mi_col, uint8_t op_mi_size,
                                         int dir, MB_MODE_INFO *above_mbmi,
                                         void *fun_ctxt, const int num_planes) {
  struct build_prediction_ctxt *ctxt = (struct build_prediction_ctxt *)fun_ctxt;
  av1_setup_address_for_obmc(xd, rel_mi_row, rel_mi_col, above_mbmi, ctxt,
                             num_planes);

  const int mi_x = (xd->mi_col + rel_mi_col) << MI_SIZE_LOG2;
  const int mi_y = (xd->mi_row + rel_mi_row) << MI_SIZE_LOG2;

  const BLOCK_SIZE bsize = xd->mi[0]->sb_type;

  InterPredParams inter_pred_params;

  for (int j = 0; j < num_planes; ++j) {
    const struct macroblockd_plane *pd = &xd->plane[j];
    int bw = 0, bh = 0;

    if (dir) {
      // prepare left reference block size
      bw = clamp(block_size_wide[bsize] >> (pd->subsampling_x + 1), 4,
                 block_size_wide[BLOCK_64X64] >> (pd->subsampling_x + 1));
      bh = (op_mi_size << MI_SIZE_LOG2) >> pd->subsampling_y;
    } else {
      // prepare above reference block size
      bw = (op_mi_size * MI_SIZE) >> pd->subsampling_x;
      bh = clamp(block_size_high[bsize] >> (pd->subsampling_y + 1), 4,
                 block_size_high[BLOCK_64X64] >> (pd->subsampling_y + 1));
    }

    if (av1_skip_u4x4_pred_in_obmc(bsize, pd, dir)) continue;

    const struct buf_2d *const pre_buf = &pd->pre[0];
    const MV mv = above_mbmi->mv[0].as_mv;

    av1_init_inter_params(&inter_pred_params, bw, bh, mi_y >> pd->subsampling_y,
                          mi_x >> pd->subsampling_x, pd->subsampling_x,
                          pd->subsampling_y, xd->bd, is_cur_buf_hbd(xd), 0,
                          xd->block_ref_scale_factors[0], pre_buf,
                          above_mbmi->interp_filters);
    inter_pred_params.conv_params = get_conv_params(0, j, xd->bd);

    av1_build_inter_predictor(pd->dst.buf, pd->dst.stride, &mv,
                              &inter_pred_params);
  }
}

void av1_build_prediction_by_above_preds(const AV1_COMMON *cm, MACROBLOCKD *xd,
                                         uint8_t *tmp_buf[MAX_MB_PLANE],
                                         int tmp_width[MAX_MB_PLANE],
                                         int tmp_height[MAX_MB_PLANE],
                                         int tmp_stride[MAX_MB_PLANE]) {
  if (!xd->up_available) return;
  struct build_prediction_ctxt ctxt = { cm,         tmp_buf,
                                        tmp_width,  tmp_height,
                                        tmp_stride, xd->mb_to_right_edge };
  BLOCK_SIZE bsize = xd->mi[0]->sb_type;
  foreach_overlappable_nb_above(cm, xd,
                                max_neighbor_obmc[mi_size_wide_log2[bsize]],
                                build_obmc_prediction, &ctxt);
}

void av1_build_prediction_by_left_preds(const AV1_COMMON *cm, MACROBLOCKD *xd,
                                        uint8_t *tmp_buf[MAX_MB_PLANE],
                                        int tmp_width[MAX_MB_PLANE],
                                        int tmp_height[MAX_MB_PLANE],
                                        int tmp_stride[MAX_MB_PLANE]) {
  if (!xd->left_available) return;
  struct build_prediction_ctxt ctxt = { cm,         tmp_buf,
                                        tmp_width,  tmp_height,
                                        tmp_stride, xd->mb_to_bottom_edge };
  BLOCK_SIZE bsize = xd->mi[0]->sb_type;
  foreach_overlappable_nb_left(cm, xd,
                               max_neighbor_obmc[mi_size_high_log2[bsize]],
                               build_obmc_prediction, &ctxt);
}

void av1_build_obmc_inter_predictors_sb(const AV1_COMMON *cm, MACROBLOCKD *xd) {
  const int num_planes = av1_num_planes(cm);
  uint8_t *dst_buf1[MAX_MB_PLANE], *dst_buf2[MAX_MB_PLANE];
  int dst_stride1[MAX_MB_PLANE] = { MAX_SB_SIZE, MAX_SB_SIZE, MAX_SB_SIZE };
  int dst_stride2[MAX_MB_PLANE] = { MAX_SB_SIZE, MAX_SB_SIZE, MAX_SB_SIZE };
  int dst_width1[MAX_MB_PLANE] = { MAX_SB_SIZE, MAX_SB_SIZE, MAX_SB_SIZE };
  int dst_width2[MAX_MB_PLANE] = { MAX_SB_SIZE, MAX_SB_SIZE, MAX_SB_SIZE };
  int dst_height1[MAX_MB_PLANE] = { MAX_SB_SIZE, MAX_SB_SIZE, MAX_SB_SIZE };
  int dst_height2[MAX_MB_PLANE] = { MAX_SB_SIZE, MAX_SB_SIZE, MAX_SB_SIZE };

  if (is_cur_buf_hbd(xd)) {
    int len = sizeof(uint16_t);
    dst_buf1[0] = CONVERT_TO_BYTEPTR(xd->tmp_obmc_bufs[0]);
    dst_buf1[1] =
        CONVERT_TO_BYTEPTR(xd->tmp_obmc_bufs[0] + MAX_SB_SQUARE * len);
    dst_buf1[2] =
        CONVERT_TO_BYTEPTR(xd->tmp_obmc_bufs[0] + MAX_SB_SQUARE * 2 * len);
    dst_buf2[0] = CONVERT_TO_BYTEPTR(xd->tmp_obmc_bufs[1]);
    dst_buf2[1] =
        CONVERT_TO_BYTEPTR(xd->tmp_obmc_bufs[1] + MAX_SB_SQUARE * len);
    dst_buf2[2] =
        CONVERT_TO_BYTEPTR(xd->tmp_obmc_bufs[1] + MAX_SB_SQUARE * 2 * len);
  } else {
    dst_buf1[0] = xd->tmp_obmc_bufs[0];
    dst_buf1[1] = xd->tmp_obmc_bufs[0] + MAX_SB_SQUARE;
    dst_buf1[2] = xd->tmp_obmc_bufs[0] + MAX_SB_SQUARE * 2;
    dst_buf2[0] = xd->tmp_obmc_bufs[1];
    dst_buf2[1] = xd->tmp_obmc_bufs[1] + MAX_SB_SQUARE;
    dst_buf2[2] = xd->tmp_obmc_bufs[1] + MAX_SB_SQUARE * 2;
  }

  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  av1_build_prediction_by_above_preds(cm, xd, dst_buf1, dst_width1, dst_height1,
                                      dst_stride1);
  av1_build_prediction_by_left_preds(cm, xd, dst_buf2, dst_width2, dst_height2,
                                     dst_stride2);
  av1_setup_dst_planes(xd->plane, xd->mi[0]->sb_type, &cm->cur_frame->buf,
                       mi_row, mi_col, 0, num_planes);
  av1_build_obmc_inter_prediction(cm, xd, dst_buf1, dst_stride1, dst_buf2,
                                  dst_stride2);
}

void av1_build_inter_predictors_for_planes_single_buf(
    MACROBLOCKD *xd, BLOCK_SIZE bsize, int plane_from, int plane_to, int ref,
    uint8_t *ext_dst[3], int ext_dst_stride[3]) {
  assert(bsize < BLOCK_SIZES_ALL);
  const MB_MODE_INFO *mi = xd->mi[0];
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  const int mi_x = mi_col * MI_SIZE;
  const int mi_y = mi_row * MI_SIZE;
  WarpTypesAllowed warp_types;
  const WarpedMotionParams *const wm = &xd->global_motion[mi->ref_frame[ref]];
  warp_types.global_warp_allowed = is_global_mv_block(mi, wm->wmtype);
  warp_types.local_warp_allowed = mi->motion_mode == WARPED_CAUSAL;

  for (int plane = plane_from; plane <= plane_to; ++plane) {
    const struct macroblockd_plane *pd = &xd->plane[plane];
    const BLOCK_SIZE plane_bsize =
        get_plane_block_size(bsize, pd->subsampling_x, pd->subsampling_y);
    const int bw = block_size_wide[plane_bsize];
    const int bh = block_size_high[plane_bsize];

    InterPredParams inter_pred_params;

    av1_init_inter_params(&inter_pred_params, bw, bh, mi_y >> pd->subsampling_y,
                          mi_x >> pd->subsampling_x, pd->subsampling_x,
                          pd->subsampling_y, xd->bd, is_cur_buf_hbd(xd), 0,
                          xd->block_ref_scale_factors[ref], &pd->pre[ref],
                          mi->interp_filters);
    inter_pred_params.conv_params = get_conv_params(0, plane, xd->bd);
    av1_init_warp_params(&inter_pred_params, &warp_types, ref, xd, mi);

    uint8_t *const dst = get_buf_by_bd(xd, ext_dst[plane]);
    const MV mv = mi->mv[ref].as_mv;

    av1_build_inter_predictor(dst, ext_dst_stride[plane], &mv,
                              &inter_pred_params);
  }
}

static void build_masked_compound(
    uint8_t *dst, int dst_stride, const uint8_t *src0, int src0_stride,
    const uint8_t *src1, int src1_stride,
    const INTERINTER_COMPOUND_DATA *const comp_data, BLOCK_SIZE sb_type, int h,
    int w) {
  // Derive subsampling from h and w passed in. May be refactored to
  // pass in subsampling factors directly.
  const int subh = (2 << mi_size_high_log2[sb_type]) == h;
  const int subw = (2 << mi_size_wide_log2[sb_type]) == w;
  const uint8_t *mask = av1_get_compound_type_mask(comp_data, sb_type);
  aom_blend_a64_mask(dst, dst_stride, src0, src0_stride, src1, src1_stride,
                     mask, block_size_wide[sb_type], w, h, subw, subh);
}

#if CONFIG_AV1_HIGHBITDEPTH
static void build_masked_compound_highbd(
    uint8_t *dst_8, int dst_stride, const uint8_t *src0_8, int src0_stride,
    const uint8_t *src1_8, int src1_stride,
    const INTERINTER_COMPOUND_DATA *const comp_data, BLOCK_SIZE sb_type, int h,
    int w, int bd) {
  // Derive subsampling from h and w passed in. May be refactored to
  // pass in subsampling factors directly.
  const int subh = (2 << mi_size_high_log2[sb_type]) == h;
  const int subw = (2 << mi_size_wide_log2[sb_type]) == w;
  const uint8_t *mask = av1_get_compound_type_mask(comp_data, sb_type);
  // const uint8_t *mask =
  //     av1_get_contiguous_soft_mask(wedge_index, wedge_sign, sb_type);
  aom_highbd_blend_a64_mask(dst_8, dst_stride, src0_8, src0_stride, src1_8,
                            src1_stride, mask, block_size_wide[sb_type], w, h,
                            subw, subh, bd);
}
#endif

static void build_wedge_inter_predictor_from_buf(
    MACROBLOCKD *xd, int plane, int x, int y, int w, int h, uint8_t *ext_dst0,
    int ext_dst_stride0, uint8_t *ext_dst1, int ext_dst_stride1) {
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const int is_compound = has_second_ref(mbmi);
  MACROBLOCKD_PLANE *const pd = &xd->plane[plane];
  struct buf_2d *const dst_buf = &pd->dst;
  uint8_t *const dst = dst_buf->buf + dst_buf->stride * y + x;
  mbmi->interinter_comp.seg_mask = xd->seg_mask;
  const INTERINTER_COMPOUND_DATA *comp_data = &mbmi->interinter_comp;
  const int is_hbd = is_cur_buf_hbd(xd);

  if (is_compound && is_masked_compound_type(comp_data->type)) {
    if (!plane && comp_data->type == COMPOUND_DIFFWTD) {
      if (is_hbd) {
        av1_build_compound_diffwtd_mask_highbd(
            comp_data->seg_mask, comp_data->mask_type,
            CONVERT_TO_BYTEPTR(ext_dst0), ext_dst_stride0,
            CONVERT_TO_BYTEPTR(ext_dst1), ext_dst_stride1, h, w, xd->bd);
      } else {
        av1_build_compound_diffwtd_mask(
            comp_data->seg_mask, comp_data->mask_type, ext_dst0,
            ext_dst_stride0, ext_dst1, ext_dst_stride1, h, w);
      }
    }
#if CONFIG_AV1_HIGHBITDEPTH
    if (is_hbd) {
      build_masked_compound_highbd(
          dst, dst_buf->stride, CONVERT_TO_BYTEPTR(ext_dst0), ext_dst_stride0,
          CONVERT_TO_BYTEPTR(ext_dst1), ext_dst_stride1, comp_data,
          mbmi->sb_type, h, w, xd->bd);
    } else {
      build_masked_compound(dst, dst_buf->stride, ext_dst0, ext_dst_stride0,
                            ext_dst1, ext_dst_stride1, comp_data, mbmi->sb_type,
                            h, w);
    }
#else
    build_masked_compound(dst, dst_buf->stride, ext_dst0, ext_dst_stride0,
                          ext_dst1, ext_dst_stride1, comp_data, mbmi->sb_type,
                          h, w);
#endif
  } else {
#if CONFIG_AV1_HIGHBITDEPTH
    if (is_hbd) {
      aom_highbd_convolve_copy(CONVERT_TO_BYTEPTR(ext_dst0), ext_dst_stride0,
                               dst, dst_buf->stride, NULL, 0, NULL, 0, w, h,
                               xd->bd);
    } else {
      aom_convolve_copy(ext_dst0, ext_dst_stride0, dst, dst_buf->stride, NULL,
                        0, NULL, 0, w, h);
    }
#else
    aom_convolve_copy(ext_dst0, ext_dst_stride0, dst, dst_buf->stride, NULL, 0,
                      NULL, 0, w, h);
#endif
  }
}

void av1_build_wedge_inter_predictor_from_buf(MACROBLOCKD *xd, BLOCK_SIZE bsize,
                                              int plane_from, int plane_to,
                                              uint8_t *ext_dst0[3],
                                              int ext_dst_stride0[3],
                                              uint8_t *ext_dst1[3],
                                              int ext_dst_stride1[3]) {
  int plane;
  assert(bsize < BLOCK_SIZES_ALL);
  for (plane = plane_from; plane <= plane_to; ++plane) {
    const BLOCK_SIZE plane_bsize = get_plane_block_size(
        bsize, xd->plane[plane].subsampling_x, xd->plane[plane].subsampling_y);
    const int bw = block_size_wide[plane_bsize];
    const int bh = block_size_high[plane_bsize];
    build_wedge_inter_predictor_from_buf(
        xd, plane, 0, 0, bw, bh, ext_dst0[plane], ext_dst_stride0[plane],
        ext_dst1[plane], ext_dst_stride1[plane]);
  }
}
