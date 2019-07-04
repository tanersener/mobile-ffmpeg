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

#include "av1/common/onyxc_int.h"

#include "av1/encoder/encoder.h"
#include "av1/encoder/firstpass.h"
#include "av1/encoder/gop_structure.h"
#include "av1/encoder/ratectrl.h"
#include "av1/encoder/use_flat_gop_model_params.h"

#define DEFAULT_KF_BOOST 2300
#define DEFAULT_GF_BOOST 2000

// Calculate an active area of the image that discounts formatting
// bars and partially discounts other 0 energy areas.
#define MIN_ACTIVE_AREA 0.5
#define MAX_ACTIVE_AREA 1.0
static double calculate_active_area(const AV1_COMP *cpi,
                                    const FIRSTPASS_STATS *this_frame) {
  const double active_pct =
      1.0 -
      ((this_frame->intra_skip_pct / 2) +
       ((this_frame->inactive_zone_rows * 2) / (double)cpi->common.mb_rows));
  return fclamp(active_pct, MIN_ACTIVE_AREA, MAX_ACTIVE_AREA);
}

// Calculate a modified Error used in distributing bits between easier and
// harder frames.
#define ACT_AREA_CORRECTION 0.5
static double calculate_modified_err(const AV1_COMP *cpi,
                                     const TWO_PASS *twopass,
                                     const AV1EncoderConfig *oxcf,
                                     const FIRSTPASS_STATS *this_frame) {
  const FIRSTPASS_STATS *const stats = &twopass->total_stats;
  const double av_weight = stats->weight / stats->count;
  const double av_err = (stats->coded_error * av_weight) / stats->count;
  double modified_error =
      av_err * pow(this_frame->coded_error * this_frame->weight /
                       DOUBLE_DIVIDE_CHECK(av_err),
                   oxcf->two_pass_vbrbias / 100.0);

  // Correction for active area. Frames with a reduced active area
  // (eg due to formatting bars) have a higher error per mb for the
  // remaining active MBs. The correction here assumes that coding
  // 0.5N blocks of complexity 2X is a little easier than coding N
  // blocks of complexity X.
  modified_error *=
      pow(calculate_active_area(cpi, this_frame), ACT_AREA_CORRECTION);

  return fclamp(modified_error, twopass->modified_error_min,
                twopass->modified_error_max);
}

// Resets the first pass file to the given position using a relative seek from
// the current position.
static void reset_fpf_position(TWO_PASS *p, const FIRSTPASS_STATS *position) {
  p->stats_in = position;
}

static int input_stats(TWO_PASS *p, FIRSTPASS_STATS *fps) {
  if (p->stats_in >= p->stats_in_end) return EOF;

  *fps = *p->stats_in;
  ++p->stats_in;
  return 1;
}

// Read frame stats at an offset from the current position.
static const FIRSTPASS_STATS *read_frame_stats(const TWO_PASS *p, int offset) {
  if ((offset >= 0 && p->stats_in + offset >= p->stats_in_end) ||
      (offset < 0 && p->stats_in + offset < p->stats_in_start)) {
    return NULL;
  }

  return &p->stats_in[offset];
}

static void subtract_stats(FIRSTPASS_STATS *section,
                           const FIRSTPASS_STATS *frame) {
  section->frame -= frame->frame;
  section->weight -= frame->weight;
  section->intra_error -= frame->intra_error;
  section->frame_avg_wavelet_energy -= frame->frame_avg_wavelet_energy;
  section->coded_error -= frame->coded_error;
  section->sr_coded_error -= frame->sr_coded_error;
  section->pcnt_inter -= frame->pcnt_inter;
  section->pcnt_motion -= frame->pcnt_motion;
  section->pcnt_second_ref -= frame->pcnt_second_ref;
  section->pcnt_neutral -= frame->pcnt_neutral;
  section->intra_skip_pct -= frame->intra_skip_pct;
  section->inactive_zone_rows -= frame->inactive_zone_rows;
  section->inactive_zone_cols -= frame->inactive_zone_cols;
  section->MVr -= frame->MVr;
  section->mvr_abs -= frame->mvr_abs;
  section->MVc -= frame->MVc;
  section->mvc_abs -= frame->mvc_abs;
  section->MVrv -= frame->MVrv;
  section->MVcv -= frame->MVcv;
  section->mv_in_out_count -= frame->mv_in_out_count;
  section->new_mv_count -= frame->new_mv_count;
  section->count -= frame->count;
  section->duration -= frame->duration;
}

// Calculate the linear size relative to a baseline of 1080P
#define BASE_SIZE 2073600.0  // 1920x1080
static double get_linear_size_factor(const AV1_COMP *cpi) {
  const double this_area = cpi->initial_width * cpi->initial_height;
  return pow(this_area / BASE_SIZE, 0.5);
}

// This function returns the maximum target rate per frame.
static int frame_max_bits(const RATE_CONTROL *rc,
                          const AV1EncoderConfig *oxcf) {
  int64_t max_bits = ((int64_t)rc->avg_frame_bandwidth *
                      (int64_t)oxcf->two_pass_vbrmax_section) /
                     100;
  if (max_bits < 0)
    max_bits = 0;
  else if (max_bits > rc->max_frame_bandwidth)
    max_bits = rc->max_frame_bandwidth;

  return (int)max_bits;
}

static double calc_correction_factor(double err_per_mb, double err_divisor,
                                     double pt_low, double pt_high, int q,
                                     aom_bit_depth_t bit_depth) {
  const double error_term = err_per_mb / err_divisor;

  // Adjustment based on actual quantizer to power term.
  const double power_term =
      AOMMIN(av1_convert_qindex_to_q(q, bit_depth) * 0.01 + pt_low, pt_high);

  // Calculate correction factor.
  if (power_term < 1.0) assert(error_term >= 0.0);

  return fclamp(pow(error_term, power_term), 0.05, 5.0);
}

#define ERR_DIVISOR 100.0
#define FACTOR_PT_LOW 0.70
#define FACTOR_PT_HIGH 0.90

// Similar to find_qindex_by_rate() function in ratectrl.c, but includes
// calculation of a correction_factor.
static int find_qindex_by_rate_with_correction(
    int desired_bits_per_mb, aom_bit_depth_t bit_depth, FRAME_TYPE frame_type,
    double error_per_mb, double ediv_size_correction,
    double group_weight_factor, int best_qindex, int worst_qindex) {
  assert(best_qindex <= worst_qindex);
  int low = best_qindex;
  int high = worst_qindex;
  while (low < high) {
    const int mid = (low + high) >> 1;
    const double mid_factor =
        calc_correction_factor(error_per_mb, ERR_DIVISOR - ediv_size_correction,
                               FACTOR_PT_LOW, FACTOR_PT_HIGH, mid, bit_depth);
    const int mid_bits_per_mb = av1_rc_bits_per_mb(
        frame_type, mid, mid_factor * group_weight_factor, bit_depth);
    if (mid_bits_per_mb > desired_bits_per_mb) {
      low = mid + 1;
    } else {
      high = mid;
    }
  }
#if CONFIG_DEBUG
  assert(low == high);
  const double low_factor =
      calc_correction_factor(error_per_mb, ERR_DIVISOR - ediv_size_correction,
                             FACTOR_PT_LOW, FACTOR_PT_HIGH, low, bit_depth);
  const int low_bits_per_mb = av1_rc_bits_per_mb(
      frame_type, low, low_factor * group_weight_factor, bit_depth);
  assert(low_bits_per_mb <= desired_bits_per_mb || low == worst_qindex);
#endif  // CONFIG_DEBUG
  return low;
}

static int get_twopass_worst_quality(const AV1_COMP *cpi,
                                     const double section_err,
                                     double inactive_zone,
                                     int section_target_bandwidth,
                                     double group_weight_factor) {
  const RATE_CONTROL *const rc = &cpi->rc;
  const AV1EncoderConfig *const oxcf = &cpi->oxcf;

  inactive_zone = fclamp(inactive_zone, 0.0, 1.0);

  if (section_target_bandwidth <= 0) {
    return rc->worst_quality;  // Highest value allowed
  } else {
    const int num_mbs = (cpi->oxcf.resize_mode != RESIZE_NONE)
                            ? cpi->initial_mbs
                            : cpi->common.MBs;
    const int active_mbs = AOMMAX(1, num_mbs - (int)(num_mbs * inactive_zone));
    const double av_err_per_mb = section_err / active_mbs;
    const int target_norm_bits_per_mb =
        (int)((uint64_t)section_target_bandwidth << BPER_MB_NORMBITS) /
        active_mbs;

    // Larger image formats are expected to be a little harder to code
    // relatively given the same prediction error score. This in part at
    // least relates to the increased size and hence coding overheads of
    // motion vectors. Some account of this is made through adjustment of
    // the error divisor.
    double ediv_size_correction =
        AOMMAX(0.2, AOMMIN(5.0, get_linear_size_factor(cpi)));
    if (ediv_size_correction < 1.0)
      ediv_size_correction = -(1.0 / ediv_size_correction);
    ediv_size_correction *= 4.0;

    // Try and pick a max Q that will be high enough to encode the
    // content at the given rate.
    int q = find_qindex_by_rate_with_correction(
        target_norm_bits_per_mb, cpi->common.seq_params.bit_depth, INTER_FRAME,
        av_err_per_mb, ediv_size_correction, group_weight_factor,
        rc->best_quality, rc->worst_quality);

    // Restriction on active max q for constrained quality mode.
    if (cpi->oxcf.rc_mode == AOM_CQ) q = AOMMAX(q, oxcf->cq_level);
    return q;
  }
}

#define SR_DIFF_PART 0.0015
#define MOTION_AMP_PART 0.003
#define INTRA_PART 0.005
#define DEFAULT_DECAY_LIMIT 0.75
#define LOW_SR_DIFF_TRHESH 0.1
#define SR_DIFF_MAX 128.0
#define NCOUNT_FRAME_II_THRESH 5.0

static double get_sr_decay_rate(const AV1_COMP *cpi,
                                const FIRSTPASS_STATS *frame) {
  const int num_mbs = (cpi->oxcf.resize_mode != RESIZE_NONE) ? cpi->initial_mbs
                                                             : cpi->common.MBs;
  double sr_diff = (frame->sr_coded_error - frame->coded_error) / num_mbs;
  double sr_decay = 1.0;
  double modified_pct_inter;
  double modified_pcnt_intra;
  const double motion_amplitude_factor =
      frame->pcnt_motion * ((frame->mvc_abs + frame->mvr_abs) / 2);

  modified_pct_inter = frame->pcnt_inter;
  if ((frame->intra_error / DOUBLE_DIVIDE_CHECK(frame->coded_error)) <
      (double)NCOUNT_FRAME_II_THRESH) {
    modified_pct_inter = frame->pcnt_inter - frame->pcnt_neutral;
  }
  modified_pcnt_intra = 100 * (1.0 - modified_pct_inter);

  if ((sr_diff > LOW_SR_DIFF_TRHESH)) {
    sr_diff = AOMMIN(sr_diff, SR_DIFF_MAX);
    sr_decay = 1.0 - (SR_DIFF_PART * sr_diff) -
               (MOTION_AMP_PART * motion_amplitude_factor) -
               (INTRA_PART * modified_pcnt_intra);
  }
  return AOMMAX(sr_decay, AOMMIN(DEFAULT_DECAY_LIMIT, modified_pct_inter));
}

