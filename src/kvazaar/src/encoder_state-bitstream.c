/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * Kvazaar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "encoder_state-bitstream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "cabac.h"
#include "checkpoint.h"
#include "cu.h"
#include "encoder.h"
#include "encoder_state-geometry.h"
#include "encoderstate.h"
#include "imagelist.h"
#include "kvazaar.h"
#include "kvz_math.h"
#include "nal.h"
#include "scalinglist.h"
#include "tables.h"
#include "threadqueue.h"
#include "videoframe.h"


static void encoder_state_write_bitstream_aud(encoder_state_t * const state)
{
  bitstream_t * const stream = &state->stream;
  kvz_nal_write(stream, KVZ_NAL_AUD_NUT, 0, 1);

  uint8_t pic_type = state->frame->slicetype == KVZ_SLICE_I ? 0
                   : state->frame->slicetype == KVZ_SLICE_P ? 1
                   :                                       2;
  WRITE_U(stream, pic_type, 3, "pic_type");

  kvz_bitstream_add_rbsp_trailing_bits(stream);
}

static void encoder_state_write_bitstream_PTL(bitstream_t *stream,
                                              encoder_state_t * const state)
{
  // PTL
  // Profile Tier
  WRITE_U(stream, 0, 2, "general_profile_space");
  WRITE_U(stream, 0, 1, "general_tier_flag");
  // Main Profile == 1,  Main 10 profile == 2
  WRITE_U(stream, (state->encoder_control->bitdepth == 8)?1:2, 5, "general_profile_idc");
  /* Compatibility flags should be set at general_profile_idc
   *  (so with general_profile_idc = 1, compatibility_flag[1] should be 1)
   * According to specification, when compatibility_flag[1] is set,
   *  compatibility_flag[2] should be set too.
   */
  WRITE_U(stream, 3<<29, 32, "general_profile_compatibility_flag[]");

  WRITE_U(stream, 1, 1, "general_progressive_source_flag");
  WRITE_U(stream, state->encoder_control->in.source_scan_type!= 0, 1, "general_interlaced_source_flag");
  WRITE_U(stream, 0, 1, "general_non_packed_constraint_flag");
  WRITE_U(stream, 0, 1, "general_frame_only_constraint_flag");

  WRITE_U(stream, 0, 32, "XXX_reserved_zero_44bits[0..31]");
  WRITE_U(stream, 0, 12, "XXX_reserved_zero_44bits[32..43]");

  // end Profile Tier

  // Level 6.2 (general_level_idc is 30 * 6.2)
  WRITE_U(stream, 186, 8, "general_level_idc");

  WRITE_U(stream, 0, 1, "sub_layer_profile_present_flag");
  WRITE_U(stream, 0, 1, "sub_layer_level_present_flag");

  for (int i = 1; i < 8; i++) {
    WRITE_U(stream, 0, 2, "reserved_zero_2bits");
  }

  // end PTL
}

static void encoder_state_write_bitstream_vid_parameter_set(bitstream_t* stream,
                                                            encoder_state_t * const state)
{
#ifdef KVZ_DEBUG
  printf("=========== Video Parameter Set ID: 0 ===========\n");
#endif

  WRITE_U(stream, 0, 4, "vps_video_parameter_set_id");
  WRITE_U(stream, 3, 2, "vps_reserved_three_2bits" );
  WRITE_U(stream, 0, 6, "vps_reserved_zero_6bits" );
  WRITE_U(stream, 1, 3, "vps_max_sub_layers_minus1");
  WRITE_U(stream, 0, 1, "vps_temporal_id_nesting_flag");
  WRITE_U(stream, 0xffff, 16, "vps_reserved_ffff_16bits");

  encoder_state_write_bitstream_PTL(stream, state);

  WRITE_U(stream, 0, 1, "vps_sub_layer_ordering_info_present_flag");

  //for each layer
  for (int i = 0; i < 1; i++) {
  WRITE_UE(stream, 1, "vps_max_dec_pic_buffering");
  WRITE_UE(stream, 0, "vps_num_reorder_pics");
  WRITE_UE(stream, 0, "vps_max_latency_increase");
  }

  WRITE_U(stream, 0, 6, "vps_max_nuh_reserved_zero_layer_id");
  WRITE_UE(stream, 0, "vps_max_op_sets_minus1");
  WRITE_U(stream, 0, 1, "vps_timing_info_present_flag");

  //IF timing info
  //END IF

  WRITE_U(stream, 0, 1, "vps_extension_flag");

  kvz_bitstream_add_rbsp_trailing_bits(stream);
}

