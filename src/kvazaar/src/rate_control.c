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

#include "rate_control.h"

#include <math.h>

#include "encoder.h"
#include "kvazaar.h"


static const int SMOOTHING_WINDOW = 40;
static const double MIN_LAMBDA    = 0.1;
static const double MAX_LAMBDA    = 10000;

/**
 * \brief Clip lambda value to a valid range.
 */
static double clip_lambda(double lambda) {
  if (isnan(lambda)) return MAX_LAMBDA;
  return CLIP(MIN_LAMBDA, MAX_LAMBDA, lambda);
}

/**
 * \brief Update alpha and beta parameters.
 *
 * \param         bits        number of bits spent for coding the area
 * \param         pixels      size of the area in pixels
 * \param         lambda_real lambda used for coding the area
 * \param[in,out] alpha       alpha parameter to update
 * \param[in,out] beta        beta parameter to update
 */
static void update_parameters(uint32_t bits,
                              uint32_t pixels,
                              double lambda_real,
                              double *alpha,
                              double *beta)
{
  const double bpp              = bits / (double)pixels;
  const double lambda_comp      = clip_lambda(*alpha * pow(bpp, *beta));
  const double lambda_log_ratio = log(lambda_real) - log(lambda_comp);

  *alpha += 0.10 * lambda_log_ratio * (*alpha);
  *alpha = CLIP(0.05, 20, *alpha);

  *beta  += 0.05 * lambda_log_ratio * CLIP(-5.0, -1.0, log(bpp));
  *beta  = CLIP(-3, -0.1, *beta);
}

/**
 * \brief Allocate bits for the current GOP.
 * \param state   the main encoder state
 * \return        target number of bits
 */
static double gop_allocate_bits(encoder_state_t * const state)
{
  const encoder_control_t * const encoder = state->encoder_control;

  // At this point, total_bits_coded of the current state contains the
  // number of bits written encoder->owf frames before the current frame.
  uint64_t bits_coded = state->frame->total_bits_coded;
  int pictures_coded = MAX(0, state->frame->num - encoder->cfg.owf);

  int gop_offset = (state->frame->gop_offset - encoder->cfg.owf) % MAX(1, encoder->cfg.gop_len);
  // Only take fully coded GOPs into account.
  if (encoder->cfg.gop_len > 0 && gop_offset != encoder->cfg.gop_len - 1) {
    // Subtract number of bits in the partially coded GOP.
    bits_coded -= state->frame->cur_gop_bits_coded;
    // Subtract number of pictures in the partially coded GOP.
    pictures_coded -= gop_offset + 1;
  }

  // Equation 12 from https://doi.org/10.1109/TIP.2014.2336550
  double gop_target_bits =
    (encoder->target_avg_bppic * (pictures_coded + SMOOTHING_WINDOW) - bits_coded)
    * MAX(1, encoder->cfg.gop_len) / SMOOTHING_WINDOW;
  // Allocate at least 200 bits for each GOP like HM does.
  return MAX(200, gop_target_bits);
}

/**
 * Estimate number of bits used for headers of the current picture.
 * \param state   the main encoder state
 * \return        number of header bits
 */
static uint64_t pic_header_bits(encoder_state_t * const state)
{
  const kvz_config* cfg = &state->encoder_control->cfg;

  // nal type and slice header
  uint64_t bits = 48 + 24;

  // entry points
  bits += 12 * state->encoder_control->in.height_in_lcu;

  switch (cfg->hash) {
    case KVZ_HASH_CHECKSUM:
      bits += 168;
      break;

    case KVZ_HASH_MD5:
      bits += 456;
      break;

    case KVZ_HASH_NONE:
      break;
  }

  if (encoder_state_must_write_vps(state)) {
    bits += 613;
  }

  if (state->frame->num == 0 && cfg->add_encoder_info) {
    bits += 1392;
  }

  return bits;
}

/**
 * Allocate bits for the current picture.
 * \param state   the main encoder state
 * \return        target number of bits, excluding headers
 */