// This function gives an estimate of how badly we believe the prediction
// quality is decaying from frame to frame.
static double get_zero_motion_factor(const AV1_COMP *cpi,
                                     const FIRSTPASS_STATS *frame) {
  const double zero_motion_pct = frame->pcnt_inter - frame->pcnt_motion;
  double sr_decay = get_sr_decay_rate(cpi, frame);
  return AOMMIN(sr_decay, zero_motion_pct);
}

#define ZM_POWER_FACTOR 0.75

static double get_prediction_decay_rate(const AV1_COMP *cpi,
                                        const FIRSTPASS_STATS *next_frame) {
  const double sr_decay_rate = get_sr_decay_rate(cpi, next_frame);
  const double zero_motion_factor =
      (0.95 * pow((next_frame->pcnt_inter - next_frame->pcnt_motion),
                  ZM_POWER_FACTOR));

  return AOMMAX(zero_motion_factor,
                (sr_decay_rate + ((1.0 - sr_decay_rate) * zero_motion_factor)));
}

// Function to test for a condition where a complex transition is followed
// by a static section. For example in slide shows where there is a fade
// between slides. This is to help with more optimal kf and gf positioning.
static int detect_transition_to_still(AV1_COMP *cpi, int frame_interval,
                                      int still_interval,
                                      double loop_decay_rate,
                                      double last_decay_rate) {
  TWO_PASS *const twopass = &cpi->twopass;
  RATE_CONTROL *const rc = &cpi->rc;

  // Break clause to detect very still sections after motion
  // For example a static image after a fade or other transition
  // instead of a clean scene cut.
  if (frame_interval > rc->min_gf_interval && loop_decay_rate >= 0.999 &&
      last_decay_rate < 0.9) {
    int j;

    // Look ahead a few frames to see if static condition persists...
    for (j = 0; j < still_interval; ++j) {
      const FIRSTPASS_STATS *stats = &twopass->stats_in[j];
      if (stats >= twopass->stats_in_end) break;

      if (stats->pcnt_inter - stats->pcnt_motion < 0.999) break;
    }

    // Only if it does do we signal a transition to still.
    return j == still_interval;
  }

  return 0;
}

// This function detects a flash through the high relative pcnt_second_ref
// score in the frame following a flash frame. The offset passed in should
// reflect this.
static int detect_flash(const TWO_PASS *twopass, int offset) {
  const FIRSTPASS_STATS *const next_frame = read_frame_stats(twopass, offset);

  // What we are looking for here is a situation where there is a
  // brief break in prediction (such as a flash) but subsequent frames
  // are reasonably well predicted by an earlier (pre flash) frame.
  // The recovery after a flash is indicated by a high pcnt_second_ref
  // compared to pcnt_inter.
  return next_frame != NULL &&
         next_frame->pcnt_second_ref > next_frame->pcnt_inter &&
         next_frame->pcnt_second_ref >= 0.5;
}

// Update the motion related elements to the GF arf boost calculation.
static void accumulate_frame_motion_stats(const FIRSTPASS_STATS *stats,
                                          double *mv_in_out,
                                          double *mv_in_out_accumulator,
                                          double *abs_mv_in_out_accumulator,
                                          double *mv_ratio_accumulator) {
  const double pct = stats->pcnt_motion;

  // Accumulate Motion In/Out of frame stats.
  *mv_in_out = stats->mv_in_out_count * pct;
  *mv_in_out_accumulator += *mv_in_out;
  *abs_mv_in_out_accumulator += fabs(*mv_in_out);

  // Accumulate a measure of how uniform (or conversely how random) the motion
  // field is (a ratio of abs(mv) / mv).
  if (pct > 0.05) {
    const double mvr_ratio =
        fabs(stats->mvr_abs) / DOUBLE_DIVIDE_CHECK(fabs(stats->MVr));
    const double mvc_ratio =
        fabs(stats->mvc_abs) / DOUBLE_DIVIDE_CHECK(fabs(stats->MVc));

    *mv_ratio_accumulator +=
        pct * (mvr_ratio < stats->mvr_abs ? mvr_ratio : stats->mvr_abs);
    *mv_ratio_accumulator +=
        pct * (mvc_ratio < stats->mvc_abs ? mvc_ratio : stats->mvc_abs);
  }
}

#define BASELINE_ERR_PER_MB 1000.0
#define BOOST_FACTOR 12.5

static double calc_frame_boost(AV1_COMP *cpi, const FIRSTPASS_STATS *this_frame,
                               double this_frame_mv_in_out, double max_boost) {
  double frame_boost;
  const double lq = av1_convert_qindex_to_q(
      cpi->rc.avg_frame_qindex[INTER_FRAME], cpi->common.seq_params.bit_depth);
  const double boost_q_correction = AOMMIN((0.5 + (lq * 0.015)), 1.5);
  int num_mbs = (cpi->oxcf.resize_mode != RESIZE_NONE) ? cpi->initial_mbs
                                                       : cpi->common.MBs;

  // Correct for any inactive region in the image
  num_mbs = (int)AOMMAX(1, num_mbs * calculate_active_area(cpi, this_frame));

  // Underlying boost factor is based on inter error ratio.
  frame_boost = (BASELINE_ERR_PER_MB * num_mbs) /
                DOUBLE_DIVIDE_CHECK(this_frame->coded_error);
  frame_boost = frame_boost * BOOST_FACTOR * boost_q_correction;

  // Increase boost for frames where new data coming into frame (e.g. zoom out).
  // Slightly reduce boost if there is a net balance of motion out of the frame
  // (zoom in). The range for this_frame_mv_in_out is -1.0 to +1.0.
  if (this_frame_mv_in_out > 0.0)
    frame_boost += frame_boost * (this_frame_mv_in_out * 2.0);
  // In the extreme case the boost is halved.
  else
    frame_boost += frame_boost * (this_frame_mv_in_out / 2.0);

  return AOMMIN(frame_boost, max_boost * boost_q_correction);
}

#define GF_MAX_BOOST 90.0
#define MIN_ARF_GF_BOOST 240
#define MIN_DECAY_FACTOR 0.01

static int calc_arf_boost(AV1_COMP *cpi, int offset, int f_frames, int b_frames,
                          int *f_boost, int *b_boost) {
  TWO_PASS *const twopass = &cpi->twopass;
  int i;
  double boost_score = 0.0;
  double mv_ratio_accumulator = 0.0;
  double decay_accumulator = 1.0;
  double this_frame_mv_in_out = 0.0;
  double mv_in_out_accumulator = 0.0;
  double abs_mv_in_out_accumulator = 0.0;
  int arf_boost;
  int flash_detected = 0;

  // Search forward from the proposed arf/next gf position.
  for (i = 0; i < f_frames; ++i) {
    const FIRSTPASS_STATS *this_frame = read_frame_stats(twopass, i + offset);
    if (this_frame == NULL) break;

    // Update the motion related elements to the boost calculation.
    accumulate_frame_motion_stats(
        this_frame, &this_frame_mv_in_out, &mv_in_out_accumulator,
        &abs_mv_in_out_accumulator, &mv_ratio_accumulator);

    // We want to discount the flash frame itself and the recovery
    // frame that follows as both will have poor scores.
    flash_detected = detect_flash(twopass, i + offset) ||
                     detect_flash(twopass, i + offset + 1);

    // Accumulate the effect of prediction quality decay.
    if (!flash_detected) {
      decay_accumulator *= get_prediction_decay_rate(cpi, this_frame);
      decay_accumulator = decay_accumulator < MIN_DECAY_FACTOR
                              ? MIN_DECAY_FACTOR
                              : decay_accumulator;
    }

    boost_score +=
        decay_accumulator *
        calc_frame_boost(cpi, this_frame, this_frame_mv_in_out, GF_MAX_BOOST);
  }

  *f_boost = (int)boost_score;

  // Reset for backward looking loop.
  boost_score = 0.0;
  mv_ratio_accumulator = 0.0;
  decay_accumulator = 1.0;
  this_frame_mv_in_out = 0.0;
  mv_in_out_accumulator = 0.0;
  abs_mv_in_out_accumulator = 0.0;

  // Search backward towards last gf position.
  for (i = -1; i >= -b_frames; --i) {
    const FIRSTPASS_STATS *this_frame = read_frame_stats(twopass, i + offset);
    if (this_frame == NULL) break;

    // Update the motion related elements to the boost calculation.
    accumulate_frame_motion_stats(
        this_frame, &this_frame_mv_in_out, &mv_in_out_accumulator,
        &abs_mv_in_out_accumulator, &mv_ratio_accumulator);

    // We want to discount the the flash frame itself and the recovery
    // frame that follows as both will have poor scores.
    flash_detected = detect_flash(twopass, i + offset) ||
                     detect_flash(twopass, i + offset + 1);

    // Cumulative effect of prediction quality decay.
    if (!flash_detected) {
      decay_accumulator *= get_prediction_decay_rate(cpi, this_frame);
      decay_accumulator = decay_accumulator < MIN_DECAY_FACTOR
                              ? MIN_DECAY_FACTOR
                              : decay_accumulator;
    }

    boost_score +=
        decay_accumulator *
        calc_frame_boost(cpi, this_frame, this_frame_mv_in_out, GF_MAX_BOOST);
  }
  *b_boost = (int)boost_score;

  arf_boost = (*f_boost + *b_boost);
  if (arf_boost < ((b_frames + f_frames) * 20))
    arf_boost = ((b_frames + f_frames) * 20);
  arf_boost = AOMMAX(arf_boost, MIN_ARF_GF_BOOST);

  return arf_boost;
}

// Calculate a section intra ratio used in setting max loop filter.
static int calculate_section_intra_ratio(const FIRSTPASS_STATS *begin,
                                         const FIRSTPASS_STATS *end,
                                         int section_length) {
  const FIRSTPASS_STATS *s = begin;
  double intra_error = 0.0;
  double coded_error = 0.0;
  int i = 0;

  while (s < end && i < section_length) {
    intra_error += s->intra_error;
    coded_error += s->coded_error;
    ++s;
    ++i;
  }

  return (int)(intra_error / DOUBLE_DIVIDE_CHECK(coded_error));
}

// Calculate the total bits to allocate in this GF/ARF group.
static int64_t calculate_total_gf_group_bits(AV1_COMP *cpi,
                                             double gf_group_err) {
  const RATE_CONTROL *const rc = &cpi->rc;
  const TWO_PASS *const twopass = &cpi->twopass;
  const int max_bits = frame_max_bits(rc, &cpi->oxcf);
  int64_t total_group_bits;

  // Calculate the bits to be allocated to the group as a whole.
  if ((twopass->kf_group_bits > 0) && (twopass->kf_group_error_left > 0)) {
    total_group_bits = (int64_t)(twopass->kf_group_bits *
                                 (gf_group_err / twopass->kf_group_error_left));
  } else {
    total_group_bits = 0;
  }

  // Clamp odd edge cases.
  total_group_bits = (total_group_bits < 0)
                         ? 0
                         : (total_group_bits > twopass->kf_group_bits)
                               ? twopass->kf_group_bits
                               : total_group_bits;

  // Clip based on user supplied data rate variability limit.
  if (total_group_bits > (int64_t)max_bits * rc->baseline_gf_interval)
    total_group_bits = (int64_t)max_bits * rc->baseline_gf_interval;

  return total_group_bits;
}