static void encoder_state_write_bitstream_scaling_list(bitstream_t *stream,
                                                       encoder_state_t * const state)
{
  const encoder_control_t * const encoder = state->encoder_control;
  uint32_t size_id;
  for (size_id = 0; size_id < SCALING_LIST_SIZE_NUM; size_id++) {
    int32_t list_id;
    for (list_id = 0; list_id < kvz_g_scaling_list_num[size_id]; list_id++) {
      uint8_t scaling_list_pred_mode_flag = 1;
      int32_t pred_list_idx;
      int32_t i;
      uint32_t ref_matrix_id = UINT32_MAX;

      for (pred_list_idx = list_id; pred_list_idx >= 0; pred_list_idx--) {
        const int32_t * const pred_list  = (list_id == pred_list_idx) ?
                                     kvz_scalinglist_get_default(size_id, pred_list_idx) :
                                     encoder->scaling_list.scaling_list_coeff[size_id][pred_list_idx];

        if (!memcmp(encoder->scaling_list.scaling_list_coeff[size_id][list_id], pred_list, sizeof(int32_t) * MIN(8, kvz_g_scaling_list_size[size_id])) &&
            ((size_id < SCALING_LIST_16x16) ||
             (encoder->scaling_list.scaling_list_dc[size_id][list_id] == encoder->scaling_list.scaling_list_dc[size_id][pred_list_idx]))) {
          ref_matrix_id = pred_list_idx;
          scaling_list_pred_mode_flag = 0;
          break;
        }
      }
      WRITE_U(stream, scaling_list_pred_mode_flag, 1, "scaling_list_pred_mode_flag" );

      if (!scaling_list_pred_mode_flag) {
        WRITE_UE(stream, list_id - ref_matrix_id, "scaling_list_pred_matrix_id_delta");
      } else {
        int32_t delta;
        const int32_t coef_num = MIN(MAX_MATRIX_COEF_NUM, kvz_g_scaling_list_size[size_id]);
        const uint32_t * const scan_cg = (size_id == 0) ? g_sig_last_scan_16x16 : g_sig_last_scan_32x32;
        int32_t next_coef = 8;
        const int32_t * const coef_list = encoder->scaling_list.scaling_list_coeff[size_id][list_id];

        if (size_id >= SCALING_LIST_16x16) {
          WRITE_SE(stream, encoder->scaling_list.scaling_list_dc[size_id][list_id] - 8, "scaling_list_dc_coef_minus8");
          next_coef = encoder->scaling_list.scaling_list_dc[size_id][list_id];
        }

        for (i = 0; i < coef_num; i++) {
          delta     = coef_list[scan_cg[i]] - next_coef;
          next_coef = coef_list[scan_cg[i]];
          if (delta > 127)
            delta -= 256;
          if (delta < -128)
            delta += 256;

          WRITE_SE(stream, delta, "scaling_list_delta_coef");
        }
      }
    }
  }
}


static void encoder_state_write_bitstream_VUI(bitstream_t *stream,
                                              encoder_state_t * const state)
{
  const encoder_control_t * const encoder = state->encoder_control;
#ifdef KVZ_DEBUG
  printf("=========== VUI Set ID: 0 ===========\n");
#endif
  if (encoder->cfg.vui.sar_width > 0 && encoder->cfg.vui.sar_height > 0) {
    int i;
    static const struct
    {
      uint8_t width;
      uint8_t height;
      uint8_t idc;
    } sar[] = {
      // aspect_ratio_idc = 0 -> unspecified
      {  1,  1, 1 }, { 12, 11, 2 }, { 10, 11, 3 }, { 16, 11, 4 },
      { 40, 33, 5 }, { 24, 11, 6 }, { 20, 11, 7 }, { 32, 11, 8 },
      { 80, 33, 9 }, { 18, 11, 10}, { 15, 11, 11}, { 64, 33, 12},
      {160, 99, 13}, {  4,  3, 14}, {  3,  2, 15}, {  2,  1, 16},
      // aspect_ratio_idc = [17..254] -> reserved
      { 0, 0, 255 }
    };

    for (i = 0; sar[i].idc != 255; i++)
      if (sar[i].width  == encoder->cfg.vui.sar_width &&
          sar[i].height == encoder->cfg.vui.sar_height)
        break;

    WRITE_U(stream, 1, 1, "aspect_ratio_info_present_flag");
    WRITE_U(stream, sar[i].idc, 8, "aspect_ratio_idc");
    if (sar[i].idc == 255) {
      // EXTENDED_SAR
      WRITE_U(stream, encoder->cfg.vui.sar_width, 16, "sar_width");
      WRITE_U(stream, encoder->cfg.vui.sar_height, 16, "sar_height");
    }
  } else
    WRITE_U(stream, 0, 1, "aspect_ratio_info_present_flag");

  //IF aspect ratio info
  //ENDIF

  if (encoder->cfg.vui.overscan > 0) {
    WRITE_U(stream, 1, 1, "overscan_info_present_flag");
    WRITE_U(stream, encoder->cfg.vui.overscan - 1, 1, "overscan_appropriate_flag");
  } else
    WRITE_U(stream, 0, 1, "overscan_info_present_flag");

  //IF overscan info
  //ENDIF

  if (encoder->cfg.vui.videoformat != 5 ||
      encoder->cfg.vui.fullrange   != 0 ||
      encoder->cfg.vui.colorprim   != 2 ||
      encoder->cfg.vui.transfer    != 2 ||
      encoder->cfg.vui.colormatrix != 2) {
    WRITE_U(stream, 1, 1, "video_signal_type_present_flag");
    WRITE_U(stream, encoder->cfg.vui.videoformat, 3, "chroma_format");
    WRITE_U(stream, encoder->cfg.vui.fullrange, 1, "video_full_range_flag");

    if (encoder->cfg.vui.colorprim   != 2 ||
        encoder->cfg.vui.transfer    != 2 ||
        encoder->cfg.vui.colormatrix != 2) {
      WRITE_U(stream, 1, 1, "colour_description_present_flag");
      WRITE_U(stream, encoder->cfg.vui.colorprim, 8, "colour_primaries");
      WRITE_U(stream, encoder->cfg.vui.transfer, 8, "transfer_characteristics");
      WRITE_U(stream, encoder->cfg.vui.colormatrix, 8, "matrix_coeffs");
    } else
      WRITE_U(stream, 0, 1, "colour_description_present_flag");
  } else
    WRITE_U(stream, 0, 1, "video_signal_type_present_flag");

  //IF video type
  //ENDIF

  if (encoder->cfg.vui.chroma_loc > 0) {
    WRITE_U(stream, 1, 1, "chroma_loc_info_present_flag");
    WRITE_UE(stream, encoder->cfg.vui.chroma_loc, "chroma_sample_loc_type_top_field");
    WRITE_UE(stream, encoder->cfg.vui.chroma_loc, "chroma_sample_loc_type_bottom_field");
  } else
    WRITE_U(stream, 0, 1, "chroma_loc_info_present_flag");

  //IF chroma loc info
  //ENDIF

  WRITE_U(stream, 0, 1, "neutral_chroma_indication_flag");
  WRITE_U(stream, encoder->vui.field_seq_flag, 1, "field_seq_flag"); // 0: frames, 1: fields
  WRITE_U(stream, encoder->vui.frame_field_info_present_flag, 1, "frame_field_info_present_flag");
  WRITE_U(stream, 0, 1, "default_display_window_flag");

  //IF default display window
  //ENDIF

  WRITE_U(stream, encoder->vui.timing_info_present_flag, 1, "vui_timing_info_present_flag");
  if (encoder->vui.timing_info_present_flag) {
    WRITE_U(stream, encoder->vui.num_units_in_tick, 32, "vui_num_units_in_tick");
    WRITE_U(stream, encoder->vui.time_scale, 32, "vui_time_scale");

    WRITE_U(stream, 0, 1, "vui_poc_proportional_to_timing_flag");
    WRITE_U(stream, 0, 1, "vui_hrd_parameters_present_flag");    
  }
  
  WRITE_U(stream, 0, 1, "bitstream_restriction_flag");

  //IF bitstream restriction
  //ENDIF
}


