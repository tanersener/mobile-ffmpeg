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
#include "config/aom_scale_rtcd.h"

#include "aom/aom_codec.h"
#include "aom/aom_encoder.h"

#include "aom_ports/system_state.h"

#if CONFIG_MISMATCH_DEBUG
#include "aom_util/debug_util.h"
#endif  // CONFIG_MISMATCH_DEBUG

#include "av1/common/onyxc_int.h"

#include "av1/encoder/encoder.h"
#include "av1/encoder/encode_strategy.h"
#include "av1/encoder/firstpass.h"
#include "av1/encoder/temporal_filter.h"
#include "av1/encoder/tpl_model.h"

void av1_configure_buffer_updates(AV1_COMP *const cpi,
                                  const FRAME_UPDATE_TYPE type,
                                  int force_refresh_all) {
  // NOTE(weitinglin): Should we define another function to take care of
  // cpi->rc.is_$Source_Type to make this function as it is in the comment?

  cpi->rc.is_src_frame_alt_ref = 0;
  cpi->rc.is_bwd_ref_frame = 0;
  cpi->rc.is_last_bipred_frame = 0;
  cpi->rc.is_bipred_frame = 0;
  cpi->rc.is_src_frame_ext_arf = 0;

  switch (type) {
    case KF_UPDATE:
      cpi->refresh_last_frame = 1;
      cpi->refresh_golden_frame = 1;
      cpi->refresh_bwd_ref_frame = 1;
      cpi->refresh_alt2_ref_frame = 1;
      cpi->refresh_alt_ref_frame = 1;
      break;

    case LF_UPDATE:
      cpi->refresh_last_frame = 1;
      cpi->refresh_golden_frame = 0;
      cpi->refresh_bwd_ref_frame = 0;
      cpi->refresh_alt2_ref_frame = 0;
      cpi->refresh_alt_ref_frame = 0;
      break;

    case GF_UPDATE:
      // TODO(zoeliu): To further investigate whether 'refresh_last_frame' is
      //               needed.
      cpi->refresh_last_frame = 1;
      cpi->refresh_golden_frame = 1;
      cpi->refresh_bwd_ref_frame = 0;
      cpi->refresh_alt2_ref_frame = 0;
      cpi->refresh_alt_ref_frame = 0;
      break;

    case OVERLAY_UPDATE:
      cpi->refresh_last_frame = 0;
      cpi->refresh_golden_frame = 1;
      cpi->refresh_bwd_ref_frame = 0;
      cpi->refresh_alt2_ref_frame = 0;
      cpi->refresh_alt_ref_frame = 0;

      cpi->rc.is_src_frame_alt_ref = 1;
      break;

    case ARF_UPDATE:
      cpi->refresh_last_frame = 0;
      cpi->refresh_golden_frame = 0;
      // NOTE: BWDREF does not get updated along with ALTREF_FRAME.
      cpi->refresh_bwd_ref_frame = 0;
      cpi->refresh_alt2_ref_frame = 0;
      cpi->refresh_alt_ref_frame = 1;
      break;

    case BRF_UPDATE:
      cpi->refresh_last_frame = 0;
      cpi->refresh_golden_frame = 0;
      cpi->refresh_bwd_ref_frame = 1;
      cpi->refresh_alt2_ref_frame = 0;
      cpi->refresh_alt_ref_frame = 0;

      cpi->rc.is_bwd_ref_frame = 1;
      break;

    case LAST_BIPRED_UPDATE:
      cpi->refresh_last_frame = 1;
      cpi->refresh_golden_frame = 0;
      cpi->refresh_bwd_ref_frame = 0;
      cpi->refresh_alt2_ref_frame = 0;
      cpi->refresh_alt_ref_frame = 0;

      cpi->rc.is_last_bipred_frame = 1;
      break;

    case BIPRED_UPDATE:
      cpi->refresh_last_frame = 1;
      cpi->refresh_golden_frame = 0;
      cpi->refresh_bwd_ref_frame = 0;
      cpi->refresh_alt2_ref_frame = 0;
      cpi->refresh_alt_ref_frame = 0;

      cpi->rc.is_bipred_frame = 1;
      break;

    case INTNL_OVERLAY_UPDATE:
      cpi->refresh_last_frame = 1;
      cpi->refresh_golden_frame = 0;
      cpi->refresh_bwd_ref_frame = 0;
      cpi->refresh_alt2_ref_frame = 0;
      cpi->refresh_alt_ref_frame = 0;

      cpi->rc.is_src_frame_alt_ref = 1;
      cpi->rc.is_src_frame_ext_arf = 1;
      break;

    case INTNL_ARF_UPDATE:
      cpi->refresh_last_frame = 0;
      cpi->refresh_golden_frame = 0;
      if (cpi->new_bwdref_update_rule == 1 && cpi->oxcf.pass == 2) {
        cpi->refresh_bwd_ref_frame = 1;
        cpi->refresh_alt2_ref_frame = 0;
      } else {
        cpi->refresh_bwd_ref_frame = 0;
        cpi->refresh_alt2_ref_frame = 1;
      }
      cpi->refresh_alt_ref_frame = 0;
      break;

    default: assert(0); break;
  }

  if (cpi->ext_refresh_frame_flags_pending &&
      (cpi->oxcf.pass == 0 || cpi->oxcf.pass == 2)) {
    cpi->refresh_last_frame = cpi->ext_refresh_last_frame;
    cpi->refresh_golden_frame = cpi->ext_refresh_golden_frame;
    cpi->refresh_alt_ref_frame = cpi->ext_refresh_alt_ref_frame;
    cpi->refresh_bwd_ref_frame = cpi->ext_refresh_bwd_ref_frame;
    cpi->refresh_alt2_ref_frame = cpi->ext_refresh_alt2_ref_frame;
  }

  if (force_refresh_all) {
    cpi->refresh_last_frame = 1;
    cpi->refresh_golden_frame = 1;
    cpi->refresh_bwd_ref_frame = 1;
    cpi->refresh_alt2_ref_frame = 1;
    cpi->refresh_alt_ref_frame = 1;
  }
}