// Calculate the number bits extra to assign to boosted frames in a group.
static int calculate_boost_bits(int frame_count, int boost,
                                int64_t total_group_bits) {
  int allocation_chunks;

  // return 0 for invalid inputs (could arise e.g. through rounding errors)
  if (!boost || (total_group_bits <= 0) || (frame_count <= 0)) return 0;

  allocation_chunks = (frame_count * 100) + boost;

  // Prevent overflow.
  if (boost > 1023) {
    int divisor = boost >> 10;
    boost /= divisor;
    allocation_chunks /= divisor;
  }

  // Calculate the number of extra bits for use in the boosted frame or frames.
  return AOMMAX((int)(((int64_t)boost * total_group_bits) / allocation_chunks),
                0);
}

#define LEAF_REDUCTION_FACTOR 0.75
static double lvl_budget_factor[MAX_PYRAMID_LVL - 1][MAX_PYRAMID_LVL - 1] = {
  { 1.0, 0.0, 0.0 }, { 0.6, 0.4, 0 }, { 0.45, 0.35, 0.20 }
};
static void allocate_gf_group_bits(
    AV1_COMP *cpi, int64_t gf_group_bits, double group_error, int gf_arf_bits,
    const EncodeFrameParams *const frame_params) {
  RATE_CONTROL *const rc = &cpi->rc;
  const AV1EncoderConfig *const oxcf = &cpi->oxcf;
  TWO_PASS *const twopass = &cpi->twopass;
  GF_GROUP *const gf_group = &cpi->gf_group;
  const int key_frame = (frame_params->frame_type == KEY_FRAME);
  const int max_bits = frame_max_bits(&cpi->rc, &cpi->oxcf);
  int64_t total_group_bits = gf_group_bits;

  // Check if GF group has any internal arfs.
  int has_internal_arfs = 0;
  for (int i = 0; i < gf_group->size; ++i) {
    if (gf_group->update_type[i] == INTNL_ARF_UPDATE) {
      has_internal_arfs = 1;
      break;
    }
  }

  // For key frames the frame target rate is already set and it
  // is also the golden frame.
  // === [frame_index == 0] ===
  int frame_index = 0;
  if (!key_frame) {
    if (rc->source_alt_ref_active)
      gf_group->bit_allocation[frame_index] = 0;
    else
      gf_group->bit_allocation[frame_index] = gf_arf_bits;

    // Step over the golden frame / overlay frame
    FIRSTPASS_STATS frame_stats;
    if (EOF == input_stats(twopass, &frame_stats)) return;
  }

  // Deduct the boost bits for arf (or gf if it is not a key frame)
  // from the group total.
  if (rc->source_alt_ref_pending || !key_frame) total_group_bits -= gf_arf_bits;

  frame_index++;

  // Store the bits to spend on the ARF if there is one.
  // === [frame_index == 1] ===
  if (rc->source_alt_ref_pending) {
    gf_group->bit_allocation[frame_index] = gf_arf_bits;

    ++frame_index;

    // Skip all the internal ARFs right after ARF at the starting segment of
    // the current GF group.
    if (has_internal_arfs) {
      while (gf_group->update_type[frame_index] == INTNL_ARF_UPDATE) {
        ++frame_index;
      }
    }
  }

  // Save.
  const int tmp_frame_index = frame_index;
  int budget_reduced_from_leaf_level = 0;

  // Allocate bits to frames other than first frame, which is either a keyframe,
  // overlay frame or golden frame.
  const int normal_frames = rc->baseline_gf_interval - 1;

  for (int i = 0; i < normal_frames; ++i) {
    FIRSTPASS_STATS frame_stats;
    if (EOF == input_stats(twopass, &frame_stats)) break;

    const double modified_err =
        calculate_modified_err(cpi, twopass, oxcf, &frame_stats);
    const double err_fraction =
        (group_error > 0) ? modified_err / DOUBLE_DIVIDE_CHECK(group_error)
                          : 0.0;
    const int target_frame_size =
        clamp((int)((double)total_group_bits * err_fraction), 0,
              AOMMIN(max_bits, (int)total_group_bits));

    if (gf_group->update_type[frame_index] == INTNL_OVERLAY_UPDATE) {
      assert(gf_group->pyramid_height <= MAX_PYRAMID_LVL &&
             "non-valid height for a pyramid structure");

      const int arf_pos = gf_group->arf_pos_in_gf[frame_index];
      gf_group->bit_allocation[frame_index] = 0;

      gf_group->bit_allocation[arf_pos] = target_frame_size;
      // Note: Boost, if needed, is added in the next loop.
    } else {
      assert(gf_group->update_type[frame_index] == LF_UPDATE);
      gf_group->bit_allocation[frame_index] = target_frame_size;
      if (has_internal_arfs) {
        const int this_budget_reduction =
            (int)(target_frame_size * LEAF_REDUCTION_FACTOR);
        gf_group->bit_allocation[frame_index] -= this_budget_reduction;
        budget_reduced_from_leaf_level += this_budget_reduction;
      }
    }

    ++frame_index;

    // Skip all the internal ARFs.
    if (has_internal_arfs) {
      while (gf_group->update_type[frame_index] == INTNL_ARF_UPDATE)
        ++frame_index;
    }
  }

  if (budget_reduced_from_leaf_level > 0) {
    assert(has_internal_arfs);
    // Restore.
    frame_index = tmp_frame_index;

    // Re-distribute this extra budget to overlay frames in the group.
    for (int i = 0; i < normal_frames; ++i) {
      if (gf_group->update_type[frame_index] == INTNL_OVERLAY_UPDATE) {
        assert(gf_group->pyramid_height <= MAX_PYRAMID_LVL &&
               "non-valid height for a pyramid structure");
        const int arf_pos = gf_group->arf_pos_in_gf[frame_index];
        const int this_lvl = gf_group->pyramid_level[arf_pos];
        const int dist2top = gf_group->pyramid_height - 1 - this_lvl;
        const double lvl_boost_factor =
            lvl_budget_factor[gf_group->pyramid_height - 2][dist2top];
        const int extra_size =
            (int)(budget_reduced_from_leaf_level * lvl_boost_factor /
                  gf_group->pyramid_lvl_nodes[this_lvl]);
        gf_group->bit_allocation[arf_pos] += extra_size;
      }
      ++frame_index;

      // Skip all the internal ARFs.
      if (has_internal_arfs) {
        while (gf_group->update_type[frame_index] == INTNL_ARF_UPDATE) {
          ++frame_index;
        }
      }
    }
  }
}

// Given the maximum allowed height of the pyramid structure, return the fixed
// GF length to be used.
static INLINE int get_fixed_gf_length(int max_pyr_height) {
  (void)max_pyr_height;
  return MAX_GF_INTERVAL;
}

// Returns true if KF group and GF group both are almost completely static.
static INLINE int is_almost_static(double gf_zero_motion, int kf_zero_motion) {
  return (gf_zero_motion >= 0.995) &&
         (kf_zero_motion >= STATIC_KF_GROUP_THRESH);
}

#define ARF_ABS_ZOOM_THRESH 4.4
#define GROUP_ADAPTIVE_MAXQ 1
#if GROUP_ADAPTIVE_MAXQ
#define RC_FACTOR_MIN 0.75
#define RC_FACTOR_MAX 1.75
#endif  // GROUP_ADAPTIVE_MAXQ
#define MIN_FWD_KF_INTERVAL 8

void av1_assign_q_and_bounds_q_mode(AV1_COMP *cpi) {
  AV1_COMMON *const cm = &cpi->common;
  GF_GROUP *const gf_group = &cpi->gf_group;
  const int width = cm->width;
  const int height = cm->height;
  RATE_CONTROL *const rc = &cpi->rc;
  const int last_boosted_q = rc->last_boosted_qindex;
  const int last_kf_q = rc->last_kf_qindex;
  const int avg_frame_qindex = rc->avg_frame_qindex[INTER_FRAME];
  int q;

  for (int cur_index = 0; cur_index < gf_group->size; ++cur_index) {
    const FRAME_UPDATE_TYPE cur_update_type = gf_group->update_type[cur_index];
    int arf_q = -1;  // Initialize to invalid value, for sanity check later.

    q = av1_estimate_q_constant_quality_two_pass(cpi, width, height, &arf_q,
                                                 cur_index);
    if (cur_update_type == ARF_UPDATE) {
      cpi->rc.arf_q = arf_q;
    }
    gf_group->q_val[cur_index] = q;

    // Update the rate control state necessary to accuratly compute q for
    // the next frames.
    // This is used to help set quality in forced key frames to reduce popping
    if ((q < rc->last_boosted_qindex) || (cur_update_type == KF_UPDATE) ||
        (!rc->constrained_gf_group && (cur_update_type == ARF_UPDATE ||
                                       cur_update_type == INTNL_ARF_UPDATE ||
                                       cur_update_type == GF_UPDATE))) {
      rc->last_boosted_qindex = q;
    }
    // TODO(sarahparker) Investigate whether or not this is actually needed
    if (cur_update_type == LF_UPDATE)
      rc->avg_frame_qindex[INTER_FRAME] =
          ROUND_POWER_OF_TWO(3 * rc->avg_frame_qindex[INTER_FRAME] + q, 2);
    if (cur_update_type == KF_UPDATE) rc->last_kf_qindex = q;
  }
  // Reset all of the modified state to the original values.
  rc->last_boosted_qindex = last_boosted_q;
  rc->last_kf_qindex = last_kf_q;
  rc->avg_frame_qindex[INTER_FRAME] = avg_frame_qindex;
}

static int calc_pframe_target_size_one_pass_vbr(
    const AV1_COMP *const cpi, FRAME_UPDATE_TYPE frame_update_type) {
  static const int af_ratio = 10;
  const RATE_CONTROL *const rc = &cpi->rc;
  int target;
#if USE_ALTREF_FOR_ONE_PASS
  if (frame_update_type == KF_UPDATE || frame_update_type == GF_UPDATE ||
      frame_update_type == ARF_UPDATE) {
    target = (rc->avg_frame_bandwidth * rc->baseline_gf_interval * af_ratio) /
             (rc->baseline_gf_interval + af_ratio - 1);
  } else {
    target = (rc->avg_frame_bandwidth * rc->baseline_gf_interval) /
             (rc->baseline_gf_interval + af_ratio - 1);
  }
#else
  target = rc->avg_frame_bandwidth;
#endif
  return av1_rc_clamp_pframe_target_size(cpi, target, frame_update_type);
}

static int calc_iframe_target_size_one_pass_vbr(const AV1_COMP *const cpi) {
  static const int kf_ratio = 25;
  const RATE_CONTROL *rc = &cpi->rc;
  const int target = rc->avg_frame_bandwidth * kf_ratio;
  return av1_rc_clamp_iframe_target_size(cpi, target);
}