static void encoder_state_write_bitstream_SPS_extension(bitstream_t *stream,
                                                        encoder_state_t * const state)
{
  const kvz_config *cfg = &state->encoder_control->cfg;
  if (cfg->implicit_rdpcm && cfg->lossless) {
    WRITE_U(stream, 1, 1, "sps_extension_present_flag");

    WRITE_U(stream, 1, 1, "sps_range_extension_flag");
    WRITE_U(stream, 0, 1, "sps_multilayer_extension_flag");
    WRITE_U(stream, 0, 1, "sps_3d_extension_flag");
    WRITE_U(stream, 0, 5, "sps_extension_5bits");

    WRITE_U(stream, 0, 1, "transform_skip_rotation_enabled_flag");
    WRITE_U(stream, 0, 1, "transform_skip_context_enabled_flag");
    WRITE_U(stream, 1, 1, "implicit_rdpcm_enabled_flag");
    WRITE_U(stream, 0, 1, "explicit_rdpcm_enabled_flag");
    WRITE_U(stream, 0, 1, "extended_precision_processing_flag");
    WRITE_U(stream, 0, 1, "intra_smoothing_disabled_flag");
    WRITE_U(stream, 0, 1, "high_precision_offsets_enabled_flag");
    WRITE_U(stream, 0, 1, "persistent_rice_adaptation_enabled_flag");
    WRITE_U(stream, 0, 1, "cabac_bypass_alignment_enabled_flag");
  } else {
    WRITE_U(stream, 0, 1, "sps_extension_present_flag");
  }
}

static void encoder_state_write_bitstream_seq_parameter_set(bitstream_t* stream,
                                                            encoder_state_t * const state)
{
  const encoder_control_t * encoder = state->encoder_control;

#ifdef KVZ_DEBUG
  printf("=========== Sequence Parameter Set ID: 0 ===========\n");
#endif

  // TODO: profile IDC and level IDC should be defined later on
  WRITE_U(stream, 0, 4, "sps_video_parameter_set_id");
  WRITE_U(stream, 1, 3, "sps_max_sub_layers_minus1");
  WRITE_U(stream, 0, 1, "sps_temporal_id_nesting_flag");

  encoder_state_write_bitstream_PTL(stream, state);

  WRITE_UE(stream, 0, "sps_seq_parameter_set_id");
  WRITE_UE(stream, encoder->chroma_format, "chroma_format_idc");

  if (encoder->chroma_format == KVZ_CSP_444) {
    WRITE_U(stream, 0, 1, "separate_colour_plane_flag");
  }

  WRITE_UE(stream, encoder->in.width, "pic_width_in_luma_samples");
  WRITE_UE(stream, encoder->in.height, "pic_height_in_luma_samples");

  if (encoder->in.width != encoder->in.real_width || encoder->in.height != encoder->in.real_height) {
    // The standard does not seem to allow setting conf_win values such that
    // the number of luma samples is not a multiple of 2. Options are to either
    // hide one line or show an extra line of non-video. Neither seems like a
    // very good option, so let's not even try.
    assert(!(encoder->in.width % 2));
    WRITE_U(stream, 1, 1, "conformance_window_flag");
    WRITE_UE(stream, 0, "conf_win_left_offset");
    WRITE_UE(stream, (encoder->in.width - encoder->in.real_width) >> 1,
             "conf_win_right_offset");
    WRITE_UE(stream, 0, "conf_win_top_offset");
    WRITE_UE(stream, (encoder->in.height - encoder->in.real_height) >> 1,
             "conf_win_bottom_offset");
  } else {
    WRITE_U(stream, 0, 1, "conformance_window_flag");
  }

  //IF window flag
  //END IF

  WRITE_UE(stream, encoder->bitdepth-8, "bit_depth_luma_minus8");
  WRITE_UE(stream, encoder->bitdepth-8, "bit_depth_chroma_minus8");
  WRITE_UE(stream, 1, "log2_max_pic_order_cnt_lsb_minus4");
  WRITE_U(stream, 0, 1, "sps_sub_layer_ordering_info_present_flag");

  //for each layer
  if (encoder->cfg.gop_lowdelay) {
    WRITE_UE(stream, encoder->cfg.ref_frames, "sps_max_dec_pic_buffering");
    WRITE_UE(stream, 0, "sps_num_reorder_pics");
  } else {
    WRITE_UE(stream, encoder->cfg.ref_frames + encoder->cfg.gop_len, "sps_max_dec_pic_buffering");
    WRITE_UE(stream, encoder->cfg.gop_len, "sps_num_reorder_pics");
  }
  WRITE_UE(stream, 0, "sps_max_latency_increase");
  //end for

  WRITE_UE(stream, MIN_SIZE-3, "log2_min_coding_block_size_minus3");
  WRITE_UE(stream, MAX_DEPTH, "log2_diff_max_min_coding_block_size");
  WRITE_UE(stream, 0, "log2_min_transform_block_size_minus2");   // 4x4
  WRITE_UE(stream, 3, "log2_diff_max_min_transform_block_size"); // 4x4...32x32
  WRITE_UE(stream, encoder->tr_depth_inter, "max_transform_hierarchy_depth_inter");
  WRITE_UE(stream, encoder->cfg.tr_depth_intra, "max_transform_hierarchy_depth_intra");

  // scaling list
  WRITE_U(stream, encoder->scaling_list.enable, 1, "scaling_list_enable_flag");
  if (encoder->scaling_list.enable) {
    WRITE_U(stream, 1, 1, "sps_scaling_list_data_present_flag");
    encoder_state_write_bitstream_scaling_list(stream, state);
  }

  WRITE_U(stream, (encoder->cfg.amp_enable ? 1 : 0), 1, "amp_enabled_flag");

  WRITE_U(stream, encoder->cfg.sao_type ? 1 : 0, 1,
          "sample_adaptive_offset_enabled_flag");
  WRITE_U(stream, ENABLE_PCM, 1, "pcm_enabled_flag");
  #if ENABLE_PCM == 1
    WRITE_U(stream, 7, 4, "pcm_sample_bit_depth_luma_minus1");
    WRITE_U(stream, 7, 4, "pcm_sample_bit_depth_chroma_minus1");
    WRITE_UE(stream, 0, "log2_min_pcm_coding_block_size_minus3");
    WRITE_UE(stream, 2, "log2_diff_max_min_pcm_coding_block_size");
    WRITE_U(stream, 1, 1, "pcm_loop_filter_disable_flag");
  #endif

  WRITE_UE(stream, 0, "num_short_term_ref_pic_sets");

  //IF num short term ref pic sets
  //ENDIF

  WRITE_U(stream, 0, 1, "long_term_ref_pics_present_flag");

  //IF long_term_ref_pics_present
  //ENDIF

  WRITE_U(stream, state->encoder_control->cfg.tmvp_enable, 1,
          "sps_temporal_mvp_enable_flag");
  WRITE_U(stream, 0, 1, "sps_strong_intra_smoothing_enable_flag");
  WRITE_U(stream, 1, 1, "vui_parameters_present_flag");

  encoder_state_write_bitstream_VUI(stream, state);

  encoder_state_write_bitstream_SPS_extension(stream, state);

  kvz_bitstream_add_rbsp_trailing_bits(stream);
}