static void set_additional_frame_flags(const AV1_COMMON *const cm,
                                       unsigned int *const frame_flags) {
  if (frame_is_intra_only(cm)) *frame_flags |= FRAMEFLAGS_INTRAONLY;
  if (frame_is_sframe(cm)) *frame_flags |= FRAMEFLAGS_SWITCH;
  if (cm->error_resilient_mode) *frame_flags |= FRAMEFLAGS_ERROR_RESILIENT;
}

static INLINE void update_keyframe_counters(AV1_COMP *cpi) {
  // TODO(zoeliu): To investigate whether we should treat BWDREF_FRAME
  //               differently here for rc->avg_frame_bandwidth.
  if (cpi->common.show_frame || cpi->rc.is_bwd_ref_frame) {
    if (!cpi->common.show_existing_frame || cpi->rc.is_src_frame_alt_ref ||
        cpi->common.current_frame.frame_type == KEY_FRAME) {
      // If this is a show_existing_frame with a source other than altref,
      // or if it is not a displayed forward keyframe, the keyframe update
      // counters were incremented when it was originally encoded.
      cpi->rc.frames_since_key++;
      cpi->rc.frames_to_key--;
    }
  }
}

static INLINE int is_frame_droppable(const AV1_COMP *const cpi) {
  return !(cpi->refresh_alt_ref_frame || cpi->refresh_alt2_ref_frame ||
           cpi->refresh_bwd_ref_frame || cpi->refresh_golden_frame ||
           cpi->refresh_last_frame);
}

static INLINE void update_frames_till_gf_update(AV1_COMP *cpi) {
  // TODO(weitinglin): Updating this counter for is_frame_droppable
  // is a work-around to handle the condition when a frame is drop.
  // We should fix the cpi->common.show_frame flag
  // instead of checking the other condition to update the counter properly.
  if (cpi->common.show_frame || is_frame_droppable(cpi)) {
    // Decrement count down till next gf
    if (cpi->rc.frames_till_gf_update_due > 0)
      cpi->rc.frames_till_gf_update_due--;
  }
}

static INLINE void update_twopass_gf_group_index(AV1_COMP *cpi) {
  // Increment the gf group index ready for the next frame. If this is
  // a show_existing_frame with a source other than altref, or if it is not
  // a displayed forward keyframe, the index was incremented when it was
  // originally encoded.
  if (!cpi->common.show_existing_frame || cpi->rc.is_src_frame_alt_ref ||
      cpi->common.current_frame.frame_type == KEY_FRAME) {
    ++cpi->twopass.gf_group.index;
  }
}

static void update_rc_counts(AV1_COMP *cpi) {
  update_keyframe_counters(cpi);
  update_frames_till_gf_update(cpi);
  if (cpi->oxcf.pass == 2) update_twopass_gf_group_index(cpi);
}