#define FRAME_OVERHEAD_BITS 200

static int calc_pframe_target_size_one_pass_cbr(
    const AV1_COMP *cpi, FRAME_UPDATE_TYPE frame_update_type) {
  const AV1EncoderConfig *oxcf = &cpi->oxcf;
  const RATE_CONTROL *rc = &cpi->rc;
  const int64_t diff = rc->optimal_buffer_level - rc->buffer_level;
  const int64_t one_pct_bits = 1 + rc->optimal_buffer_level / 100;
  int min_frame_target =
      AOMMAX(rc->avg_frame_bandwidth >> 4, FRAME_OVERHEAD_BITS);
  int target;

  if (oxcf->gf_cbr_boost_pct) {
    const int af_ratio_pct = oxcf->gf_cbr_boost_pct + 100;
    if (frame_update_type == GF_UPDATE || frame_update_type == OVERLAY_UPDATE) {
      target =
          (rc->avg_frame_bandwidth * rc->baseline_gf_interval * af_ratio_pct) /
          (rc->baseline_gf_interval * 100 + af_ratio_pct - 100);
    } else {
      target = (rc->avg_frame_bandwidth * rc->baseline_gf_interval * 100) /
               (rc->baseline_gf_interval * 100 + af_ratio_pct - 100);
    }
  } else {
    target = rc->avg_frame_bandwidth;
  }

  if (diff > 0) {
    // Lower the target bandwidth for this frame.
    const int pct_low = (int)AOMMIN(diff / one_pct_bits, oxcf->under_shoot_pct);
    target -= (target * pct_low) / 200;
  } else if (diff < 0) {
    // Increase the target bandwidth for this frame.
    const int pct_high =
        (int)AOMMIN(-diff / one_pct_bits, oxcf->over_shoot_pct);
    target += (target * pct_high) / 200;
  }
  if (oxcf->rc_max_inter_bitrate_pct) {
    const int max_rate =
        rc->avg_frame_bandwidth * oxcf->rc_max_inter_bitrate_pct / 100;
    target = AOMMIN(target, max_rate);
  }
  return AOMMAX(min_frame_target, target);
}

static int calc_iframe_target_size_one_pass_cbr(const AV1_COMP *cpi) {
  const RATE_CONTROL *rc = &cpi->rc;
  int target;
  if (cpi->common.current_frame.frame_number == 0) {
    target = ((rc->starting_buffer_level / 2) > INT_MAX)
                 ? INT_MAX
                 : (int)(rc->starting_buffer_level / 2);
  } else {
    int kf_boost = 32;
    double framerate = cpi->framerate;

    kf_boost = AOMMAX(kf_boost, (int)(2 * framerate - 16));
    if (rc->frames_since_key < framerate / 2) {
      kf_boost = (int)(kf_boost * rc->frames_since_key / (framerate / 2));
    }
    target = ((16 + kf_boost) * rc->avg_frame_bandwidth) >> 4;
  }
  return av1_rc_clamp_iframe_target_size(cpi, target);
}

static void define_gf_group_pass0(AV1_COMP *cpi,
                                  const EncodeFrameParams *const frame_params) {
  RATE_CONTROL *const rc = &cpi->rc;
  GF_GROUP *const gf_group = &cpi->gf_group;
  int target;

  if (cpi->oxcf.aq_mode == CYCLIC_REFRESH_AQ)
    av1_cyclic_refresh_set_golden_update(cpi);
  else
    rc->baseline_gf_interval = MAX_GF_INTERVAL;

  if (rc->baseline_gf_interval > rc->frames_to_key)
    rc->baseline_gf_interval = rc->frames_to_key;

  rc->gfu_boost = DEFAULT_GF_BOOST;
  rc->constrained_gf_group =
      (rc->baseline_gf_interval >= rc->frames_to_key) ? 1 : 0;
  const int use_alt_ref =
      is_altref_enabled(cpi) &&
      (rc->baseline_gf_interval < cpi->oxcf.lag_in_frames) &&
      !(cpi->lookahead->sz < rc->baseline_gf_interval - 1) &&
      (cpi->oxcf.gf_max_pyr_height > MIN_PYRAMID_LVL);
  rc->source_alt_ref_pending = use_alt_ref;
  cpi->preserve_arf_as_gld = use_alt_ref;

  // Set up the structure of this Group-Of-Pictures (same as GF_GROUP)
  av1_gop_setup_structure(cpi, frame_params);

  if (cpi->oxcf.rc_mode == AOM_Q) av1_assign_q_and_bounds_q_mode(cpi);

  // Allocate bits to each of the frames in the GF group.
  // TODO(sarahparker) Extend this to work with pyramid structure.
  for (int cur_index = 0; cur_index < gf_group->size; ++cur_index) {
    const FRAME_UPDATE_TYPE cur_update_type = gf_group->update_type[cur_index];
    if (cpi->oxcf.rc_mode == AOM_CBR) {
      if (cur_update_type == KEY_FRAME) {
        target = calc_iframe_target_size_one_pass_cbr(cpi);
      } else {
        target = calc_pframe_target_size_one_pass_cbr(cpi, cur_update_type);
      }
    } else {
      if (cur_update_type == KEY_FRAME) {
        target = calc_iframe_target_size_one_pass_vbr(cpi);
      } else {
        target = calc_pframe_target_size_one_pass_vbr(cpi, cur_update_type);
      }
    }
    gf_group->bit_allocation[cur_index] = target;
  }
}