static void encoder_state_write_bitstream_pic_parameter_set(bitstream_t* stream,
                                                            encoder_state_t * const state)
{
  const encoder_control_t * const encoder = state->encoder_control;
#ifdef KVZ_DEBUG
  printf("=========== Picture Parameter Set ID: 0 ===========\n");
#endif
  WRITE_UE(stream, 0, "pic_parameter_set_id");
  WRITE_UE(stream, 0, "seq_parameter_set_id");
  WRITE_U(stream, encoder->pps.dependent_slice_segments_enabled_flag, 1, "dependent_slice_segments_enabled_flag");
  WRITE_U(stream, 0, 1, "output_flag_present_flag");
  WRITE_U(stream, 0, 3, "num_extra_slice_header_bits");
  WRITE_U(stream, encoder->cfg.signhide_enable, 1, "sign_data_hiding_flag");
  WRITE_U(stream, 0, 1, "cabac_init_present_flag");

  WRITE_UE(stream, 0, "num_ref_idx_l0_default_active_minus1");
  WRITE_UE(stream, 0, "num_ref_idx_l1_default_active_minus1");
  WRITE_SE(stream, ((int8_t)encoder->cfg.qp) - 26, "pic_init_qp_minus26");
  WRITE_U(stream, 0, 1, "constrained_intra_pred_flag");
  WRITE_U(stream, encoder->cfg.trskip_enable, 1, "transform_skip_enabled_flag");

  if (encoder->lcu_dqp_enabled) {
    // Use separate QP for each LCU when rate control is enabled.
    WRITE_U(stream, 1, 1, "cu_qp_delta_enabled_flag");
    WRITE_UE(stream, 0, "diff_cu_qp_delta_depth");
  } else {
  WRITE_U(stream, 0, 1, "cu_qp_delta_enabled_flag");
  }

  //TODO: add QP offsets
  WRITE_SE(stream, 0, "pps_cb_qp_offset");
  WRITE_SE(stream, 0, "pps_cr_qp_offset");
  WRITE_U(stream, 0, 1, "pps_slice_chroma_qp_offsets_present_flag");
  WRITE_U(stream, 0, 1, "weighted_pred_flag");
  WRITE_U(stream, 0, 1, "weighted_bipred_idc");

  //WRITE_U(stream, 0, 1, "dependent_slices_enabled_flag");
  WRITE_U(stream, encoder->cfg.lossless, 1, "transquant_bypass_enable_flag");
  WRITE_U(stream, encoder->tiles_enable, 1, "tiles_enabled_flag");
  //wavefronts
  WRITE_U(stream, encoder->cfg.wpp, 1, "entropy_coding_sync_enabled_flag");

  if (encoder->tiles_enable) {
    WRITE_UE(stream, encoder->cfg.tiles_width_count  - 1, "num_tile_columns_minus1");
    WRITE_UE(stream, encoder->cfg.tiles_height_count - 1, "num_tile_rows_minus1");
    
    WRITE_U(stream, encoder->tiles_uniform_spacing_flag, 1, "uniform_spacing_flag");
    
    if (!encoder->tiles_uniform_spacing_flag) {
      int i;
      for (i = 0; i < encoder->cfg.tiles_width_count - 1; ++i) {
        WRITE_UE(stream, encoder->tiles_col_width[i] - 1, "column_width_minus1[...]");
      }
      for (i = 0; i < encoder->cfg.tiles_height_count - 1; ++i) {
        WRITE_UE(stream, encoder->tiles_row_height[i] - 1, "row_height_minus1[...]");
      }
    }
    WRITE_U(stream, 0, 1, "loop_filter_across_tiles_enabled_flag");
    
  }
  
  WRITE_U(stream, 0, 1, "loop_filter_across_slice_flag");
  WRITE_U(stream, 1, 1, "deblocking_filter_control_present_flag");

  //IF deblocking_filter
    WRITE_U(stream, 0, 1, "deblocking_filter_override_enabled_flag");
  WRITE_U(stream, encoder->cfg.deblock_enable ? 0 : 1, 1,
          "pps_disable_deblocking_filter_flag");

    //IF !disabled
  if (encoder->cfg.deblock_enable) {
     WRITE_SE(stream, encoder->cfg.deblock_beta, "beta_offset_div2");
     WRITE_SE(stream, encoder->cfg.deblock_tc, "tc_offset_div2");
    }

    //ENDIF
  //ENDIF
  WRITE_U(stream, 0, 1, "pps_scaling_list_data_present_flag");
  //IF scaling_list
  //ENDIF
  WRITE_U(stream, 0, 1, "lists_modification_present_flag");
  WRITE_UE(stream, 0, "log2_parallel_merge_level_minus2");
  WRITE_U(stream, 0, 1, "slice_segment_header_extension_present_flag");
  WRITE_U(stream, 0, 1, "pps_extension_flag");

  kvz_bitstream_add_rbsp_trailing_bits(stream);
}