static double pic_allocate_bits(encoder_state_t * const state)
{
  const encoder_control_t * const encoder = state->encoder_control;

  if (encoder->cfg.gop_len == 0 ||
      state->frame->gop_offset == 0 ||
      state->frame->num == 0)
  {
    // A new GOP starts at this frame.
    state->frame->cur_gop_target_bits = gop_allocate_bits(state);
    state->frame->cur_gop_bits_coded  = 0;
  } else {
    state->frame->cur_gop_target_bits =
      state->previous_encoder_state->frame->cur_gop_target_bits;
  }

  if (encoder->cfg.gop_len <= 0) {
    return state->frame->cur_gop_target_bits;
  }

  const double pic_weight = encoder->gop_layer_weights[
    encoder->cfg.gop[state->frame->gop_offset].layer - 1];
  const double pic_target_bits =
    state->frame->cur_gop_target_bits * pic_weight - pic_header_bits(state);
  // Allocate at least 100 bits for each picture like HM does.
  return MAX(100, pic_target_bits);
}

static int8_t lambda_to_qp(const double lambda)
{
  const int8_t qp = 4.2005 * log(lambda) + 13.7223 + 0.5;
  return CLIP_TO_QP(qp);
}

static double qp_to_lamba(encoder_state_t * const state, int qp)
{
  const encoder_control_t * const ctrl = state->encoder_control;
  const int gop_len = ctrl->cfg.gop_len;
  const int period = gop_len > 0 ? gop_len : ctrl->cfg.intra_period;

  kvz_gop_config const * const gop = &ctrl->cfg.gop[state->frame->gop_offset];

  double lambda = pow(2.0, (qp - 12) / 3.0);

  if (state->frame->slicetype == KVZ_SLICE_I) {
    lambda *= 0.57;

    // Reduce lambda for I-frames according to the number of references.
    if (period == 0) {
      lambda *= 0.5;
    } else {
      lambda *= 1.0 - CLIP(0.0, 0.5, 0.05 * (period - 1));
    }
  } else if (gop_len > 0) {
    lambda *= gop->qp_factor;
  } else {
    lambda *= 0.4624;
  }

  // Increase lambda if not key-frame.
  if (period > 0 && state->frame->poc % period != 0) {
    lambda *= CLIP(2.0, 4.0, (state->frame->QP - 12) / 6.0);
  }

  return lambda;
}

/**
 * \brief Allocate bits and set lambda and QP for the current picture.
 * \param state the main encoder state
 */
void kvz_set_picture_lambda_and_qp(encoder_state_t * const state)
{
  const encoder_control_t * const ctrl = state->encoder_control;

  if (ctrl->cfg.target_bitrate > 0) {
    // Rate control enabled

    if (state->frame->num > ctrl->cfg.owf) {
      // At least one frame has been written.
      update_parameters(state->stats_bitstream_length * 8,
                        ctrl->in.pixels_per_pic,
                        state->frame->lambda,
                        &state->frame->rc_alpha,
                        &state->frame->rc_beta);
    }

    const double pic_target_bits = pic_allocate_bits(state);
    const double target_bpp = pic_target_bits / ctrl->in.pixels_per_pic;
    double lambda = state->frame->rc_alpha * pow(target_bpp, state->frame->rc_beta);
    lambda = clip_lambda(lambda);

    state->frame->lambda              = lambda;
    state->frame->QP                  = lambda_to_qp(lambda);
    state->frame->cur_pic_target_bits = pic_target_bits;

  } else {
    // Rate control disabled
    kvz_gop_config const * const gop = &ctrl->cfg.gop[state->frame->gop_offset];
    const int gop_len = ctrl->cfg.gop_len;

    if (gop_len > 0 && state->frame->slicetype != KVZ_SLICE_I) {
      state->frame->QP = CLIP_TO_QP(ctrl->cfg.qp + gop->qp_offset);
    } else {
      state->frame->QP = ctrl->cfg.qp;
    }

    state->frame->lambda = qp_to_lamba(state, state->frame->QP);
  }
}