// Analyse and define a gf/arf group.
static void define_gf_group(AV1_COMP *cpi, FIRSTPASS_STATS *this_frame,
                            const EncodeFrameParams *const frame_params) {
  AV1_COMMON *const cm = &cpi->common;
  RATE_CONTROL *const rc = &cpi->rc;
  AV1EncoderConfig *const oxcf = &cpi->oxcf;
  TWO_PASS *const twopass = &cpi->twopass;
  FIRSTPASS_STATS next_frame;
  const FIRSTPASS_STATS *const start_pos = twopass->stats_in;
  int i;

  double boost_score = 0.0;
  double gf_group_err = 0.0;
#if GROUP_ADAPTIVE_MAXQ
  double gf_group_raw_error = 0.0;
#endif
  double gf_group_skip_pct = 0.0;
  double gf_group_inactive_zone_rows = 0.0;
  double gf_first_frame_err = 0.0;
  double mod_frame_err = 0.0;

  double mv_ratio_accumulator = 0.0;
  double decay_accumulator = 1.0;
  double zero_motion_accumulator = 1.0;

  double loop_decay_rate = 1.00;
  double last_loop_decay_rate = 1.00;

  double this_frame_mv_in_out = 0.0;
  double mv_in_out_accumulator = 0.0;
  double abs_mv_in_out_accumulator = 0.0;

  unsigned int allow_alt_ref = is_altref_enabled(cpi);

  int f_boost = 0;
  int b_boost = 0;
  int flash_detected;
  int64_t gf_group_bits;
  double gf_group_error_left;
  int gf_arf_bits;
  const int is_intra_only = frame_params->frame_type == KEY_FRAME ||
                            frame_params->frame_type == INTRA_ONLY_FRAME;
  const int arf_active_or_kf = is_intra_only || rc->source_alt_ref_active;

  cpi->internal_altref_allowed = (oxcf->gf_max_pyr_height > 1);

  // Reset the GF group data structures unless this is a key
  // frame in which case it will already have been done.
  if (!is_intra_only) {
    av1_zero(cpi->gf_group);
  }

  aom_clear_system_state();
  av1_zero(next_frame);

  if (oxcf->pass == 0) {
    define_gf_group_pass0(cpi, frame_params);
    return;
  }

  // Load stats for the current frame.
  mod_frame_err = calculate_modified_err(cpi, twopass, oxcf, this_frame);

  // Note the error of the frame at the start of the group. This will be
  // the GF frame error if we code a normal gf.
  gf_first_frame_err = mod_frame_err;

  const double first_frame_coded_error = this_frame->coded_error;
  const double first_frame_sr_coded_error = this_frame->sr_coded_error;
  const double first_frame_tr_coded_error = this_frame->tr_coded_error;

  // If this is a key frame or the overlay from a previous arf then
  // the error score / cost of this frame has already been accounted for.
  if (arf_active_or_kf) {
    gf_group_err -= gf_first_frame_err;
#if GROUP_ADAPTIVE_MAXQ
    gf_group_raw_error -= this_frame->coded_error;
#endif
    gf_group_skip_pct -= this_frame->intra_skip_pct;
    gf_group_inactive_zone_rows -= this_frame->inactive_zone_rows;
  }
  // Motion breakout threshold for loop below depends on image size.
  const double mv_ratio_accumulator_thresh =
      (cpi->initial_height + cpi->initial_width) / 4.0;

  // TODO(urvang): Try logic to vary min and max interval based on q.
  const int active_min_gf_interval = rc->min_gf_interval;
  const int active_max_gf_interval =
      AOMMIN(rc->max_gf_interval, get_fixed_gf_length(oxcf->gf_max_pyr_height));

  double avg_sr_coded_error = 0;
  double avg_tr_coded_error = 0;

  double avg_pcnt_second_ref = 0;
  double avg_pcnt_third_ref = 0;

  double avg_new_mv_count = 0;

  double avg_wavelet_energy = 0;

  double avg_raw_err_stdev = 0;
  int non_zero_stdev_count = 0;

  i = 0;
  while (i < rc->static_scene_max_gf_interval && i < rc->frames_to_key) {
    ++i;

    // Accumulate error score of frames in this gf group.
    mod_frame_err = calculate_modified_err(cpi, twopass, oxcf, this_frame);
    gf_group_err += mod_frame_err;
#if GROUP_ADAPTIVE_MAXQ
    gf_group_raw_error += this_frame->coded_error;
#endif
    gf_group_skip_pct += this_frame->intra_skip_pct;
    gf_group_inactive_zone_rows += this_frame->inactive_zone_rows;

    if (EOF == input_stats(twopass, &next_frame)) break;

    // Test for the case where there is a brief flash but the prediction
    // quality back to an earlier frame is then restored.
    flash_detected = detect_flash(twopass, 0);

    // Update the motion related elements to the boost calculation.
    accumulate_frame_motion_stats(
        &next_frame, &this_frame_mv_in_out, &mv_in_out_accumulator,
        &abs_mv_in_out_accumulator, &mv_ratio_accumulator);
    // sum up the metric values of current gf group
    avg_sr_coded_error += next_frame.sr_coded_error;
    avg_tr_coded_error += next_frame.tr_coded_error;
    avg_pcnt_second_ref += next_frame.pcnt_second_ref;
    avg_pcnt_third_ref += next_frame.pcnt_third_ref;
    avg_new_mv_count += next_frame.new_mv_count;
    avg_wavelet_energy += next_frame.frame_avg_wavelet_energy;
    if (fabs(next_frame.raw_error_stdev) > 0.000001) {
      non_zero_stdev_count++;
      avg_raw_err_stdev += next_frame.raw_error_stdev;
    }

    // Accumulate the effect of prediction quality decay.
    if (!flash_detected) {
      last_loop_decay_rate = loop_decay_rate;
      loop_decay_rate = get_prediction_decay_rate(cpi, &next_frame);

      decay_accumulator = decay_accumulator * loop_decay_rate;

      // Monitor for static sections.
      if ((rc->frames_since_key + i - 1) > 1) {
        zero_motion_accumulator = AOMMIN(
            zero_motion_accumulator, get_zero_motion_factor(cpi, &next_frame));
      }

      // Break clause to detect very still sections after motion. For example,
      // a static image after a fade or other transition.
      if (detect_transition_to_still(cpi, i, 5, loop_decay_rate,
                                     last_loop_decay_rate)) {
        allow_alt_ref = 0;
        break;
      }
    }

    // Calculate a boost number for this frame.
    boost_score +=
        decay_accumulator *
        calc_frame_boost(cpi, &next_frame, this_frame_mv_in_out, GF_MAX_BOOST);
    // If almost totally static, we will not use the the max GF length later,
    // so we can continue for more frames.
    if ((i >= active_max_gf_interval + 1) &&
        !is_almost_static(zero_motion_accumulator,
                          twopass->kf_zeromotion_pct)) {
      break;
    }

    // Some conditions to breakout after min interval.
    if (i >= active_min_gf_interval &&
        // If possible don't break very close to a kf
        (rc->frames_to_key - i >= rc->min_gf_interval) && (i & 0x01) &&
        !flash_detected &&
        (mv_ratio_accumulator > mv_ratio_accumulator_thresh ||
         abs_mv_in_out_accumulator > ARF_ABS_ZOOM_THRESH)) {
      break;
    }
    *this_frame = next_frame;
  }

  // Was the group length constrained by the requirement for a new KF?
  rc->constrained_gf_group = (i >= rc->frames_to_key) ? 1 : 0;

  const int num_mbs = (cpi->oxcf.resize_mode != RESIZE_NONE) ? cpi->initial_mbs
                                                             : cpi->common.MBs;
  assert(num_mbs > 0);
  const double last_frame_coded_error = next_frame.coded_error;
  const double last_frame_sr_coded_error = next_frame.sr_coded_error;
  const double last_frame_tr_coded_error = next_frame.tr_coded_error;
  double avg_pcnt_third_ref_nolast = avg_pcnt_third_ref;
  if (i) {
    avg_sr_coded_error /= i;
    avg_tr_coded_error /= i;
    avg_pcnt_second_ref /= i;
    if (i - 1) {
      avg_pcnt_third_ref_nolast =
          (avg_pcnt_third_ref - next_frame.pcnt_third_ref) / (i - 1);
    } else {
      avg_pcnt_third_ref_nolast = avg_pcnt_third_ref / i;
    }
    avg_pcnt_third_ref /= i;
    avg_new_mv_count /= i;
    avg_wavelet_energy /= i;
  }

  if (non_zero_stdev_count) avg_raw_err_stdev /= non_zero_stdev_count;

  // Disable internal ARFs for "still" gf groups.
  //   zero_motion_accumulator: minimum percentage of (0,0) motion;
  //   avg_sr_coded_error:      average of the SSE per pixel of each frame;
  //   avg_raw_err_stdev:       average of the standard deviation of (0,0)
  //                            motion error per block of each frame.
  if (zero_motion_accumulator > MIN_ZERO_MOTION &&
      avg_sr_coded_error / num_mbs < MAX_SR_CODED_ERROR &&
      avg_raw_err_stdev < MAX_RAW_ERR_VAR) {
    cpi->internal_altref_allowed = 0;
  }

  int use_alt_ref =
      !is_almost_static(zero_motion_accumulator, twopass->kf_zeromotion_pct) &&
      allow_alt_ref && (i < cpi->oxcf.lag_in_frames) &&
      (i >= rc->min_gf_interval) &&
      (cpi->oxcf.gf_max_pyr_height > MIN_PYRAMID_LVL);

  // TODO(urvang): Improve and use model for VBR, CQ etc as well.
  if (use_alt_ref && cpi->oxcf.rc_mode == AOM_Q && cpi->oxcf.cq_level <= 200) {
    aom_clear_system_state();

    /* clang-format off */
    // Generate features.
    const float features[] = {
      (float)abs_mv_in_out_accumulator,
      (float)(avg_new_mv_count / num_mbs),
      (float)avg_pcnt_second_ref,
      (float)avg_pcnt_third_ref,
      (float)avg_pcnt_third_ref_nolast,
      (float)(avg_sr_coded_error / num_mbs),
      (float)(avg_tr_coded_error / num_mbs),
      (float)(avg_wavelet_energy / num_mbs),
      (float)(rc->constrained_gf_group),
      (float)decay_accumulator,
      (float)(first_frame_coded_error / num_mbs),
      (float)(first_frame_sr_coded_error / num_mbs),
      (float)(first_frame_tr_coded_error / num_mbs),
      (float)(gf_first_frame_err / num_mbs),
      (float)(twopass->kf_zeromotion_pct),
      (float)(last_frame_coded_error / num_mbs),
      (float)(last_frame_sr_coded_error / num_mbs),
      (float)(last_frame_tr_coded_error / num_mbs),
      (float)i,
      (float)mv_ratio_accumulator,
      (float)non_zero_stdev_count
    };
    /* clang-format on */
    // Infer using ML model.
    float score;
    av1_nn_predict(features, &av1_use_flat_gop_nn_config, &score);
    use_alt_ref = (score <= 0.0);
  }

#define REDUCE_GF_LENGTH_THRESH 4
#define REDUCE_GF_LENGTH_TO_KEY_THRESH 9
#define REDUCE_GF_LENGTH_BY 1
  int alt_offset = 0;
  // The length reduction strategy is tweaked for certain cases, and doesn't
  // work well for certain other cases.
  const int allow_gf_length_reduction =
      ((cpi->oxcf.rc_mode == AOM_Q && cpi->oxcf.cq_level <= 128) ||
       !cpi->internal_altref_allowed) &&
      !is_lossless_requested(&cpi->oxcf);

  if (allow_gf_length_reduction && use_alt_ref) {
    // adjust length of this gf group if one of the following condition met
    // 1: only one overlay frame left and this gf is too long
    // 2: next gf group is too short to have arf compared to the current gf

    // maximum length of next gf group
    const int next_gf_len = rc->frames_to_key - i;
    const int single_overlay_left =
        next_gf_len == 0 && i > REDUCE_GF_LENGTH_THRESH;
    // the next gf is probably going to have a ARF but it will be shorter than
    // this gf
    const int unbalanced_gf =
        i > REDUCE_GF_LENGTH_TO_KEY_THRESH &&
        next_gf_len + 1 < REDUCE_GF_LENGTH_TO_KEY_THRESH &&
        next_gf_len + 1 >= rc->min_gf_interval;

    if (single_overlay_left || unbalanced_gf) {
      const int roll_back = REDUCE_GF_LENGTH_BY;
      // Reduce length only if active_min_gf_interval will be respected later.
      if (i - roll_back >= active_min_gf_interval + 1) {
        alt_offset = -roll_back;
        i -= roll_back;
      }
    }
  }

  // Should we use the alternate reference frame.
  if (use_alt_ref) {
    // Calculate the boost for alt ref.
    rc->gfu_boost =
        calc_arf_boost(cpi, alt_offset, (i - 1), (i - 1), &f_boost, &b_boost);
    rc->source_alt_ref_pending = 1;

    // do not replace ARFs with overlay frames, and keep it as GOLDEN_REF
    cpi->preserve_arf_as_gld = 1;
  } else {
    rc->gfu_boost = AOMMAX((int)boost_score, MIN_ARF_GF_BOOST);
    rc->source_alt_ref_pending = 0;
    cpi->preserve_arf_as_gld = 0;
  }

  // Set the interval until the next gf.
  // If forward keyframes are enabled, ensure the final gf group obeys the
  // MIN_FWD_KF_INTERVAL.
  if (cpi->oxcf.fwd_kf_enabled &&
      ((twopass->stats_in - i + rc->frames_to_key) < twopass->stats_in_end)) {
    if (i == rc->frames_to_key) {
      rc->baseline_gf_interval = i;
      // if the last gf group will be smaller than MIN_FWD_KF_INTERVAL
    } else if ((rc->frames_to_key - i <
                AOMMAX(MIN_FWD_KF_INTERVAL, rc->min_gf_interval)) &&
               (rc->frames_to_key != i)) {
      // if possible, merge the last two gf groups
      if (rc->frames_to_key <= active_max_gf_interval) {
        rc->baseline_gf_interval = rc->frames_to_key;
        // if merging the last two gf groups creates a group that is too long,
        // split them and force the last gf group to be the MIN_FWD_KF_INTERVAL
      } else {
        rc->baseline_gf_interval = rc->frames_to_key - MIN_FWD_KF_INTERVAL;
      }
    } else {
      rc->baseline_gf_interval = i - rc->source_alt_ref_pending;
    }
  } else {
    rc->baseline_gf_interval = i - rc->source_alt_ref_pending;
  }

#define LAST_ALR_BOOST_FACTOR 0.2f
  rc->arf_boost_factor = 1.0;
  if (rc->source_alt_ref_pending && !is_lossless_requested(&cpi->oxcf)) {
    // Reduce the boost of altref in the last gf group
    if (rc->frames_to_key - i == REDUCE_GF_LENGTH_BY ||
        rc->frames_to_key - i == 0) {
      rc->arf_boost_factor = LAST_ALR_BOOST_FACTOR;
    }
  }

  rc->frames_till_gf_update_due = rc->baseline_gf_interval;

  // Reset the file position.
  reset_fpf_position(twopass, start_pos);

  // Calculate the bits to be allocated to the gf/arf group as a whole
  gf_group_bits = calculate_total_gf_group_bits(cpi, gf_group_err);

#if GROUP_ADAPTIVE_MAXQ
  // Calculate an estimate of the maxq needed for the group.
  // We are more agressive about correcting for sections
  // where there could be significant overshoot than for easier
  // sections where we do not wish to risk creating an overshoot
  // of the allocated bit budget.
  if ((cpi->oxcf.rc_mode != AOM_Q) && (rc->baseline_gf_interval > 1)) {
    const int vbr_group_bits_per_frame =
        (int)(gf_group_bits / rc->baseline_gf_interval);
    const double group_av_err = gf_group_raw_error / rc->baseline_gf_interval;
    const double group_av_skip_pct =
        gf_group_skip_pct / rc->baseline_gf_interval;
    const double group_av_inactive_zone =
        ((gf_group_inactive_zone_rows * 2) /
         (rc->baseline_gf_interval * (double)cm->mb_rows));

    int tmp_q;
    // rc factor is a weight factor that corrects for local rate control drift.
    double rc_factor = 1.0;
    if (rc->rate_error_estimate > 0) {
      rc_factor = AOMMAX(RC_FACTOR_MIN,
                         (double)(100 - rc->rate_error_estimate) / 100.0);
    } else {
      rc_factor = AOMMIN(RC_FACTOR_MAX,
                         (double)(100 - rc->rate_error_estimate) / 100.0);
    }
    tmp_q = get_twopass_worst_quality(
        cpi, group_av_err, (group_av_skip_pct + group_av_inactive_zone),
        vbr_group_bits_per_frame, twopass->kfgroup_inter_fraction * rc_factor);
    rc->active_worst_quality = AOMMAX(tmp_q, rc->active_worst_quality >> 1);
  }
#endif

  // Calculate the extra bits to be used for boosted frame(s)
  gf_arf_bits = calculate_boost_bits(rc->baseline_gf_interval, rc->gfu_boost,
                                     gf_group_bits);

  // Adjust KF group bits and error remaining.
  twopass->kf_group_error_left -= (int64_t)gf_group_err;

  // If this is an arf update we want to remove the score for the overlay
  // frame at the end which will usually be very cheap to code.
  // The overlay frame has already, in effect, been coded so we want to spread
  // the remaining bits among the other frames.
  // For normal GFs remove the score for the GF itself unless this is
  // also a key frame in which case it has already been accounted for.
  if (rc->source_alt_ref_pending) {
    gf_group_error_left = gf_group_err - mod_frame_err;
  } else if (!is_intra_only) {
    gf_group_error_left = gf_group_err - gf_first_frame_err;
  } else {
    gf_group_error_left = gf_group_err;
  }

  // Set up the structure of this Group-Of-Pictures (same as GF_GROUP)
  av1_gop_setup_structure(cpi, frame_params);

  if (cpi->oxcf.rc_mode == AOM_Q) av1_assign_q_and_bounds_q_mode(cpi);

  // Allocate bits to each of the frames in the GF group.
  allocate_gf_group_bits(cpi, gf_group_bits, gf_group_error_left, gf_arf_bits,
                         frame_params);

  // Reset the file position.
  reset_fpf_position(twopass, start_pos);

  // Calculate a section intra ratio used in setting max loop filter.
  if (frame_params->frame_type != KEY_FRAME) {
    twopass->section_intra_rating = calculate_section_intra_ratio(
        start_pos, twopass->stats_in_end, rc->baseline_gf_interval);
  }
}