static void encoder_state_write_bitstream_prefix_sei_version(encoder_state_t * const state)
{
#define STR_BUF_LEN 1000
  bitstream_t * const stream = &state->stream;
  int i, length;
  char buf[STR_BUF_LEN] = { 0 };
  char *s = buf + 16;
  const kvz_config * const cfg = &state->encoder_control->cfg;

  // random uuid_iso_iec_11578 generated with www.famkruithof.net/uuid/uuidgen
  static const uint8_t uuid[16] = {
    0x32, 0xfe, 0x46, 0x6c, 0x98, 0x41, 0x42, 0x69,
    0xae, 0x35, 0x6a, 0x91, 0x54, 0x9e, 0xf3, 0xf1
  };
  memcpy(buf, uuid, 16);

  // user_data_payload_byte
  s += sprintf(s, "Kvazaar HEVC Encoder v. " VERSION_STRING " - "
                  "Copyleft 2012-2015 - http://ultravideo.cs.tut.fi/ - options:");
  s += sprintf(s, " %dx%d", cfg->width, cfg->height);
  s += sprintf(s, " deblock=%d:%d:%d", cfg->deblock_enable,
               cfg->deblock_beta, cfg->deblock_tc);
  s += sprintf(s, " sao=%d", cfg->sao_type);
  s += sprintf(s, " intra_period=%d", cfg->intra_period);
  s += sprintf(s, " qp=%d", cfg->qp);
  s += sprintf(s, " ref=%d", cfg->ref_frames);

  length = (int)(s - buf + 1);  // length, +1 for \0

  // Assert this so that in the future if the message gets longer, we remember
  // to increase the buf len. Divide by 2 for margin.
  assert(length < STR_BUF_LEN / 2);

  // payloadType = 5 -> user_data_unregistered
  WRITE_U(stream, 5, 8, "last_payload_type_byte");

  // payloadSize
  for (i = 0; i <= length - 255; i += 255)
    WRITE_U(stream, 255, 8, "ff_byte");
  WRITE_U(stream, length - i, 8, "last_payload_size_byte");

  for (i = 0; i < length; i++)
    WRITE_U(stream, ((uint8_t *)buf)[i], 8, "sei_payload");

  // The bitstream is already aligned, but align it anyway.
  kvz_bitstream_align(stream);

#undef STR_BUF_LEN
}

/*
static void encoder_state_write_active_parameter_sets_sei_message(encoder_state_t * const state) {

  const encoder_control_t * const encoder = state->encoder_control;
  bitstream_t * const stream = &state->stream;

  int i = 0;

  int active_vps_id = 0;
  int self_contained_cvs_flag = 0;
  int no_parameter_set_update_flag = 0;
  int num_sps_ids_minus1 = 0;
  int layer_sps_idx = 0;
  int active_seq_parameter_set_id = 0;
  int vps_base_layer_internal_flag = 0;

  int max_layers_minus1 = 0;

  WRITE_U(stream, 129, 8, "last_payload_type_byte"); //active_parameter_sets
  WRITE_U(stream, 2, 8, "last_payload_size_byte");
  WRITE_U(stream, active_vps_id, 4, "active_video_parameter_set_id");
  WRITE_U(stream, self_contained_cvs_flag, 1, "self_contained_cvs_flag");
  WRITE_U(stream, no_parameter_set_update_flag, 1, "no_parameter_set_update_flag");
  WRITE_UE(stream, num_sps_ids_minus1, "num_sps_ids_minus1");
  //for (i = 0; i <= num_sps_ids_minus1; ++i) {
  WRITE_UE(stream, active_seq_parameter_set_id, "active_seq_parameter_set_id");
  //}
  // for (i = vps_base_layer_internal_flag; i <= max_layers_minus1; ++i){
  WRITE_UE(stream, layer_sps_idx, "layer_sps_idx");
  //}

  kvz_bitstream_rbsp_trailing_bits(stream); //rbsp_trailing_bits
}
*/