static void check_show_existing_frame(AV1_COMP *cpi) {
  const GF_GROUP *const gf_group = &cpi->twopass.gf_group;
  AV1_COMMON *const cm = &cpi->common;
  const FRAME_UPDATE_TYPE frame_update_type =
      gf_group->update_type[gf_group->index];
  const int which_arf = cpi->new_bwdref_update_rule
                            ? gf_group->arf_update_idx[gf_group->index] > 0
                            : gf_group->arf_update_idx[gf_group->index];

  if (cm->show_existing_frame == 1) {
    cm->show_existing_frame = 0;
  } else if (cpi->rc.is_last_bipred_frame) {
    // NOTE: When new structure is used, every bwdref will have one overlay
    //       frame. Therefore, there is no need to find out which frame to
    //       show in advance.
    if (!cpi->new_bwdref_update_rule) {
      // NOTE: If the last frame is a last bi-predictive frame, it is
      //       needed next to show the BWDREF_FRAME, which is pointed by
      //       the last_fb_idxes[0] after reference frame buffer update
      cm->show_existing_frame = 1;
      cpi->existing_fb_idx_to_show = cm->remapped_ref_idx[0];
    }
  } else if (cpi->is_arf_filter_off[which_arf] &&
             (frame_update_type == OVERLAY_UPDATE ||
              frame_update_type == INTNL_OVERLAY_UPDATE)) {
    const int bwdref_to_show =
        cpi->new_bwdref_update_rule ? BWDREF_FRAME : ALTREF2_FRAME;
    // Other parameters related to OVERLAY_UPDATE will be taken care of
    // in av1_rc_get_second_pass_params(cpi)
    cm->show_existing_frame = 1;
    cpi->existing_fb_idx_to_show =
        (frame_update_type == OVERLAY_UPDATE)
            ? get_ref_frame_map_idx(cm, ALTREF_FRAME)
            : get_ref_frame_map_idx(cm, bwdref_to_show);
    if (!cpi->new_bwdref_update_rule) {
      cpi->is_arf_filter_off[which_arf] = 0;
    }
  }
}

static void set_ext_overrides(AV1_COMP *const cpi,
                              EncodeFrameParams *const frame_params) {
  // Overrides the defaults with the externally supplied values with
  // av1_update_reference() and av1_update_entropy() calls
  // Note: The overrides are valid only for the next frame passed
  // to av1_encode_lowlevel()

  AV1_COMMON *const cm = &cpi->common;

  if (cpi->ext_use_s_frame) {
    frame_params->frame_type = S_FRAME;
  }

  if (cpi->ext_refresh_frame_context_pending) {
    cm->refresh_frame_context = cpi->ext_refresh_frame_context;
    cpi->ext_refresh_frame_context_pending = 0;
  }
  cm->allow_ref_frame_mvs = cpi->ext_use_ref_frame_mvs;

  frame_params->error_resilient_mode = cpi->ext_use_error_resilient;
  // A keyframe is already error resilient and keyframes with
  // error_resilient_mode interferes with the use of show_existing_frame
  // when forward reference keyframes are enabled.
  frame_params->error_resilient_mode &= frame_params->frame_type != KEY_FRAME;
  // For bitstream conformance, s-frames must be error-resilient
  frame_params->error_resilient_mode |= frame_params->frame_type == S_FRAME;
}

static int get_ref_frame_flags(const AV1_COMP *const cpi) {
  const AV1_COMMON *const cm = &cpi->common;

  const RefCntBuffer *last_buf = get_ref_frame_buf(cm, LAST_FRAME);
  const RefCntBuffer *last2_buf = get_ref_frame_buf(cm, LAST2_FRAME);
  const RefCntBuffer *last3_buf = get_ref_frame_buf(cm, LAST3_FRAME);
  const RefCntBuffer *golden_buf = get_ref_frame_buf(cm, GOLDEN_FRAME);
  const RefCntBuffer *bwd_buf = get_ref_frame_buf(cm, BWDREF_FRAME);
  const RefCntBuffer *alt2_buf = get_ref_frame_buf(cm, ALTREF2_FRAME);
  const RefCntBuffer *alt_buf = get_ref_frame_buf(cm, ALTREF_FRAME);

  // No.1 Priority: LAST_FRAME
  const int last2_is_last = (last2_buf == last_buf);
  const int last3_is_last = (last3_buf == last_buf);
  const int gld_is_last = (golden_buf == last_buf);
  const int bwd_is_last = (bwd_buf == last_buf);
  const int alt2_is_last = (alt2_buf == last_buf);
  const int alt_is_last = (alt_buf == last_buf);

  // No.2 Priority: ALTREF_FRAME
  const int last2_is_alt = (last2_buf == alt_buf);
  const int last3_is_alt = (last3_buf == alt_buf);
  const int gld_is_alt = (golden_buf == alt_buf);
  const int bwd_is_alt = (bwd_buf == alt_buf);
  const int alt2_is_alt = (alt2_buf == alt_buf);

  // No.3 Priority: LAST2_FRAME
  const int last3_is_last2 = (last3_buf == last2_buf);
  const int gld_is_last2 = (golden_buf == last2_buf);
  const int bwd_is_last2 = (bwd_buf == last2_buf);
  const int alt2_is_last2 = (alt2_buf == last2_buf);

  // No.4 Priority: LAST3_FRAME
  const int gld_is_last3 = (golden_buf == last3_buf);
  const int bwd_is_last3 = (bwd_buf == last3_buf);
  const int alt2_is_last3 = (alt2_buf == last3_buf);

  // No.5 Priority: GOLDEN_FRAME
  const int bwd_is_gld = (bwd_buf == golden_buf);
  const int alt2_is_gld = (alt2_buf == golden_buf);

  // No.6 Priority: BWDREF_FRAME
  const int alt2_is_bwd = (alt2_buf == bwd_buf);

  // No.7 Priority: ALTREF2_FRAME

  // cpi->ext_ref_frame_flags allows certain reference types to be disabled
  // by the external interface.  These are set by av1_apply_encoding_flags().
  // Start with what the external interface allows, then suppress any reference
  // types which we have found to be duplicates.

  int flags = cpi->ext_ref_frame_flags;

  if (cpi->rc.frames_till_gf_update_due == INT_MAX) flags &= ~AOM_GOLD_FLAG;

  if (alt_is_last) flags &= ~AOM_ALT_FLAG;

  if (last2_is_last || last2_is_alt) flags &= ~AOM_LAST2_FLAG;

  if (last3_is_last || last3_is_alt || last3_is_last2) flags &= ~AOM_LAST3_FLAG;

  if (gld_is_last || gld_is_alt || gld_is_last2 || gld_is_last3)
    flags &= ~AOM_GOLD_FLAG;

  if ((bwd_is_last || bwd_is_alt || bwd_is_last2 || bwd_is_last3 || bwd_is_gld))
    flags &= ~AOM_BWD_FLAG;

  if ((alt2_is_last || alt2_is_alt || alt2_is_last2 || alt2_is_last3 ||
       alt2_is_gld || alt2_is_bwd))
    flags &= ~AOM_ALT2_FLAG;

  return flags;
}