// Minimum % intra coding observed in first pass (1.0 = 100%)
#define MIN_INTRA_LEVEL 0.25
// Minimum ratio between the % of intra coding and inter coding in the first
// pass after discounting neutral blocks (discounting neutral blocks in this
// way helps catch scene cuts in clips with very flat areas or letter box
// format clips with image padding.
#define INTRA_VS_INTER_THRESH 2.0
// Hard threshold where the first pass chooses intra for almost all blocks.
// In such a case even if the frame is not a scene cut coding a key frame
// may be a good option.
#define VERY_LOW_INTER_THRESH 0.05
// Maximum threshold for the relative ratio of intra error score vs best
// inter error score.
#define KF_II_ERR_THRESHOLD 2.5
// In real scene cuts there is almost always a sharp change in the intra
// or inter error score.
#define ERR_CHANGE_THRESHOLD 0.4
// For real scene cuts we expect an improvment in the intra inter error
// ratio in the next frame.
#define II_IMPROVEMENT_THRESHOLD 3.5
#define KF_II_MAX 128.0

// Threshold for use of the lagging second reference frame. High second ref
// usage may point to a transient event like a flash or occlusion rather than
// a real scene cut.
// We adapt the threshold based on number of frames in this key-frame group so
// far.
static double get_second_ref_usage_thresh(int frame_count_so_far) {
  const int adapt_upto = 32;
  const double min_second_ref_usage_thresh = 0.085;
  const double second_ref_usage_thresh_max_delta = 0.035;
  if (frame_count_so_far >= adapt_upto) {
    return min_second_ref_usage_thresh + second_ref_usage_thresh_max_delta;
  }
  return min_second_ref_usage_thresh +
         ((double)frame_count_so_far / (adapt_upto - 1)) *
             second_ref_usage_thresh_max_delta;
}

static int test_candidate_kf(TWO_PASS *twopass,
                             const FIRSTPASS_STATS *last_frame,
                             const FIRSTPASS_STATS *this_frame,
                             const FIRSTPASS_STATS *next_frame,
                             int frame_count_so_far) {
  int is_viable_kf = 0;
  double pcnt_intra = 1.0 - this_frame->pcnt_inter;
  double modified_pcnt_inter =
      this_frame->pcnt_inter - this_frame->pcnt_neutral;
  const double second_ref_usage_thresh =
      get_second_ref_usage_thresh(frame_count_so_far);

  // Does the frame satisfy the primary criteria of a key frame?
  // See above for an explanation of the test criteria.
  // If so, then examine how well it predicts subsequent frames.
  if ((this_frame->pcnt_second_ref < second_ref_usage_thresh) &&
      (next_frame->pcnt_second_ref < second_ref_usage_thresh) &&
      ((this_frame->pcnt_inter < VERY_LOW_INTER_THRESH) ||
       ((pcnt_intra > MIN_INTRA_LEVEL) &&
        (pcnt_intra > (INTRA_VS_INTER_THRESH * modified_pcnt_inter)) &&
        ((this_frame->intra_error /
          DOUBLE_DIVIDE_CHECK(this_frame->coded_error)) <
         KF_II_ERR_THRESHOLD) &&
        ((fabs(last_frame->coded_error - this_frame->coded_error) /
              DOUBLE_DIVIDE_CHECK(this_frame->coded_error) >
          ERR_CHANGE_THRESHOLD) ||
         (fabs(last_frame->intra_error - this_frame->intra_error) /
              DOUBLE_DIVIDE_CHECK(this_frame->intra_error) >
          ERR_CHANGE_THRESHOLD) ||
         ((next_frame->intra_error /
           DOUBLE_DIVIDE_CHECK(next_frame->coded_error)) >
          II_IMPROVEMENT_THRESHOLD))))) {
    int i;
    const FIRSTPASS_STATS *start_pos = twopass->stats_in;
    FIRSTPASS_STATS local_next_frame = *next_frame;
    double boost_score = 0.0;
    double old_boost_score = 0.0;
    double decay_accumulator = 1.0;

    // Examine how well the key frame predicts subsequent frames.
    for (i = 0; i < 16; ++i) {
      double next_iiratio = (BOOST_FACTOR * local_next_frame.intra_error /
                             DOUBLE_DIVIDE_CHECK(local_next_frame.coded_error));

      if (next_iiratio > KF_II_MAX) next_iiratio = KF_II_MAX;

      // Cumulative effect of decay in prediction quality.
      if (local_next_frame.pcnt_inter > 0.85)
        decay_accumulator *= local_next_frame.pcnt_inter;
      else
        decay_accumulator *= (0.85 + local_next_frame.pcnt_inter) / 2.0;

      // Keep a running total.
      boost_score += (decay_accumulator * next_iiratio);

      // Test various breakout clauses.
      if ((local_next_frame.pcnt_inter < 0.05) || (next_iiratio < 1.5) ||
          (((local_next_frame.pcnt_inter - local_next_frame.pcnt_neutral) <
            0.20) &&
           (next_iiratio < 3.0)) ||
          ((boost_score - old_boost_score) < 3.0) ||
          (local_next_frame.intra_error < 200)) {
        break;
      }

      old_boost_score = boost_score;

      // Get the next frame details
      if (EOF == input_stats(twopass, &local_next_frame)) break;
    }

    // If there is tolerable prediction for at least the next 3 frames then
    // break out else discard this potential key frame and move on
    if (boost_score > 30.0 && (i > 3)) {
      is_viable_kf = 1;
    } else {
      // Reset the file position
      reset_fpf_position(twopass, start_pos);

      is_viable_kf = 0;
    }
  }

  return is_viable_kf;
}

#define FRAMES_TO_CHECK_DECAY 8
#define KF_MIN_FRAME_BOOST 80.0
#define KF_MAX_FRAME_BOOST 128.0
#define MIN_KF_BOOST 300          // Minimum boost for non-static KF interval
#define MIN_STATIC_KF_BOOST 5400  // Minimum boost for static KF interval