static void encoder_state_write_picture_timing_sei_message(encoder_state_t * const state) {

  bitstream_t * const stream = &state->stream;

  if (state->encoder_control->vui.frame_field_info_present_flag){

    int8_t odd_picture = state->frame->num % 2;
    int8_t pic_struct = 0; //0: progressive picture, 1: top field, 2: bottom field, 3...
    int8_t source_scan_type = 1; //0: interlaced, 1: progressive

    switch (state->tile->frame->source->interlacing){
    case 0: //Progressive frame
      pic_struct = 0;
      source_scan_type = 1;
      break;
    case 1: //Top field first
      pic_struct = odd_picture ? 2 : 1;
      source_scan_type = 0;
      break;
    case 2: //Bottom field first
      pic_struct = odd_picture ? 1 : 2;
      source_scan_type = 0;
      break;
    default:
      assert(0); //Should never execute
      break;
    }

    WRITE_U(stream, 1, 8, "last_payload_type_byte"); //pic_timing
    WRITE_U(stream, 1, 8, "last_payload_size_byte");
    WRITE_U(stream, pic_struct, 4, "pic_struct");
    WRITE_U(stream, source_scan_type, 2, "source_scan_type");
    WRITE_U(stream, 0, 1, "duplicate_flag");

    kvz_bitstream_align(stream);
  }
}


static void encoder_state_entry_points_explore(const encoder_state_t * const state, int * const r_count, int * const r_max_length) {
  int i;
  for (i = 0; state->children[i].encoder_control; ++i) {
    if (state->children[i].is_leaf) {
      const int my_length = kvz_bitstream_tell(&state->children[i].stream)/8;
      ++(*r_count);
      if (my_length > *r_max_length) {
        *r_max_length = my_length;
      }
    } else {
      encoder_state_entry_points_explore(&state->children[i], r_count, r_max_length);
    }
  }
}

static void encoder_state_write_bitstream_entry_points_write(bitstream_t * const stream, const encoder_state_t * const state, const int num_entry_points, const int write_length, int * const r_count) {
  int i;
  for (i = 0; state->children[i].encoder_control; ++i) {
    if (state->children[i].is_leaf) {
      const int my_length = kvz_bitstream_tell(&state->children[i].stream)/8;
      ++(*r_count);
      //Don't write the last one
      if (*r_count < num_entry_points) {
        WRITE_U(stream, my_length - 1, write_length, "entry_point_offset-minus1")
      }
    } else {
      encoder_state_write_bitstream_entry_points_write(stream, &state->children[i], num_entry_points, write_length, r_count);
    }
  }
}

static void kvz_encoder_state_write_bitstream_slice_header_independent(
    struct bitstream_t * const stream,
    struct encoder_state_t * const state)
{
  const encoder_control_t * const encoder = state->encoder_control;

  int j;
  int ref_negative = 0;
  int ref_positive = 0;
  if (encoder->cfg.gop_len) {
    for (j = 0; j < state->frame->ref->used_size; j++) {
      if (state->frame->ref->pocs[j] < state->frame->poc) {
        ref_negative++;
      } else {
        ref_positive++;
      }
    }
  } else ref_negative = state->frame->ref->used_size;

  WRITE_UE(stream, state->frame->slicetype, "slice_type");

  if (state->frame->pictype != KVZ_NAL_IDR_W_RADL
      && state->frame->pictype != KVZ_NAL_IDR_N_LP)
  {
    int last_poc = 0;
    int poc_shift = 0;

      WRITE_U(stream, state->frame->poc&0x1f, 5, "pic_order_cnt_lsb");
      WRITE_U(stream, 0, 1, "short_term_ref_pic_set_sps_flag");
      WRITE_UE(stream, ref_negative, "num_negative_pics");
      WRITE_UE(stream, ref_positive, "num_positive_pics");
    for (j = 0; j < ref_negative; j++) {      
      int8_t delta_poc = 0;
      
      if (encoder->cfg.gop_len) {
        int8_t found = 0;
        do {
          delta_poc = encoder->cfg.gop[state->frame->gop_offset].ref_neg[j + poc_shift];
          for (int i = 0; i < state->frame->ref->used_size; i++) {
            if (state->frame->ref->pocs[i] == state->frame->poc - delta_poc) {
              found = 1;
              break;
            }
          }
          if (!found) poc_shift++;
          if (j + poc_shift == ref_negative) {
            fprintf(stderr, "Failure, reference not found!");
            exit(EXIT_FAILURE);
          }
        } while (!found);
      }

      WRITE_UE(stream, encoder->cfg.gop_len?delta_poc - last_poc - 1:0, "delta_poc_s0_minus1");
      last_poc = delta_poc;
      WRITE_U(stream, !state->frame->is_irap, 1, "used_by_curr_pic_s0_flag");
    }
    last_poc = 0;
    poc_shift = 0;
    for (j = 0; j < ref_positive; j++) {      
      int8_t delta_poc = 0;
      
      if (encoder->cfg.gop_len) {
        int8_t found = 0;
        do {
          delta_poc = encoder->cfg.gop[state->frame->gop_offset].ref_pos[j + poc_shift];
          for (int i = 0; i < state->frame->ref->used_size; i++) {
            if (state->frame->ref->pocs[i] == state->frame->poc + delta_poc) {
              found = 1;
              break;
            }
          }
          if (!found) poc_shift++;
          if (j + poc_shift == ref_positive) {
            fprintf(stderr, "Failure, reference not found!");
            exit(EXIT_FAILURE);
          }
        } while (!found);
      }
      
      WRITE_UE(stream, encoder->cfg.gop_len ? delta_poc - last_poc - 1 : 0, "delta_poc_s1_minus1");
      last_poc = delta_poc;
      WRITE_U(stream, !state->frame->is_irap, 1, "used_by_curr_pic_s1_flag");
    }
    //WRITE_UE(stream, 0, "short_term_ref_pic_set_idx");
    
    if (state->encoder_control->cfg.tmvp_enable) {
      WRITE_U(stream, ref_negative ? 1 : 0, 1, "slice_temporal_mvp_enabled_flag");
    }
  }

    //end if
  //end if


  if (encoder->cfg.sao_type) {
    WRITE_U(stream, 1, 1, "slice_sao_luma_flag");
    if (encoder->chroma_format != KVZ_CSP_400) {
      WRITE_U(stream, 1, 1, "slice_sao_chroma_flag");
    }
  }
    