static int get_current_frame_ref_type(
    const AV1_COMP *const cpi, const EncodeFrameParams *const frame_params) {
  const GF_GROUP *const gf_group = &cpi->twopass.gf_group;
  // We choose the reference "type" of this frame from the flags which indicate
  // which reference frames will be refreshed by it.  More than one of these
  // flags may be set, so the order here implies an order of precedence.
  // This is just used to choose the primary_ref_frame (as the most recent
  // reference buffer of the same reference-type as the current frame)

  const int intra_only = frame_params->frame_type == KEY_FRAME ||
                         frame_params->frame_type == INTRA_ONLY_FRAME;
  if (intra_only || frame_params->error_resilient_mode ||
      cpi->ext_use_primary_ref_none)
    return REGULAR_FRAME;
  else if (gf_group->update_type[gf_group->index] == INTNL_ARF_UPDATE)
    return EXT_ARF_FRAME;
  else if (cpi->refresh_alt_ref_frame)
    return ARF_FRAME;
  else if (cpi->rc.is_src_frame_alt_ref)
    return OVERLAY_FRAME;
  else if (cpi->refresh_golden_frame)
    return GLD_FRAME;
  else if (cpi->refresh_bwd_ref_frame)
    return BRF_FRAME;
  else
    return REGULAR_FRAME;
}

static int choose_primary_ref_frame(
    const AV1_COMP *const cpi, const EncodeFrameParams *const frame_params) {
  const AV1_COMMON *const cm = &cpi->common;

  const int intra_only = frame_params->frame_type == KEY_FRAME ||
                         frame_params->frame_type == INTRA_ONLY_FRAME;
  if (intra_only || frame_params->error_resilient_mode ||
      cpi->ext_use_primary_ref_none) {
    return PRIMARY_REF_NONE;
  }

  // Find the most recent reference frame with the same reference type as the
  // current frame
  const FRAME_CONTEXT_INDEX current_ref_type =
      get_current_frame_ref_type(cpi, frame_params);
  int wanted_fb = cpi->fb_of_context_type[current_ref_type];

  int primary_ref_frame = PRIMARY_REF_NONE;
  for (int ref_frame = LAST_FRAME; ref_frame <= ALTREF_FRAME; ref_frame++) {
    if (get_ref_frame_map_idx(cm, ref_frame) == wanted_fb) {
      primary_ref_frame = ref_frame - LAST_FRAME;
    }
  }
  return primary_ref_frame;
}

static void update_fb_of_context_type(
    const AV1_COMP *const cpi, const EncodeFrameParams *const frame_params,
    int *const fb_of_context_type) {
  const AV1_COMMON *const cm = &cpi->common;

  if (frame_is_intra_only(cm) || cm->error_resilient_mode ||
      cpi->ext_use_primary_ref_none) {
    for (int i = 0; i < REF_FRAMES; i++) {
      fb_of_context_type[i] = -1;
    }
    fb_of_context_type[REGULAR_FRAME] =
        cm->show_frame ? get_ref_frame_map_idx(cm, GOLDEN_FRAME)
                       : get_ref_frame_map_idx(cm, ALTREF_FRAME);
  }

  if (!encode_show_existing_frame(cm)) {
    // Refresh fb_of_context_type[]: see encoder.h for explanation
    // Note that we want the value of refresh_frame_flags for the frame that
    // just happened.  If we call get_refresh_frame_flags now we will get a
    // different answer, because update_reference_frames() has happened.
    if (cm->current_frame.frame_type == KEY_FRAME) {
      // All ref frames are refreshed, pick one that will live long enough
      fb_of_context_type[REGULAR_FRAME] = 0;
    } else {
      // If more than one frame is refreshed, it doesn't matter which one we
      // pick so pick the first.  LST sometimes doesn't refresh any: this is ok
      const int current_frame_ref_type =
          get_current_frame_ref_type(cpi, frame_params);
      for (int i = 0; i < REF_FRAMES; i++) {
        if (cm->current_frame.refresh_frame_flags & (1 << i)) {
          fb_of_context_type[current_frame_ref_type] = i;
          break;
        }
      }
    }
  }
}