static void find_next_key_frame(AV1_COMP *cpi, FIRSTPASS_STATS *this_frame) {
  RATE_CONTROL *const rc = &cpi->rc;
  TWO_PASS *const twopass = &cpi->twopass;
  GF_GROUP *const gf_group = &cpi->gf_group;
  AV1_COMMON *const cm = &cpi->common;
  CurrentFrame *const current_frame = &cm->current_frame;
  const AV1EncoderConfig *const oxcf = &cpi->oxcf;
  const FIRSTPASS_STATS first_frame = *this_frame;
  FIRSTPASS_STATS next_frame;
  FIRSTPASS_STATS last_frame;
  av1_zero(next_frame);

  rc->frames_since_key = 0;

  // Reset the GF group data structures.
  av1_zero(*gf_group);

  // Clear the alt ref active flag and last group multi arf flags as they
  // can never be set for a key frame.
  rc->source_alt_ref_active = 0;

  // KF is always a GF so clear frames till next gf counter.
  rc->frames_till_gf_update_due = 0;

  rc->frames_to_key = 1;

  if (cpi->oxcf.pass == 0) {
    rc->this_key_frame_forced =
        current_frame->frame_number != 0 && rc->frames_to_key == 0;
    rc->frames_to_key = cpi->oxcf.key_freq;
    rc->kf_boost = DEFAULT_KF_BOOST;
    rc->source_alt_ref_active = 0;
    gf_group->update_type[0] = KF_UPDATE;
    return;
  }
  int i, j;
  const FIRSTPASS_STATS *const start_position = twopass->stats_in;
  int kf_bits = 0;
  int loop_decay_counter = 0;
  double decay_accumulator = 1.0;
  double av_decay_accumulator = 0.0;
  double zero_motion_accumulator = 1.0;
  double boost_score = 0.0;
  double kf_mod_err = 0.0;
  double kf_group_err = 0.0;
  double recent_loop_decay[FRAMES_TO_CHECK_DECAY];

  // Is this a forced key frame by interval.
  rc->this_key_frame_forced = rc->next_key_frame_forced;

  twopass->kf_group_bits = 0;        // Total bits available to kf group
  twopass->kf_group_error_left = 0;  // Group modified error score.

  kf_mod_err = calculate_modified_err(cpi, twopass, oxcf, this_frame);

  // Initialize the decay rates for the recent frames to check
  for (j = 0; j < FRAMES_TO_CHECK_DECAY; ++j) recent_loop_decay[j] = 1.0;

  // Find the next keyframe.
  i = 0;
  while (twopass->stats_in < twopass->stats_in_end &&
         rc->frames_to_key < cpi->oxcf.key_freq) {
    // Accumulate kf group error.
    kf_group_err += calculate_modified_err(cpi, twopass, oxcf, this_frame);

    // Load the next frame's stats.
    last_frame = *this_frame;
    input_stats(twopass, this_frame);

    // Provided that we are not at the end of the file...
    if (cpi->oxcf.auto_key && twopass->stats_in < twopass->stats_in_end) {
      double loop_decay_rate;

      // Check for a scene cut.
      if (test_candidate_kf(twopass, &last_frame, this_frame, twopass->stats_in,
                            rc->frames_to_key))
        break;

      // How fast is the prediction quality decaying?
      loop_decay_rate = get_prediction_decay_rate(cpi, twopass->stats_in);

      // We want to know something about the recent past... rather than
      // as used elsewhere where we are concerned with decay in prediction
      // quality since the last GF or KF.
      recent_loop_decay[i % FRAMES_TO_CHECK_DECAY] = loop_decay_rate;
      decay_accumulator = 1.0;
      for (j = 0; j < FRAMES_TO_CHECK_DECAY; ++j)
        decay_accumulator *= recent_loop_decay[j];

      // Special check for transition or high motion followed by a
      // static scene.
      if (detect_transition_to_still(cpi, i, cpi->oxcf.key_freq - i,
                                     loop_decay_rate, decay_accumulator))
        break;

      // Step on to the next frame.
      ++rc->frames_to_key;

      // If we don't have a real key frame within the next two
      // key_freq intervals then break out of the loop.
      if (rc->frames_to_key >= 2 * cpi->oxcf.key_freq) break;
    } else {
      ++rc->frames_to_key;
    }
    ++i;
  }

  // If there is a max kf interval set by the user we must obey it.
  // We already breakout of the loop above at 2x max.
  // This code centers the extra kf if the actual natural interval
  // is between 1x and 2x.
  if (cpi->oxcf.auto_key && rc->frames_to_key > cpi->oxcf.key_freq) {
    FIRSTPASS_STATS tmp_frame = first_frame;

    rc->frames_to_key /= 2;

    // Reset to the start of the group.
    reset_fpf_position(twopass, start_position);

    kf_group_err = 0.0;

    // Rescan to get the correct error data for the forced kf group.
    for (i = 0; i < rc->frames_to_key; ++i) {
      kf_group_err += calculate_modified_err(cpi, twopass, oxcf, &tmp_frame);
      input_stats(twopass, &tmp_frame);
    }
    rc->next_key_frame_forced = 1;
  } else if (twopass->stats_in == twopass->stats_in_end ||
             rc->frames_to_key >= cpi->oxcf.key_freq) {
    rc->next_key_frame_forced = 1;
  } else {
    rc->next_key_frame_forced = 0;
  }

  // Special case for the last key frame of the file.
  if (twopass->stats_in >= twopass->stats_in_end) {
    // Accumulate kf group error.
    kf_group_err += calculate_modified_err(cpi, twopass, oxcf, this_frame);
  }

  // Calculate the number of bits that should be assigned to the kf group.
  if (twopass->bits_left > 0 && twopass->modified_error_left > 0.0) {
    // Maximum number of bits for a single normal frame (not key frame).
    const int max_bits = frame_max_bits(rc, &cpi->oxcf);

    // Maximum number of bits allocated to the key frame group.
    int64_t max_grp_bits;

    // Default allocation based on bits left and relative
    // complexity of the section.
    twopass->kf_group_bits = (int64_t)(
        twopass->bits_left * (kf_group_err / twopass->modified_error_left));

    // Clip based on maximum per frame rate defined by the user.
    max_grp_bits = (int64_t)max_bits * (int64_t)rc->frames_to_key;
    if (twopass->kf_group_bits > max_grp_bits)
      twopass->kf_group_bits = max_grp_bits;
  } else {
    twopass->kf_group_bits = 0;
  }
  twopass->kf_group_bits = AOMMAX(0, twopass->kf_group_bits);

  // Reset the first pass file position.
  reset_fpf_position(twopass, start_position);

  // Scan through the kf group collating various stats used to determine
  // how many bits to spend on it.
  decay_accumulator = 1.0;
  boost_score = 0.0;
  const double kf_max_boost =
      cpi->oxcf.rc_mode == AOM_Q
          ? AOMMIN(AOMMAX(rc->frames_to_key * 2.0, KF_MIN_FRAME_BOOST),
                   KF_MAX_FRAME_BOOST)
          : KF_MAX_FRAME_BOOST;
  for (i = 0; i < (rc->frames_to_key - 1); ++i) {
    if (EOF == input_stats(twopass, &next_frame)) break;

    // Monitor for static sections.
    // For the first frame in kf group, the second ref indicator is invalid.
    if (i > 0) {
      zero_motion_accumulator = AOMMIN(
          zero_motion_accumulator, get_zero_motion_factor(cpi, &next_frame));
    } else {
      zero_motion_accumulator = next_frame.pcnt_inter - next_frame.pcnt_motion;
    }

    // Not all frames in the group are necessarily used in calculating boost.
    if ((i <= rc->max_gf_interval) ||
        ((i <= (rc->max_gf_interval * 4)) && (decay_accumulator > 0.5))) {
      const double frame_boost =
          calc_frame_boost(cpi, this_frame, 0, kf_max_boost);

      // How fast is prediction quality decaying.
      if (!detect_flash(twopass, 0)) {
        const double loop_decay_rate =
            get_prediction_decay_rate(cpi, &next_frame);
        decay_accumulator *= loop_decay_rate;
        decay_accumulator = AOMMAX(decay_accumulator, MIN_DECAY_FACTOR);
        av_decay_accumulator += decay_accumulator;
        ++loop_decay_counter;
      }
      boost_score += (decay_accumulator * frame_boost);
    }
  }
  if (loop_decay_counter > 0)
    av_decay_accumulator /= (double)loop_decay_counter;

  reset_fpf_position(twopass, start_position);

  // Store the zero motion percentage
  twopass->kf_zeromotion_pct = (int)(zero_motion_accumulator * 100.0);

  // Calculate a section intra ratio used in setting max loop filter.
  twopass->section_intra_rating = calculate_section_intra_ratio(
      start_position, twopass->stats_in_end, rc->frames_to_key);

  rc->kf_boost = (int)(av_decay_accumulator * boost_score);

  // Special case for static / slide show content but don't apply
  // if the kf group is very short.
  if ((zero_motion_accumulator > STATIC_KF_GROUP_FLOAT_THRESH) &&
      (rc->frames_to_key > 8)) {
    rc->kf_boost = AOMMAX(rc->kf_boost, MIN_STATIC_KF_BOOST);
  } else {
    // Apply various clamps for min and max boost
    rc->kf_boost = AOMMAX(rc->kf_boost, (rc->frames_to_key * 3));
    rc->kf_boost = AOMMAX(rc->kf_boost, MIN_KF_BOOST);
  }

  // Work out how many bits to allocate for the key frame itself.
  kf_bits = calculate_boost_bits((rc->frames_to_key - 1), rc->kf_boost,
                                 twopass->kf_group_bits);
  // printf("kf boost = %d kf_bits = %d kf_zeromotion_pct = %d\n", rc->kf_boost,
  //        kf_bits, twopass->kf_zeromotion_pct);

  // Work out the fraction of the kf group bits reserved for the inter frames
  // within the group after discounting the bits for the kf itself.
  if (twopass->kf_group_bits) {
    twopass->kfgroup_inter_fraction =
        (double)(twopass->kf_group_bits - kf_bits) /
        (double)twopass->kf_group_bits;
  } else {
    twopass->kfgroup_inter_fraction = 1.0;
  }

  twopass->kf_group_bits -= kf_bits;

  // Save the bits to spend on the key frame.
  gf_group->bit_allocation[0] = kf_bits;
  gf_group->update_type[0] = KF_UPDATE;

  // Note the total error score of the kf group minus the key frame itself.
  twopass->kf_group_error_left = (int)(kf_group_err - kf_mod_err);

  // Adjust the count of total modified error left.
  // The count of bits left is adjusted elsewhere based on real coded frame
  // sizes.
  twopass->modified_error_left -= kf_group_err;
}

static int is_skippable_frame(const AV1_COMP *cpi) {
  if (cpi->oxcf.pass == 0) return 0;
  // If the current frame does not have non-zero motion vector detected in the
  // first  pass, and so do its previous and forward frames, then this frame
  // can be skipped for partition check, and the partition size is assigned
  // according to the variance
  const TWO_PASS *const twopass = &cpi->twopass;

  return (!frame_is_intra_only(&cpi->common) &&
          twopass->stats_in - 2 > twopass->stats_in_start &&
          twopass->stats_in < twopass->stats_in_end &&
          (twopass->stats_in - 1)->pcnt_inter -
                  (twopass->stats_in - 1)->pcnt_motion ==
              1 &&
          (twopass->stats_in - 2)->pcnt_inter -
                  (twopass->stats_in - 2)->pcnt_motion ==
              1 &&
          twopass->stats_in->pcnt_inter - twopass->stats_in->pcnt_motion == 1);
}

#define ARF_STATS_OUTPUT 0
#if ARF_STATS_OUTPUT
unsigned int arf_count = 0;
#endif
#define DEFAULT_GRP_WEIGHT 1.0

static void process_first_pass_stats(AV1_COMP *cpi,
                                     FIRSTPASS_STATS *this_frame) {
  AV1_COMMON *const cm = &cpi->common;
  CurrentFrame *const current_frame = &cm->current_frame;
  RATE_CONTROL *const rc = &cpi->rc;
  TWO_PASS *const twopass = &cpi->twopass;

  if (cpi->oxcf.rc_mode != AOM_Q && current_frame->frame_number == 0) {
    const int frames_left =
        (int)(twopass->total_stats.count - current_frame->frame_number);

    // Special case code for first frame.
    const int section_target_bandwidth =
        (int)(twopass->bits_left / frames_left);
    const double section_length = twopass->total_left_stats.count;
    const double section_error =
        twopass->total_left_stats.coded_error / section_length;
    const double section_intra_skip =
        twopass->total_left_stats.intra_skip_pct / section_length;
    const double section_inactive_zone =
        (twopass->total_left_stats.inactive_zone_rows * 2) /
        ((double)cm->mb_rows * section_length);
    const int tmp_q = get_twopass_worst_quality(
        cpi, section_error, section_intra_skip + section_inactive_zone,
        section_target_bandwidth, DEFAULT_GRP_WEIGHT);

    rc->active_worst_quality = tmp_q;
    rc->ni_av_qi = tmp_q;
    rc->last_q[INTER_FRAME] = tmp_q;
    rc->avg_q = av1_convert_qindex_to_q(tmp_q, cm->seq_params.bit_depth);
    rc->avg_frame_qindex[INTER_FRAME] = tmp_q;
    rc->last_q[KEY_FRAME] = (tmp_q + cpi->oxcf.best_allowed_q) / 2;
    rc->avg_frame_qindex[KEY_FRAME] = rc->last_q[KEY_FRAME];
  }

  if (EOF == input_stats(twopass, this_frame)) return;

  {
    const int num_mbs = (cpi->oxcf.resize_mode != RESIZE_NONE)
                            ? cpi->initial_mbs
                            : cpi->common.MBs;
    // The multiplication by 256 reverses a scaling factor of (>> 8)
    // applied when combining MB error values for the frame.
    twopass->mb_av_energy = log((this_frame->intra_error / num_mbs) + 1.0);
    twopass->frame_avg_haar_energy =
        log((this_frame->frame_avg_wavelet_energy / num_mbs) + 1.0);
  }

  // Update the total stats remaining structure.
  subtract_stats(&twopass->total_left_stats, this_frame);

  // Set the frame content type flag.
  if (this_frame->intra_skip_pct >= FC_ANIMATION_THRESH)
    twopass->fr_content_type = FC_GRAPHICS_ANIMATION;
  else
    twopass->fr_content_type = FC_NORMAL;
}