  if (state->frame->slicetype != KVZ_SLICE_I) {
      WRITE_U(stream, 1, 1, "num_ref_idx_active_override_flag");
      WRITE_UE(stream, ref_negative != 0 ? ref_negative - 1: 0, "num_ref_idx_l0_active_minus1");
      if (state->frame->slicetype == KVZ_SLICE_B) {
        WRITE_UE(stream, ref_positive != 0 ? ref_positive - 1 : 0, "num_ref_idx_l1_active_minus1");
        WRITE_U(stream, 0, 1, "mvd_l1_zero_flag");
      }

      // Temporal Motion Vector Prediction flags
      if (state->encoder_control->cfg.tmvp_enable && ref_negative > 0) {
        if (state->frame->slicetype == KVZ_SLICE_B) {
          // Always use L0 for prediction
          WRITE_U(stream, 1, 1, "collocated_from_l0_flag");
        }

        if (ref_negative > 1) {
          // Use first reference from L0
          // ToDo: use better reference
          WRITE_UE(stream, 0, "collocated_ref_idx");
        }
      }

      WRITE_UE(stream, 5-MRG_MAX_NUM_CANDS, "five_minus_max_num_merge_cand");
  }

  {
    int slice_qp_delta = state->frame->QP - encoder->cfg.qp;
    WRITE_SE(stream, slice_qp_delta, "slice_qp_delta");
  }
}

void kvz_encoder_state_write_bitstream_slice_header(
    struct bitstream_t * const stream,
    struct encoder_state_t * const state,
    bool independent)
{
  const encoder_control_t * const encoder = state->encoder_control;

#ifdef KVZ_DEBUG
  printf("=========== Slice ===========\n");
#endif

  bool first_slice_segment_in_pic = (state->slice->start_in_rs == 0);
  if ((state->encoder_control->cfg.slices & KVZ_SLICES_WPP)
      && state->wfrow->lcu_offset_y > 0)
  {
    first_slice_segment_in_pic = false;
  }

  WRITE_U(stream, first_slice_segment_in_pic, 1, "first_slice_segment_in_pic_flag");

  if (state->frame->pictype >= KVZ_NAL_BLA_W_LP
    && state->frame->pictype <= KVZ_NAL_RSV_IRAP_VCL23) {
    WRITE_U(stream, 0, 1, "no_output_of_prior_pics_flag");
  }

  WRITE_UE(stream, 0, "slice_pic_parameter_set_id");

  if (!first_slice_segment_in_pic) {
    if (encoder->pps.dependent_slice_segments_enabled_flag) {
      WRITE_U(stream, !independent, 1, "dependent_slice_segment_flag");
    }

    int lcu_cnt = encoder->in.width_in_lcu * encoder->in.height_in_lcu;
    int num_bits = kvz_math_ceil_log2(lcu_cnt);
    int slice_start_rs = state->slice->start_in_rs;
    if (state->encoder_control->cfg.slices & KVZ_SLICES_WPP) {
      slice_start_rs += state->wfrow->lcu_offset_y * state->tile->frame->width_in_lcu;
    }
    WRITE_U(stream, slice_start_rs, num_bits, "slice_segment_address");
  }

  if (independent) {
    kvz_encoder_state_write_bitstream_slice_header_independent(stream, state);
  }
   
  if (encoder->tiles_enable || encoder->cfg.wpp) {
    int num_entry_points = 0;
    int max_length_seen = 0;
    
    if (state->is_leaf) {
      num_entry_points = 1;
    } else {
    encoder_state_entry_points_explore(state, &num_entry_points, &max_length_seen);
    }
    
    int num_offsets = num_entry_points - 1;

    WRITE_UE(stream, num_offsets, "num_entry_point_offsets");
    if (num_offsets > 0) {
      int entry_points_written = 0;
      int offset_len = kvz_math_floor_log2(max_length_seen) + 1;
      WRITE_UE(stream, offset_len - 1, "offset_len_minus1");
      encoder_state_write_bitstream_entry_points_write(stream, state, num_entry_points, offset_len, &entry_points_written); 
    }
  }
}


/**
 * \brief Add a checksum SEI message to the bitstream.
 * \param encoder The encoder.
 * \returns Void
 */
static void add_checksum(encoder_state_t * const state)
{
  bitstream_t * const stream = &state->stream;
  const videoframe_t * const frame = state->tile->frame;
  unsigned char checksum[3][SEI_HASH_MAX_LENGTH];

  kvz_nal_write(stream, KVZ_NAL_SUFFIX_SEI_NUT, 0, 0);

  WRITE_U(stream, 132, 8, "sei_type");

  int num_colors = (state->encoder_control->chroma_format == KVZ_CSP_400 ? 1 : 3);

  switch (state->encoder_control->cfg.hash)
  {
  case KVZ_HASH_CHECKSUM:
    kvz_image_checksum(frame->rec, checksum, state->encoder_control->bitdepth);

    WRITE_U(stream, 1 + num_colors * 4, 8, "size");
    WRITE_U(stream, 2, 8, "hash_type");  // 2 = checksum

    for (int i = 0; i < num_colors; ++i) {
      uint32_t checksum_val = (
        (checksum[i][0] << 24) + (checksum[i][1] << 16) +
        (checksum[i][2] << 8) + (checksum[i][3]));
      WRITE_U(stream, checksum_val, 32, "picture_checksum");
      CHECKPOINT("checksum[%d] = %u", i, checksum_val);
    }

    break;

  case KVZ_HASH_MD5:
    kvz_image_md5(frame->rec, checksum, state->encoder_control->bitdepth);

    WRITE_U(stream, 1 + num_colors * 16, 8, "size");
    WRITE_U(stream, 0, 8, "hash_type");  // 0 = md5

    for (int i = 0; i < num_colors; ++i) {
      for (int b = 0; b < 16; ++b) {
        WRITE_U(stream, checksum[i][b], 8, "picture_md5");
      }
    }

    break;

  case KVZ_HASH_NONE:
    // Means we shouldn't be writing this SEI.
    assert(0);
  }

  kvz_bitstream_align(stream);

  // spec:sei_rbsp() rbsp_trailing_bits
  kvz_bitstream_add_rbsp_trailing_bits(stream);
}