static int get_order_offset(const AV1_COMP *const cpi,
                            const EncodeFrameParams *const frame_params) {
  // shown frame by definition has order offset 0
  // show_existing_frame ignores order_offset and simply takes the order_hint
  // from the reference frame being shown.
  if (frame_params->show_frame || cpi->common.show_existing_frame) return 0;

  const GF_GROUP *const gf_group = &cpi->twopass.gf_group;
  const int arf_offset =
      AOMMIN((MAX_GF_INTERVAL - 1), gf_group->arf_src_offset[gf_group->index]);
  const int brf_offset = gf_group->brf_src_offset[gf_group->index];
  return AOMMIN((MAX_GF_INTERVAL - 1), arf_offset + brf_offset);
}

static void adjust_frame_rate(AV1_COMP *cpi,
                              const struct lookahead_entry *source) {
  int64_t this_duration;
  int step = 0;

  // Clear down mmx registers
  aom_clear_system_state();

  if (source->ts_start == cpi->first_time_stamp_ever) {
    this_duration = source->ts_end - source->ts_start;
    step = 1;
  } else {
    int64_t last_duration =
        cpi->last_end_time_stamp_seen - cpi->last_time_stamp_seen;

    this_duration = source->ts_end - cpi->last_end_time_stamp_seen;

    // do a step update if the duration changes by 10%
    if (last_duration)
      step = (int)((this_duration - last_duration) * 10 / last_duration);
  }

  if (this_duration) {
    if (step) {
      av1_new_framerate(cpi, 10000000.0 / this_duration);
    } else {
      // Average this frame's rate into the last second's average
      // frame rate. If we haven't seen 1 second yet, then average
      // over the whole interval seen.
      const double interval = AOMMIN(
          (double)(source->ts_end - cpi->first_time_stamp_ever), 10000000.0);
      double avg_duration = 10000000.0 / cpi->framerate;
      avg_duration *= (interval - avg_duration + this_duration);
      avg_duration /= interval;

      av1_new_framerate(cpi, 10000000.0 / avg_duration);
    }
  }
  cpi->last_time_stamp_seen = source->ts_start;
  cpi->last_end_time_stamp_seen = source->ts_end;
}

// Returns 0 if this is not an alt ref else the offset of the source frame
// used as the arf midpoint.
static int get_arf_src_index(AV1_COMP *cpi) {
  RATE_CONTROL *const rc = &cpi->rc;
  int arf_src_index = 0;
  if (is_altref_enabled(cpi)) {
    if (cpi->oxcf.pass == 2) {
      const GF_GROUP *const gf_group = &cpi->twopass.gf_group;
      if (gf_group->update_type[gf_group->index] == ARF_UPDATE) {
        arf_src_index = gf_group->arf_src_offset[gf_group->index];
      }
    } else if (rc->source_alt_ref_pending) {
      arf_src_index = rc->frames_till_gf_update_due;
    }
  }
  return arf_src_index;
}

static int get_brf_src_index(AV1_COMP *cpi) {
  int brf_src_index = 0;
  const GF_GROUP *const gf_group = &cpi->twopass.gf_group;

  // TODO(zoeliu): We need to add the check on the -bwd_ref command line setup
  //               flag.
  if (gf_group->bidir_pred_enabled[gf_group->index]) {
    if (cpi->oxcf.pass == 2) {
      if (gf_group->update_type[gf_group->index] == BRF_UPDATE)
        brf_src_index = gf_group->brf_src_offset[gf_group->index];
    } else {
      // TODO(zoeliu): To re-visit the setup for this scenario
      brf_src_index = cpi->rc.bipred_group_interval - 1;
    }
  }

  return brf_src_index;
}

// Returns 0 if this is not an alt ref else the offset of the source frame
// used as the arf midpoint.
static int get_arf2_src_index(AV1_COMP *cpi) {
  int arf2_src_index = 0;
  if (is_altref_enabled(cpi) && cpi->num_extra_arfs) {
    if (cpi->oxcf.pass == 2) {
      const GF_GROUP *const gf_group = &cpi->twopass.gf_group;
      if (gf_group->update_type[gf_group->index] == INTNL_ARF_UPDATE) {
        arf2_src_index = gf_group->arf_src_offset[gf_group->index];
      }
    }
  }
  return arf2_src_index;
}