static void setup_target_rate(AV1_COMP *cpi, FRAME_TYPE frame_type) {
  RATE_CONTROL *const rc = &cpi->rc;
  GF_GROUP *const gf_group = &cpi->gf_group;

  int target_rate = gf_group->bit_allocation[gf_group->index];

  if (cpi->oxcf.pass == 0) {
    av1_rc_set_frame_target(cpi, target_rate, cpi->common.width,
                            cpi->common.height);
  } else {
    if (frame_type == KEY_FRAME) {
      target_rate = av1_rc_clamp_iframe_target_size(cpi, target_rate);
    } else {
      target_rate = av1_rc_clamp_pframe_target_size(
          cpi, target_rate, gf_group->update_type[gf_group->index]);
    }
  }

  rc->base_frame_target = target_rate;
}

void av1_get_second_pass_params(AV1_COMP *cpi,
                                EncodeFrameParams *const frame_params,
                                unsigned int frame_flags) {
  RATE_CONTROL *const rc = &cpi->rc;
  TWO_PASS *const twopass = &cpi->twopass;
  GF_GROUP *const gf_group = &cpi->gf_group;

  if (cpi->oxcf.pass == 2 && !twopass->stats_in) return;

  if (rc->frames_till_gf_update_due > 0 && !(frame_flags & FRAMEFLAGS_KEY)) {
    assert(gf_group->index < gf_group->size);
    const int update_type = gf_group->update_type[gf_group->index];

    setup_target_rate(cpi, frame_params->frame_type);

    // If this is an arf frame then we dont want to read the stats file or
    // advance the input pointer as we already have what we need.
    if (update_type == ARF_UPDATE || update_type == INTNL_ARF_UPDATE) {
      if (cpi->no_show_kf) {
        assert(update_type == ARF_UPDATE);
        frame_params->frame_type = KEY_FRAME;
      } else {
        frame_params->frame_type = INTER_FRAME;
      }

      // Do the firstpass stats indicate that this frame is skippable for the
      // partition search?
      if (cpi->sf.allow_partition_search_skip && cpi->oxcf.pass == 2) {
        cpi->partition_search_skippable_frame = is_skippable_frame(cpi);
      }

      return;
    }
  }

  aom_clear_system_state();

  if (cpi->oxcf.rc_mode == AOM_Q) rc->active_worst_quality = cpi->oxcf.cq_level;
  FIRSTPASS_STATS this_frame;
  av1_zero(this_frame);
  // call above fn
  if (cpi->oxcf.pass == 2) {
    process_first_pass_stats(cpi, &this_frame);
  } else {
    rc->active_worst_quality = cpi->oxcf.cq_level;
  }

  // Keyframe and section processing.
  if (rc->frames_to_key == 0 || (frame_flags & FRAMEFLAGS_KEY)) {
    FIRSTPASS_STATS this_frame_copy;
    this_frame_copy = this_frame;
    frame_params->frame_type = KEY_FRAME;
    // Define next KF group and assign bits to it.
    find_next_key_frame(cpi, &this_frame);
    this_frame = this_frame_copy;
  } else {
    frame_params->frame_type = INTER_FRAME;
  }

  // Define a new GF/ARF group. (Should always enter here for key frames).
  if (rc->frames_till_gf_update_due == 0) {
    assert(cpi->common.current_frame.frame_number == 0 ||
           gf_group->index == gf_group->size);
    define_gf_group(cpi, &this_frame, frame_params);
    rc->frames_till_gf_update_due = rc->baseline_gf_interval;
    cpi->num_gf_group_show_frames = 0;
    assert(gf_group->index == 0);

#if ARF_STATS_OUTPUT
    {
      FILE *fpfile;
      fpfile = fopen("arf.stt", "a");
      ++arf_count;
      fprintf(fpfile, "%10d %10d %10d %10d %10d\n", current_frame->frame_number,
              rc->frames_till_gf_update_due, rc->kf_boost, arf_count,
              rc->gfu_boost);

      fclose(fpfile);
    }
#endif
  }
  assert(gf_group->index < gf_group->size);

  // Do the firstpass stats indicate that this frame is skippable for the
  // partition search?
  if (cpi->sf.allow_partition_search_skip && cpi->oxcf.pass == 2) {
    cpi->partition_search_skippable_frame = is_skippable_frame(cpi);
  }

  setup_target_rate(cpi, frame_params->frame_type);
}

void av1_init_second_pass(AV1_COMP *cpi) {
  const AV1EncoderConfig *const oxcf = &cpi->oxcf;
  TWO_PASS *const twopass = &cpi->twopass;
  double frame_rate;
  FIRSTPASS_STATS *stats;

  av1_twopass_zero_stats(&twopass->total_stats);
  av1_twopass_zero_stats(&twopass->total_left_stats);

  if (!twopass->stats_in_end) return;

  stats = &twopass->total_stats;

  *stats = *twopass->stats_in_end;
  twopass->total_left_stats = *stats;

  frame_rate = 10000000.0 * stats->count / stats->duration;
  // Each frame can have a different duration, as the frame rate in the source
  // isn't guaranteed to be constant. The frame rate prior to the first frame
  // encoded in the second pass is a guess. However, the sum duration is not.
  // It is calculated based on the actual durations of all frames from the
  // first pass.
  av1_new_framerate(cpi, frame_rate);
  twopass->bits_left =
      (int64_t)(stats->duration * oxcf->target_bandwidth / 10000000.0);

  // This variable monitors how far behind the second ref update is lagging.
  twopass->sr_update_lag = 1;

  // Scan the first pass file and calculate a modified total error based upon
  // the bias/power function used to allocate bits.
  {
    const double avg_error =
        stats->coded_error / DOUBLE_DIVIDE_CHECK(stats->count);
    const FIRSTPASS_STATS *s = twopass->stats_in;
    double modified_error_total = 0.0;
    twopass->modified_error_min =
        (avg_error * oxcf->two_pass_vbrmin_section) / 100;
    twopass->modified_error_max =
        (avg_error * oxcf->two_pass_vbrmax_section) / 100;
    while (s < twopass->stats_in_end) {
      modified_error_total += calculate_modified_err(cpi, twopass, oxcf, s);
      ++s;
    }
    twopass->modified_error_left = modified_error_total;
  }

  // Reset the vbr bits off target counters
  cpi->rc.vbr_bits_off_target = 0;
  cpi->rc.vbr_bits_off_target_fast = 0;

  cpi->rc.rate_error_estimate = 0;

  // Static sequence monitor variables.
  twopass->kf_zeromotion_pct = 100;
  twopass->last_kfgroup_zeromotion_pct = 100;
}

#define MINQ_ADJ_LIMIT 48
#define MINQ_ADJ_LIMIT_CQ 20
#define HIGH_UNDERSHOOT_RATIO 2
void av1_twopass_postencode_update(AV1_COMP *cpi) {
  TWO_PASS *const twopass = &cpi->twopass;
  RATE_CONTROL *const rc = &cpi->rc;
  const int bits_used = rc->base_frame_target;

  // VBR correction is done through rc->vbr_bits_off_target. Based on the
  // sign of this value, a limited % adjustment is made to the target rate
  // of subsequent frames, to try and push it back towards 0. This method
  // is designed to prevent extreme behaviour at the end of a clip
  // or group of frames.
  rc->vbr_bits_off_target += rc->base_frame_target - rc->projected_frame_size;
  twopass->bits_left = AOMMAX(twopass->bits_left - bits_used, 0);

  // Calculate the pct rc error.
  if (rc->total_actual_bits) {
    rc->rate_error_estimate =
        (int)((rc->vbr_bits_off_target * 100) / rc->total_actual_bits);
    rc->rate_error_estimate = clamp(rc->rate_error_estimate, -100, 100);
  } else {
    rc->rate_error_estimate = 0;
  }

  if (cpi->common.current_frame.frame_type != KEY_FRAME) {
    twopass->kf_group_bits -= bits_used;
    twopass->last_kfgroup_zeromotion_pct = twopass->kf_zeromotion_pct;
  }
  twopass->kf_group_bits = AOMMAX(twopass->kf_group_bits, 0);

  // If the rate control is drifting consider adjustment to min or maxq.
  if ((cpi->oxcf.rc_mode != AOM_Q) && !cpi->rc.is_src_frame_alt_ref) {
    const int maxq_adj_limit = rc->worst_quality - rc->active_worst_quality;
    const int minq_adj_limit =
        (cpi->oxcf.rc_mode == AOM_CQ ? MINQ_ADJ_LIMIT_CQ : MINQ_ADJ_LIMIT);

    // Undershoot.
    if (rc->rate_error_estimate > cpi->oxcf.under_shoot_pct) {
      --twopass->extend_maxq;
      if (rc->rolling_target_bits >= rc->rolling_actual_bits)
        ++twopass->extend_minq;
      // Overshoot.
    } else if (rc->rate_error_estimate < -cpi->oxcf.over_shoot_pct) {
      --twopass->extend_minq;
      if (rc->rolling_target_bits < rc->rolling_actual_bits)
        ++twopass->extend_maxq;
    } else {
      // Adjustment for extreme local overshoot.
      if (rc->projected_frame_size > (2 * rc->base_frame_target) &&
          rc->projected_frame_size > (2 * rc->avg_frame_bandwidth))
        ++twopass->extend_maxq;

      // Unwind undershoot or overshoot adjustment.
      if (rc->rolling_target_bits < rc->rolling_actual_bits)
        --twopass->extend_minq;
      else if (rc->rolling_target_bits > rc->rolling_actual_bits)
        --twopass->extend_maxq;
    }

    twopass->extend_minq = clamp(twopass->extend_minq, 0, minq_adj_limit);
    twopass->extend_maxq = clamp(twopass->extend_maxq, 0, maxq_adj_limit);

    // If there is a big and undexpected undershoot then feed the extra
    // bits back in quickly. One situation where this may happen is if a
    // frame is unexpectedly almost perfectly predicted by the ARF or GF
    // but not very well predcited by the previous frame.
    if (!frame_is_kf_gf_arf(cpi) && !cpi->rc.is_src_frame_alt_ref) {
      int fast_extra_thresh = rc->base_frame_target / HIGH_UNDERSHOOT_RATIO;
      if (rc->projected_frame_size < fast_extra_thresh) {
        rc->vbr_bits_off_target_fast +=
            fast_extra_thresh - rc->projected_frame_size;
        rc->vbr_bits_off_target_fast =
            AOMMIN(rc->vbr_bits_off_target_fast, (4 * rc->avg_frame_bandwidth));

        // Fast adaptation of minQ if necessary to use up the extra bits.
        if (rc->avg_frame_bandwidth) {
          twopass->extend_minq_fast =
              (int)(rc->vbr_bits_off_target_fast * 8 / rc->avg_frame_bandwidth);
        }
        twopass->extend_minq_fast = AOMMIN(
            twopass->extend_minq_fast, minq_adj_limit - twopass->extend_minq);
      } else if (rc->vbr_bits_off_target_fast) {
        twopass->extend_minq_fast = AOMMIN(
            twopass->extend_minq_fast, minq_adj_limit - twopass->extend_minq);
      } else {
        twopass->extend_minq_fast = 0;
      }
    }
  }
}