static void encoder_state_write_slice_header(
  bitstream_t * stream,
  encoder_state_t * state,
  bool independent)
{
  kvz_nal_write(stream, state->frame->pictype, 0, state->frame->first_nal);
  state->frame->first_nal = false;

  kvz_encoder_state_write_bitstream_slice_header(stream, state, independent);
  kvz_bitstream_add_rbsp_trailing_bits(stream);
}

/**
 * \brief Move child state bitstreams to the parent stream.
 */
static void encoder_state_write_bitstream_children(encoder_state_t * const state)
{
  // Write Slice headers to the parent stream instead of the child stream
  // in case the child stream is a leaf with something in it already.
  for (int i = 0; state->children[i].encoder_control; ++i) {
    if (state->children[i].type == ENCODER_STATE_TYPE_SLICE) {
      encoder_state_write_slice_header(&state->stream, &state->children[i], true);
    } else if (state->children[i].type == ENCODER_STATE_TYPE_WAVEFRONT_ROW) {
      if ((state->encoder_control->cfg.slices & KVZ_SLICES_WPP) && i != 0) {
        // Add header for dependent WPP row slice.
        encoder_state_write_slice_header(&state->stream, &state->children[i], false);
      }
    }
    kvz_encoder_state_write_bitstream(&state->children[i]);
    kvz_bitstream_move(&state->stream, &state->children[i].stream);
  }
}

static void encoder_state_write_bitstream_main(encoder_state_t * const state)
{
  const encoder_control_t * const encoder = state->encoder_control;
  bitstream_t * const stream = &state->stream;
  uint64_t curpos = kvz_bitstream_tell(stream);

  // The first NAL unit of the access unit must use a long start code.
  state->frame->first_nal = true;

  // Access Unit Delimiter (AUD)
  if (encoder->cfg.aud_enable) {
    state->frame->first_nal = false;
    encoder_state_write_bitstream_aud(state);
  }

  if (encoder_state_must_write_vps(state)) {
    state->frame->first_nal = false;
    kvz_encoder_state_write_parameter_sets(&state->stream, state);
  }

  // Send Kvazaar version information only in the first frame.
  if (state->frame->num == 0 && encoder->cfg.add_encoder_info) {
    kvz_nal_write(stream, KVZ_NAL_PREFIX_SEI_NUT, 0, state->frame->first_nal);
    state->frame->first_nal = false;
    encoder_state_write_bitstream_prefix_sei_version(state);

    // spec:sei_rbsp() rbsp_trailing_bits
    kvz_bitstream_add_rbsp_trailing_bits(stream);
  }

  //SEI messages for interlacing
  if (encoder->vui.frame_field_info_present_flag) {
    // These should be optional, needed for earlier versions
    // of HM decoder to accept bitstream
    //kvz_nal_write(stream, KVZ_NAL_PREFIX_SEI_NUT, 0, 0);
    //encoder_state_write_active_parameter_sets_sei_message(state);
    //kvz_bitstream_rbsp_trailing_bits(stream);

    kvz_nal_write(stream, KVZ_NAL_PREFIX_SEI_NUT, 0, state->frame->first_nal);
    state->frame->first_nal = false;
    encoder_state_write_picture_timing_sei_message(state);

    // spec:sei_rbsp() rbsp_trailing_bits
    kvz_bitstream_add_rbsp_trailing_bits(stream);
  }

  encoder_state_write_bitstream_children(state);

  if (state->encoder_control->cfg.hash != KVZ_HASH_NONE) {
    // Calculate checksum
    add_checksum(state);
  }

  //Get bitstream length for stats
  uint64_t newpos = kvz_bitstream_tell(stream);
  state->stats_bitstream_length = (newpos >> 3) - (curpos >> 3);

  if (state->frame->num > 0) {
    state->frame->total_bits_coded = state->previous_encoder_state->frame->total_bits_coded;
  }
  state->frame->total_bits_coded += newpos - curpos;

    state->frame->cur_gop_bits_coded = state->previous_encoder_state->frame->cur_gop_bits_coded;
  state->frame->cur_gop_bits_coded += newpos - curpos;
}

void kvz_encoder_state_write_bitstream(encoder_state_t * const state)
{
  if (!state->is_leaf) {
    switch (state->type) {
      case ENCODER_STATE_TYPE_MAIN:
        encoder_state_write_bitstream_main(state);
        break;
      case ENCODER_STATE_TYPE_TILE:
      case ENCODER_STATE_TYPE_SLICE:
        encoder_state_write_bitstream_children(state);
        break;
      default:
        fprintf(stderr, "Unsupported node type %c!\n", state->type);
        assert(0);
    }
  }
}

void kvz_encoder_state_worker_write_bitstream(void * opaque)
{
  kvz_encoder_state_write_bitstream((encoder_state_t *) opaque);
}

void kvz_encoder_state_write_parameter_sets(bitstream_t *stream,
                                            encoder_state_t * const state)
{
  // Video Parameter Set (VPS)
  kvz_nal_write(stream, KVZ_NAL_VPS_NUT, 0, 1);
  encoder_state_write_bitstream_vid_parameter_set(stream, state);

  // Sequence Parameter Set (SPS)
  kvz_nal_write(stream, KVZ_NAL_SPS_NUT, 0, 1);
  encoder_state_write_bitstream_seq_parameter_set(stream, state);

  // Picture Parameter Set (PPS)
  kvz_nal_write(stream, KVZ_NAL_PPS_NUT, 0, 1);
  encoder_state_write_bitstream_pic_parameter_set(stream, state);
}