// Called if this frame is an ARF or ARF2. Also handles forward-keyframes
// For an ARF set arf2=0, for ARF2 set arf2=1
// temporal_filtered is set to 1 if we temporally filter the ARF frame, so that
// the correct post-filter buffer can be used.
static struct lookahead_entry *setup_arf_or_arf2(
    AV1_COMP *const cpi, const int arf_src_index, const int arf2,
    int *temporal_filtered, EncodeFrameParams *const frame_params) {
  AV1_COMMON *const cm = &cpi->common;
  RATE_CONTROL *const rc = &cpi->rc;
  const AV1EncoderConfig *const oxcf = &cpi->oxcf;

  assert(arf_src_index <= rc->frames_to_key);
  *temporal_filtered = 0;

  struct lookahead_entry *source =
      av1_lookahead_peek(cpi->lookahead, arf_src_index);

  if (source != NULL) {
    cm->showable_frame = 1;
    cpi->alt_ref_source = source;

    // When arf_src_index == rc->frames_to_key, it indicates a fwd_kf
    if (!arf2 && arf_src_index == rc->frames_to_key) {
      // Skip temporal filtering and mark as intra_only if we have a fwd_kf
      const GF_GROUP *const gf_group = &cpi->twopass.gf_group;
      int which_arf = gf_group->arf_update_idx[gf_group->index];
      cpi->is_arf_filter_off[which_arf] = 1;
      cpi->no_show_kf = 1;
    } else {
      if (oxcf->arnr_max_frames > 0) {
        // Produce the filtered ARF frame.
        av1_temporal_filter(cpi, arf_src_index);
        aom_extend_frame_borders(&cpi->alt_ref_buffer, av1_num_planes(cm));
        *temporal_filtered = 1;
      }
    }
    frame_params->show_frame = 0;
  }
  rc->source_alt_ref_pending = 0;
  return source;
}

// Determine whether there is a forced keyframe pending in the lookahead buffer
static int is_forced_keyframe_pending(struct lookahead_ctx *lookahead,
                                      const int up_to_index) {
  for (int i = 0; i <= up_to_index; i++) {
    const struct lookahead_entry *e = av1_lookahead_peek(lookahead, i);
    if (e == NULL) {
      // We have reached the end of the lookahead buffer and not early-returned
      // so there isn't a forced key-frame pending.
      return 0;
    } else if (e->flags == AOM_EFLAG_FORCE_KF) {
      return 1;
    } else {
      continue;
    }
  }
  return 0;  // Never reached
}

// Check if we should encode an ARF, ARF2 or BRF.  If not, try a LAST
// Do some setup associated with the chosen source
// temporal_filtered, flush, and frame_update_type are outputs.
// Return the frame source, or NULL if we couldn't find one
struct lookahead_entry *choose_frame_source(
    AV1_COMP *const cpi, int *const temporal_filtered, int *const flush,
    struct lookahead_entry **last_source, FRAME_UPDATE_TYPE *frame_update_type,
    EncodeFrameParams *const frame_params) {
  AV1_COMMON *const cm = &cpi->common;
  struct lookahead_entry *source = NULL;
  *temporal_filtered = 0;

  // Should we encode an alt-ref frame.
  int arf_src_index = get_arf_src_index(cpi);
  if (arf_src_index &&
      is_forced_keyframe_pending(cpi->lookahead, arf_src_index)) {
    arf_src_index = 0;
    *flush = 1;
  }

  if (arf_src_index) {
    source = setup_arf_or_arf2(cpi, arf_src_index, 0, temporal_filtered,
                               frame_params);
    *frame_update_type = ARF_UPDATE;
  }

  // Should we encode an arf2 frame (mutually exclusive to ARF)
  arf_src_index = get_arf2_src_index(cpi);
  if (arf_src_index &&
      is_forced_keyframe_pending(cpi->lookahead, arf_src_index)) {
    arf_src_index = 0;
    *flush = 1;
  }

  if (arf_src_index) {
    source = setup_arf_or_arf2(cpi, arf_src_index, 1, temporal_filtered,
                               frame_params);
    *frame_update_type = INTNL_ARF_UPDATE;
  }

  int brf_src_index = get_brf_src_index(cpi);
  if (brf_src_index) {
    assert(brf_src_index <= cpi->rc.frames_to_key);
    if ((source = av1_lookahead_peek(cpi->lookahead, brf_src_index)) != NULL) {
      cm->showable_frame = 1;
      frame_params->show_frame = 0;
      *frame_update_type = BRF_UPDATE;
    }
  }

  if (!source) {
    // Get last frame source.
    if (cm->current_frame.frame_number > 0) {
      *last_source = av1_lookahead_peek(cpi->lookahead, -1);
    }
    // Read in the source frame.
    source = av1_lookahead_pop(cpi->lookahead, *flush);
    if (source == NULL) return NULL;
    *frame_update_type = LF_UPDATE;  // Default update type
    frame_params->show_frame = 1;

    // Check to see if the frame should be encoded as an arf overlay.
    if (cpi->alt_ref_source == source) {
      *frame_update_type = OVERLAY_UPDATE;
      cpi->alt_ref_source = NULL;
    }
  }
  return source;
}