/**
 * \brief Allocate bits for a LCU.
 * \param state   the main encoder state
 * \param pos     location of the LCU as number of LCUs from top left
 * \return number of bits allocated for the LCU
 */
static double lcu_allocate_bits(encoder_state_t * const state,
                                vector2d_t pos)
{
  double lcu_weight;
  if (state->frame->num > state->encoder_control->cfg.owf) {
    lcu_weight = kvz_get_lcu_stats(state, pos.x, pos.y)->weight;
  } else {
    const uint32_t num_lcus = state->encoder_control->in.width_in_lcu *
                              state->encoder_control->in.height_in_lcu;
    lcu_weight = 1.0 / num_lcus;
  }

  // Target number of bits for the current LCU.
  const double lcu_target_bits = state->frame->cur_pic_target_bits * lcu_weight;

  // Allocate at least one bit for each LCU.
  return MAX(1, lcu_target_bits);
}

void kvz_set_lcu_lambda_and_qp(encoder_state_t * const state,
                               vector2d_t pos)
{
  const encoder_control_t * const ctrl = state->encoder_control;

  if (ctrl->cfg.roi.dqps != NULL) {
    vector2d_t lcu = {
      pos.x + state->tile->lcu_offset_x,
      pos.y + state->tile->lcu_offset_y
    };
    vector2d_t roi = {
      lcu.x * ctrl->cfg.roi.width / ctrl->in.width_in_lcu,
      lcu.y * ctrl->cfg.roi.height / ctrl->in.height_in_lcu
    };
    int roi_index = roi.x + roi.y * ctrl->cfg.roi.width;
    int dqp = ctrl->cfg.roi.dqps[roi_index];
    state->qp = CLIP_TO_QP(state->frame->QP + dqp);
    state->lambda = qp_to_lamba(state, state->qp);
    state->lambda_sqrt = sqrt(state->frame->lambda);

  } else if (ctrl->cfg.target_bitrate > 0) {
    lcu_stats_t *lcu         = kvz_get_lcu_stats(state, pos.x, pos.y);
    const uint32_t pixels    = MIN(LCU_WIDTH, state->tile->frame->width  - LCU_WIDTH * pos.x) *
                               MIN(LCU_WIDTH, state->tile->frame->height - LCU_WIDTH * pos.y);

    if (state->frame->num > ctrl->cfg.owf) {
      update_parameters(lcu->bits,
                        pixels,
                        lcu->lambda,
                        &lcu->rc_alpha,
                        &lcu->rc_beta);
    } else {
      lcu->rc_alpha = state->frame->rc_alpha;
      lcu->rc_beta  = state->frame->rc_beta;
    }

    const double target_bits = lcu_allocate_bits(state, pos);
    const double target_bpp  = target_bits / pixels;

    double lambda = clip_lambda(lcu->rc_alpha * pow(target_bpp, lcu->rc_beta));
    // Clip lambda according to the equations 24 and 26 in
    // https://doi.org/10.1109/TIP.2014.2336550
    if (state->frame->num > ctrl->cfg.owf) {
      const double bpp         = lcu->bits / (double)pixels;
      const double lambda_comp = clip_lambda(lcu->rc_alpha * pow(bpp, lcu->rc_beta));
      lambda = CLIP(lambda_comp * 0.7937005259840998,
                    lambda_comp * 1.2599210498948732,
                    lambda);
    }
    lambda = CLIP(state->frame->lambda * 0.6299605249474366,
                  state->frame->lambda * 1.5874010519681994,
                  lambda);
    lambda = clip_lambda(lambda);

    lcu->lambda        = lambda;
    state->lambda      = lambda;
    state->lambda_sqrt = sqrt(lambda);
    state->qp          = lambda_to_qp(lambda);

  } else {
    state->qp          = state->frame->QP;
    state->lambda      = state->frame->lambda;
    state->lambda_sqrt = sqrt(state->frame->lambda);
  }
}