// Don't allow a show_existing_frame to coincide with an error resilient or
// S-Frame. An exception can be made in the case of a keyframe, since it does
// not depend on any previous frames.
static int allow_show_existing(const AV1_COMP *const cpi) {
  if (cpi->common.current_frame.frame_number == 0) return 0;

  const struct lookahead_entry *lookahead_src =
      av1_lookahead_peek(cpi->lookahead, 0);
  if (lookahead_src == NULL) return 1;

  const int is_error_resilient =
      cpi->oxcf.error_resilient_mode ||
      (lookahead_src->flags & AOM_EFLAG_ERROR_RESILIENT);
  const int is_s_frame =
      cpi->oxcf.s_frame_mode || (lookahead_src->flags & AOM_EFLAG_SET_S_FRAME);
  const int is_key_frame =
      (cpi->rc.frames_to_key == 0) || (cpi->frame_flags & FRAMEFLAGS_KEY);
  return !(is_error_resilient || is_s_frame) || is_key_frame;
}

int av1_encode_strategy(AV1_COMP *const cpi, size_t *const size,
                        uint8_t *const dest, unsigned int *frame_flags,
                        int64_t *const time_stamp, int64_t *const time_end,
                        const aom_rational_t *const timebase, int flush) {
  const AV1EncoderConfig *const oxcf = &cpi->oxcf;
  AV1_COMMON *const cm = &cpi->common;

  EncodeFrameInput frame_input;
  EncodeFrameParams frame_params;
  EncodeFrameResults frame_results;
  memset(&frame_input, 0, sizeof(frame_input));
  memset(&frame_params, 0, sizeof(frame_params));
  memset(&frame_results, 0, sizeof(frame_results));

  if (oxcf->pass == 0 || oxcf->pass == 2) {
    check_show_existing_frame(cpi);
    cm->show_existing_frame &= allow_show_existing(cpi);
  } else {
    cm->show_existing_frame = 0;
  }

  int temporal_filtered = 0;
  struct lookahead_entry *source = NULL;
  struct lookahead_entry *last_source = NULL;
  FRAME_UPDATE_TYPE frame_update_type;
  if (cm->show_existing_frame) {
    source = av1_lookahead_pop(cpi->lookahead, flush);
    frame_update_type = LF_UPDATE;
  } else {
    source = choose_frame_source(cpi, &temporal_filtered, &flush, &last_source,
                                 &frame_update_type, &frame_params);
  }

  // In pass 2 we get the frame_update_type from gf_group
  if (oxcf->pass == 2) {
    frame_update_type =
        cpi->twopass.gf_group.update_type[cpi->twopass.gf_group.index];
  }

  if (source == NULL) {  // If no source was found, we can't encode a frame.
    if (flush && oxcf->pass == 1 && !cpi->twopass.first_pass_done) {
      av1_end_first_pass(cpi); /* get last stats packet */
      cpi->twopass.first_pass_done = 1;
    }
    return -1;
  }

  frame_input.source = temporal_filtered ? &cpi->alt_ref_buffer : &source->img;
  frame_input.last_source = last_source != NULL ? &last_source->img : NULL;
  frame_input.ts_duration = source->ts_end - source->ts_start;

  *time_stamp = source->ts_start;
  *time_end = source->ts_end;
  if (source->ts_start < cpi->first_time_stamp_ever) {
    cpi->first_time_stamp_ever = source->ts_start;
    cpi->last_end_time_stamp_seen = source->ts_start;
  }

  av1_apply_encoding_flags(cpi, source->flags);
  if (!cm->show_existing_frame)
    *frame_flags = (source->flags & AOM_EFLAG_FORCE_KF) ? FRAMEFLAGS_KEY : 0;
  cpi->frame_flags = *frame_flags;

  const int is_overlay =
      cm->show_existing_frame && (frame_update_type == OVERLAY_UPDATE ||
                                  frame_update_type == INTNL_OVERLAY_UPDATE);
  if (frame_params.show_frame || is_overlay) {
    // Shown frames and arf-overlay frames need frame-rate considering
    adjust_frame_rate(cpi, source);
  }

  if (cm->show_existing_frame) {
    // show_existing_frame implies this frame is shown!
    frame_params.show_frame = 1;
  } else {
    // Retain the RF_LEVEL for the current newly coded frame.
    cm->cur_frame->frame_rf_level =
        cpi->twopass.gf_group.rf_level[cpi->twopass.gf_group.index];

    if (cpi->film_grain_table) {
      cm->seq_params.film_grain_params_present = aom_film_grain_table_lookup(
          cpi->film_grain_table, *time_stamp, *time_end, 0 /* =erase */,
          &cm->film_grain_params);
    }
    cm->cur_frame->film_grain_params_present =
        cm->seq_params.film_grain_params_present;

    // only one operating point supported now
    const int64_t pts64 = ticks_to_timebase_units(timebase, *time_stamp);
    if (pts64 < 0 || pts64 > UINT32_MAX) return AOM_CODEC_ERROR;
    cpi->common.frame_presentation_time = (uint32_t)pts64;
  }

  if (oxcf->pass == 2 && (!cm->show_existing_frame || is_overlay)) {
    // GF_GROUP needs updating for arf overlays as well as non-show-existing
    av1_rc_get_second_pass_params(cpi, &frame_params);
    frame_update_type =
        cpi->twopass.gf_group.update_type[cpi->twopass.gf_group.index];
  }

  if (cm->show_existing_frame && frame_params.frame_type != KEY_FRAME) {
    // Force show-existing frames to be INTER, except forward keyframes
    frame_params.frame_type = INTER_FRAME;
  }

  frame_params.frame_flags = frame_flags;

  // TODO(david.turner@argondesign.com): Move all the encode strategy
  // (largely near av1_get_compressed_data) in here

  // TODO(david.turner@argondesign.com): Change all the encode strategy to
  // modify frame_params instead of cm or cpi.

  // Per-frame encode speed.  In theory this can vary, but things may have been
  // written assuming speed-level will not change within a sequence, so this
  // parameter should be used with caution.
  frame_params.speed = oxcf->speed;

  if (!cm->show_existing_frame) {
    cm->using_qmatrix = cpi->oxcf.using_qm;
    cm->min_qmlevel = cpi->oxcf.qm_minlevel;
    cm->max_qmlevel = cpi->oxcf.qm_maxlevel;
    if (cpi->twopass.gf_group.index == 1 && cpi->oxcf.enable_tpl_model) {
      av1_configure_buffer_updates(cpi, frame_update_type, 0);
      av1_set_frame_size(cpi, cm->width, cm->height);
      av1_tpl_setup_stats(cpi, &frame_input);
    }
  }

  // Work out some encoding parameters specific to the pass:
  if (oxcf->pass == 0) {
    if (cpi->oxcf.rc_mode == AOM_CBR) {
      av1_rc_get_one_pass_cbr_params(cpi, &frame_update_type, &frame_params);
    } else {
      av1_rc_get_one_pass_vbr_params(cpi, &frame_update_type, &frame_params);
    }
  } else if (oxcf->pass == 1) {
    cpi->td.mb.e_mbd.lossless[0] = is_lossless_requested(&cpi->oxcf);
    const int kf_requested = (cm->current_frame.frame_number == 0 ||
                              (cpi->frame_flags & FRAMEFLAGS_KEY));
    if (kf_requested && frame_update_type != OVERLAY_UPDATE &&
        frame_update_type != INTNL_OVERLAY_UPDATE) {
      frame_params.frame_type = KEY_FRAME;
    } else {
      frame_params.frame_type = INTER_FRAME;
    }
  } else if (oxcf->pass == 2) {
#if CONFIG_MISMATCH_DEBUG
    mismatch_move_frame_idx_w();
#endif
#if TXCOEFF_COST_TIMER
    cm->txcoeff_cost_timer = 0;
    cm->txcoeff_cost_count = 0;
#endif
  }

  if (oxcf->pass == 0 || oxcf->pass == 2) set_ext_overrides(cpi, &frame_params);

  // Shown keyframes and S frames refresh all reference buffers
  const int force_refresh_all =
      ((frame_params.frame_type == KEY_FRAME && frame_params.show_frame) ||
       frame_params.frame_type == S_FRAME) &&
      !cm->show_existing_frame;

  av1_configure_buffer_updates(cpi, frame_update_type, force_refresh_all);

  if (oxcf->pass == 0 || oxcf->pass == 2) {
    // Work out which reference frame slots may be used.
    frame_params.ref_frame_flags = get_ref_frame_flags(cpi);

    frame_params.primary_ref_frame =
        choose_primary_ref_frame(cpi, &frame_params);
    frame_params.order_offset = get_order_offset(cpi, &frame_params);
  }

  if (av1_encode(cpi, dest, &frame_input, &frame_params, &frame_results) !=
      AOM_CODEC_OK) {
    return AOM_CODEC_ERROR;
  }

  if (oxcf->pass == 2) {
#if TXCOEFF_COST_TIMER
    cm->cum_txcoeff_cost_timer += cm->txcoeff_cost_timer;
    fprintf(stderr,
            "\ntxb coeff cost block number: %ld, frame time: %ld, cum time %ld "
            "in us\n",
            cm->txcoeff_cost_count, cm->txcoeff_cost_timer,
            cm->cum_txcoeff_cost_timer);
#endif
    av1_twopass_postencode_update(cpi);
  }

  if (oxcf->pass == 0 || oxcf->pass == 2) {
    update_fb_of_context_type(cpi, &frame_params, cpi->fb_of_context_type);
    set_additional_frame_flags(cm, frame_params.frame_flags);
    update_rc_counts(cpi);
  }

  // Unpack frame_results:
  *size = frame_results.size;

  // Leave a signal for a higher level caller about if this frame is droppable
  if (*size > 0) {
    cpi->droppable = is_frame_droppable(cpi);
  }

  return AOM_CODEC_OK;
}
