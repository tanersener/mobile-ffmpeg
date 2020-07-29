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
#include "pthread.h"


static const int SMOOTHING_WINDOW = 40;
static const double MIN_LAMBDA    = 0.1;
static const double MAX_LAMBDA    = 10000;
#define BETA1 1.2517

static kvz_rc_data *data;

/**
 * \brief Clip lambda value to a valid range.
 */
static double clip_lambda(double lambda) {
  if (isnan(lambda)) return MAX_LAMBDA;
  return CLIP(MIN_LAMBDA, MAX_LAMBDA, lambda);
}

kvz_rc_data * kvz_get_rc_data(const encoder_control_t * const encoder) {
  if (data != NULL || encoder == NULL) return data;

  data = calloc(1, sizeof(kvz_rc_data));

  if (data == NULL) return NULL;
  if (pthread_mutex_init(&data->ck_frame_lock, NULL) != 0) return NULL;
  if (pthread_mutex_init(&data->lambda_lock, NULL) != 0) return NULL;
  if (pthread_mutex_init(&data->intra_lock, NULL) != 0) return NULL;
  for (int (i) = 0; (i) < KVZ_MAX_GOP_LAYERS; ++(i)) {
    if (pthread_rwlock_init(&data->ck_ctu_lock[i], NULL) != 0) return NULL;
  }

  const int num_lcus = encoder->in.width_in_lcu * encoder->in.height_in_lcu;

  for (int i = 0; i < KVZ_MAX_GOP_LAYERS; i++) {
    data->c_para[i] = malloc(sizeof(double) * num_lcus);
    if (data->c_para[i] == NULL) return NULL;

    data->k_para[i] = malloc(sizeof(double) * num_lcus);
    if (data->k_para[i] == NULL) return NULL;

    data->pic_c_para[i] = 5.0;
    data->pic_k_para[i] = -0.1;

    for (int j = 0; j < num_lcus; j++) {
      data->c_para[i][j] = 5.0;
      data->k_para[i][j] = -0.1;
    }
  }
  data->intra_bpp = calloc(num_lcus, sizeof(double));
  if (data->intra_bpp == NULL) return NULL;
  data->intra_dis = calloc(num_lcus, sizeof(double));
  if (data->intra_dis == NULL) return NULL;

  memset(data->previous_lambdas, 0, sizeof(data->previous_lambdas));

  data->previous_frame_lambda = 0.0;

  data->intra_pic_bpp = 0.0;
  data->intra_pic_distortion = 0.0;

  data->intra_alpha = 6.7542000000000000;
  data->intra_beta = 1.7860000000000000;
  return data;
}

void kvz_free_rc_data() {
  if (data == NULL) return;

  pthread_mutex_destroy(&data->ck_frame_lock);
  pthread_mutex_destroy(&data->lambda_lock);
  pthread_mutex_destroy(&data->intra_lock);
  for (int i = 0; i < KVZ_MAX_GOP_LAYERS; ++i) {
    pthread_rwlock_destroy(&data->ck_ctu_lock[i]);
  }

  if (data->intra_bpp) FREE_POINTER(data->intra_bpp);
  if (data->intra_dis) FREE_POINTER(data->intra_dis);
  for (int i = 0; i < KVZ_MAX_GOP_LAYERS; i++) {
    if (data->c_para[i]) FREE_POINTER(data->c_para[i]);
    if (data->k_para[i]) FREE_POINTER(data->k_para[i]);
  }
  FREE_POINTER(data);
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
  
  if (encoder->cfg.gop_len > 0 && gop_offset != encoder->cfg.gop_len - 1 && encoder->cfg.gop_lp_definition.d == 0) {
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

static int xCalcHADs8x8_ISlice(kvz_pixel * piOrg, int y, int iStrideOrg)
{
  piOrg += y * iStrideOrg;
  int i, j;
  int diff[64], m1[8][8], m2[8][8], m3[8][8], iSumHad = 0;

  for (int k = 0; k < 64; k += 8) {
    diff[k + 0] = piOrg[0];
    diff[k + 1] = piOrg[1];
    diff[k + 2] = piOrg[2];
    diff[k + 3] = piOrg[3];
    diff[k + 4] = piOrg[4];
    diff[k + 5] = piOrg[5];
    diff[k + 6] = piOrg[6];
    diff[k + 7] = piOrg[7];

    piOrg += iStrideOrg;
  }

  //horizontal
  for (j = 0; j < 8; j++) {
    int jj = j << 3;
    m2[j][0] = diff[jj] + diff[jj + 4];
    m2[j][1] = diff[jj + 1] + diff[jj + 5];
    m2[j][2] = diff[jj + 2] + diff[jj + 6];
    m2[j][3] = diff[jj + 3] + diff[jj + 7];
    m2[j][4] = diff[jj] - diff[jj + 4];
    m2[j][5] = diff[jj + 1] - diff[jj + 5];
    m2[j][6] = diff[jj + 2] - diff[jj + 6];
    m2[j][7] = diff[jj + 3] - diff[jj + 7];

    m1[j][0] = m2[j][0] + m2[j][2];
    m1[j][1] = m2[j][1] + m2[j][3];
    m1[j][2] = m2[j][0] - m2[j][2];
    m1[j][3] = m2[j][1] - m2[j][3];
    m1[j][4] = m2[j][4] + m2[j][6];
    m1[j][5] = m2[j][5] + m2[j][7];
    m1[j][6] = m2[j][4] - m2[j][6];
    m1[j][7] = m2[j][5] - m2[j][7];

    m2[j][0] = m1[j][0] + m1[j][1];
    m2[j][1] = m1[j][0] - m1[j][1];
    m2[j][2] = m1[j][2] + m1[j][3];
    m2[j][3] = m1[j][2] - m1[j][3];
    m2[j][4] = m1[j][4] + m1[j][5];
    m2[j][5] = m1[j][4] - m1[j][5];
    m2[j][6] = m1[j][6] + m1[j][7];
    m2[j][7] = m1[j][6] - m1[j][7];
  }

  //vertical
  for (i = 0; i < 8; i++) {
    m3[0][i] = m2[0][i] + m2[4][i];
    m3[1][i] = m2[1][i] + m2[5][i];
    m3[2][i] = m2[2][i] + m2[6][i];
    m3[3][i] = m2[3][i] + m2[7][i];
    m3[4][i] = m2[0][i] - m2[4][i];
    m3[5][i] = m2[1][i] - m2[5][i];
    m3[6][i] = m2[2][i] - m2[6][i];
    m3[7][i] = m2[3][i] - m2[7][i];

    m1[0][i] = m3[0][i] + m3[2][i];
    m1[1][i] = m3[1][i] + m3[3][i];
    m1[2][i] = m3[0][i] - m3[2][i];
    m1[3][i] = m3[1][i] - m3[3][i];
    m1[4][i] = m3[4][i] + m3[6][i];
    m1[5][i] = m3[5][i] + m3[7][i];
    m1[6][i] = m3[4][i] - m3[6][i];
    m1[7][i] = m3[5][i] - m3[7][i];

    m2[0][i] = m1[0][i] + m1[1][i];
    m2[1][i] = m1[0][i] - m1[1][i];
    m2[2][i] = m1[2][i] + m1[3][i];
    m2[3][i] = m1[2][i] - m1[3][i];
    m2[4][i] = m1[4][i] + m1[5][i];
    m2[5][i] = m1[4][i] - m1[5][i];
    m2[6][i] = m1[6][i] + m1[7][i];
    m2[7][i] = m1[6][i] - m1[7][i];
  }

  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      iSumHad += abs(m2[i][j]);
    }
  }
  iSumHad -= abs(m2[0][0]);
  iSumHad = (iSumHad + 2) >> 2;
  return(iSumHad);
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

  if (state->frame->is_irap && encoder->cfg.intra_bit_allocation) {
    int total_cost = 0;
    for (int y = 0; y < encoder->cfg.height; y += 8) {
      for (int x = 0; x < encoder->cfg.width; x += 8) {
        int cost = xCalcHADs8x8_ISlice(state->tile->frame->source->y + x, y, state->tile->frame->source->stride);
        total_cost += cost;
        kvz_get_lcu_stats(state, x / 64, y / 64)->i_cost += cost;
      }
    }
    state->frame->icost = total_cost;
    state->frame->remaining_weight = total_cost;

    double bits = state->frame->cur_gop_target_bits / MAX(encoder->cfg.gop_len, 1);
    double alpha, beta = 0.5582;
    if (bits * 40 < encoder->cfg.width * encoder->cfg.height) {
      alpha = 0.25;
    }
    else {
      alpha = 0.3;
    }
    return MAX(100, alpha*pow(state->frame->icost * 4 / bits, beta)*bits);
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

static double solve_cubic_equation(const encoder_state_config_frame_t * const state,
                            int ctu_index,
                            int last_ctu,
                            double est_lambda,
                            double target_bits) 
{
  double best_lambda = 0.0;
  double para_a = 0.0;
  double para_b = 0.0;
  double para_c = 0.0;
  double para_d = 0.0;
  double delta = 0.0;
  double para_aa = 0.0;
  double para_bb = 0.0;
  double para_cc = 0.0;
  for (int i = ctu_index; i < last_ctu; i++)
  {
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    double d = 0.0;
    assert(!((state->c_para[i] <= 0) || (state->k_para[i] >= 0))); //Check C and K during each solution 

    double CLCU = state->c_para[i];
    double KLCU = state->k_para[i];
    a = -CLCU * KLCU / pow(state->lcu_stats[i].pixels, KLCU - 1.0);
    b = -1.0 / (KLCU - 1.0);
    d = est_lambda;
    c = pow(a / d, b);
    para_a = para_a - c * pow(b, 3.0) / 6.0;
    para_b = para_b + (pow(b, 2.0) / 2.0 + pow(b, 3.0)*log(d) / 2.0)*c;
    para_c = para_c - (pow(b, 3.0) / 2.0*pow(log(d), 2.0) + pow(b, 2.0)*log(d) + b)*c;
    para_d = para_d + c * (1 + b * log(d) + pow(b, 2.0) / 2 * pow(log(d), 2.0) + pow(b, 3.0) / 6 * pow(log(d), 3.0));
  }

  para_d = para_d - target_bits;
  para_aa = para_b * para_b - 3 * para_a*para_c;
  para_bb = para_b * para_c - 9 * para_a*para_d;
  para_cc = para_c * para_c - 3 * para_b*para_d;

  delta = para_bb * para_bb - 4 * para_aa*para_cc;

  if (delta > 0.0)  //Check whether delta is right
  {
    double temp_x = 0.0;
    double part1 = 0.0;
    double part2 = 0.0;
    double flag1 = 0.0;
    double flag2 = 0.0;
    part1 = para_aa * para_b + 3 * para_a*(-para_bb - pow(delta, 0.5)) / 2.0;
    part2 = para_aa * para_b + 3 * para_a*(-para_bb + pow(delta, 0.5)) / 2.0;
    if (part1 < 0.0) {
      part1 = -part1;
      flag1 = -1.0;
    }
    else {
      flag1 = 1.0;
    }
    if (part2 < 0.0) {
      part2 = -part2;
      flag2 = -1.0;
    }
    else {
      flag2 = 1.0;
    }
    temp_x = (-para_b - flag1 * pow(part1, 1.0 / 3.0) - flag2 * pow(part2, 1.0 / 3.0)) / 3 / para_a;
    best_lambda = exp(temp_x);
  }
  else {
    best_lambda = est_lambda;  //Use the original picture estimated lambda for the current CTU
  }
  best_lambda = CLIP(0.001, 100000000.0, best_lambda);

  return best_lambda;
}

static INLINE double calculate_weights(encoder_state_t* const state, const int ctu_count, double est_lambda) {
  double total_weight = 0;
  for(int i = 0; i < ctu_count; i++) {
    double c_lcu = state->frame->c_para[i];
    double k_lcu = state->frame->k_para[i];
    double a = -c_lcu * k_lcu / pow(state->frame->lcu_stats[i].pixels, k_lcu - 1.0);
    double b = -1.0 / (k_lcu - 1.0);
    state->frame->lcu_stats[i].original_weight = state->frame->lcu_stats[i].weight = pow(a / est_lambda, b);
    if (state->frame->lcu_stats[i].weight < 0.01) {
      state->frame->lcu_stats[i].weight = 0.01;
    }
    total_weight += state->frame->lcu_stats[i].weight;
  }
  return total_weight;
}


void kvz_estimate_pic_lambda(encoder_state_t * const state) {
  const encoder_control_t * const encoder = state->encoder_control;

  const int layer = encoder->cfg.gop[state->frame->gop_offset].layer - (state->frame->is_irap ? 1 : 0);
  const int ctu_count = state->tile->frame->height_in_lcu * state->tile->frame->width_in_lcu;

  double alpha;
  double beta;
  if(state->frame->is_irap && encoder->cfg.intra_bit_allocation) {
    pthread_mutex_lock(&state->frame->new_ratecontrol->intra_lock);
    alpha = state->frame->new_ratecontrol->intra_alpha;
    beta = state->frame->new_ratecontrol->intra_beta;
    pthread_mutex_unlock(&state->frame->new_ratecontrol->intra_lock);
  }
  else if(state->frame->poc == 0) {
    alpha = state->frame->rc_alpha;
    beta = state->frame->rc_beta;
  }
  else {
    pthread_mutex_lock(&state->frame->new_ratecontrol->ck_frame_lock);
    alpha = -state->frame->new_ratecontrol->pic_c_para[layer] *
      state->frame->new_ratecontrol->pic_k_para[layer];
    beta = state->frame->new_ratecontrol->pic_k_para[layer] - 1;
    pthread_mutex_unlock(&state->frame->new_ratecontrol->ck_frame_lock);
  }
  double bits = pic_allocate_bits(state);
  state->frame->cur_pic_target_bits = bits;

  double est_lambda;
  int32_t num_pixels = state->encoder_control->cfg.width * state->encoder_control->cfg.height;
  double bpp = bits / num_pixels;
  if (state->frame->is_irap) {
    if(encoder->cfg.intra_bit_allocation) {
      state->frame->i_bits_left = bits;
      double temp = pow(state->frame->icost / num_pixels, BETA1);
      est_lambda = alpha / 256 * pow(temp/bpp, beta);
    }
    else {
      // arbitrary reduction to the lambda for intra frames
      est_lambda = alpha * pow(bpp, beta) * 0.5;
    }
  }
  else {
    est_lambda = alpha * pow(bpp, beta);
  }

  double temp_lambda;
  pthread_mutex_lock(&state->frame->new_ratecontrol->lambda_lock);
  if ((temp_lambda = state->frame->new_ratecontrol->previous_lambdas[layer]) > 0.0) {
    temp_lambda = CLIP(0.1, 10000.0, temp_lambda);
    est_lambda = CLIP(temp_lambda * pow(2.0, -1), temp_lambda * 2, est_lambda);
  }

  if((temp_lambda = state->frame->new_ratecontrol->previous_frame_lambda) > 0.0) {
    temp_lambda = CLIP(0.1, 2000.0, temp_lambda);
    est_lambda = CLIP(temp_lambda * pow(2.0, -10.0 / 3.0), temp_lambda * pow(2.0, 10.0 / 3.0), est_lambda);
  }
  pthread_mutex_unlock(&state->frame->new_ratecontrol->lambda_lock);

  est_lambda = CLIP(0.1, 10000.0, est_lambda);

  double total_weight = 0;

  if(!state->frame->is_irap) {
    double best_lambda = est_lambda;
    if(!state->encoder_control->cfg.frame_allocation) {
      pthread_rwlock_rdlock(&state->frame->new_ratecontrol->ck_ctu_lock[layer]);
      memcpy(state->frame->c_para, state->frame->new_ratecontrol->c_para[layer], ctu_count * sizeof(double));
      memcpy(state->frame->k_para, state->frame->new_ratecontrol->k_para[layer], ctu_count * sizeof(double));
      pthread_rwlock_unlock(&state->frame->new_ratecontrol->ck_ctu_lock[layer]);
      temp_lambda = est_lambda;
      double taylor_e3;
      int iteration_number = 0;
      do {
        taylor_e3 = 0.0;
        best_lambda = temp_lambda = solve_cubic_equation(state->frame, 0, ctu_count, temp_lambda, bits);
        for (int i = 0; i < ctu_count; ++i) {
          double CLCU = state->frame->c_para[i];
          double KLCU = state->frame->k_para[i];
          double a = -CLCU * KLCU / pow(state->frame->lcu_stats[i].pixels, KLCU - 1.0);
          double b = -1.0 / (KLCU - 1.0);
          taylor_e3 += pow(a / best_lambda, b);
        }
        iteration_number++;
      }
      while (fabs(taylor_e3 - bits) > 0.01 && iteration_number <= 11);
    }
    total_weight = calculate_weights(state, ctu_count, best_lambda);
    state->frame->remaining_weight = bits;
  }
  else {
    for (int i = 0; i < ctu_count; ++i) {
      state->frame->lcu_stats[i].weight = MAX(0.01,
        state->frame->lcu_stats[i].pixels * pow(est_lambda / alpha,
                                                1.0 / beta));
      total_weight += state->frame->lcu_stats[i].weight;
    }
  }

  for(int i = 0; i < ctu_count; ++i) {
    state->frame->lcu_stats[i].weight = bits * state->frame->lcu_stats[i].weight / total_weight;
  }

  state->frame->lambda = est_lambda;
  state->frame->QP = lambda_to_qp(est_lambda);
}


static double get_ctu_bits(encoder_state_t * const state, vector2d_t pos) {
  int avg_bits;
  const encoder_control_t * const encoder = state->encoder_control;
  
  int num_ctu = state->encoder_control->in.width_in_lcu * state->encoder_control->in.height_in_lcu;
  const int index = pos.x + pos.y * state->tile->frame->width_in_lcu;

  if (state->frame->is_irap) {
    if(encoder->cfg.intra_bit_allocation) {
      int cus_left = num_ctu - index + 1;
      int window = MIN(4, cus_left);
      double mad = kvz_get_lcu_stats(state, pos.x, pos.y)->i_cost;

      pthread_mutex_lock(&state->frame->rc_lock);
      double bits_left = state->frame->cur_pic_target_bits - state->frame->cur_frame_bits_coded;
      double weighted_bits_left = (bits_left * window + (bits_left - state->frame->i_bits_left)*cus_left) / window;
      avg_bits = mad * weighted_bits_left / state->frame->remaining_weight;
      state->frame->remaining_weight -= mad;
      state->frame->i_bits_left -= state->frame->cur_pic_target_bits * mad / state->frame->icost;
      pthread_mutex_unlock(&state->frame->rc_lock);
    }
    else {
      avg_bits = state->frame->cur_pic_target_bits * ((double)state->frame->lcu_stats[index].pixels /
        (state->encoder_control->in.height * state->encoder_control->in.width));
    }
  }
  else {
    double total_weight = 0;
    // In case wpp is used only the ctus of the current frame are safe to use
    const int used_ctu_count = MIN(4, (encoder->cfg.wpp ? (pos.y + 1) * encoder->in.width_in_lcu : num_ctu) - index);
    int target_bits = 0;
    double best_lambda = 0.0;
    double temp_lambda = state->frame->lambda;
    double taylor_e3 = 0.0;
    int iter = 0;

    int last_ctu = index + used_ctu_count;
    for (int i = index; i < last_ctu; i++) {
      target_bits += state->frame->lcu_stats[i].weight;
    }

    pthread_mutex_lock(&state->frame->rc_lock);
    total_weight = state->frame->remaining_weight;
    target_bits = MAX(target_bits + state->frame->cur_pic_target_bits - state->frame->cur_frame_bits_coded - (int)total_weight, 10);
    pthread_mutex_unlock(&state->frame->rc_lock);

    //just similar with the process at frame level, details can refer to the function kvz_estimate_pic_lambda
    do {
      taylor_e3 = 0.0;
      best_lambda = solve_cubic_equation(state->frame, index, last_ctu, temp_lambda, target_bits);
      temp_lambda = best_lambda;
      for (int i = index; i < last_ctu; i++) {
        double CLCU = state->frame->c_para[i];
        double KLCU = state->frame->k_para[i];
        double a = -CLCU * KLCU / pow((double)state->frame->lcu_stats[i].pixels, KLCU - 1.0);
        double b = -1.0 / (KLCU - 1.0);
        taylor_e3 += pow(a / best_lambda, b);
      }
      iter++;
    } while (fabs(taylor_e3 - target_bits) > 0.01 && iter < 5);

    double c_ctu = state->frame->c_para[index];
    double k_ctu = state->frame->k_para[index];
    double a = -c_ctu * k_ctu / pow(((double)state->frame->lcu_stats[index].pixels), k_ctu - 1.0);
    double b = -1.0 / (k_ctu - 1.0);

    state->frame->lcu_stats[index].weight = MAX(pow(a / best_lambda, b), 0.01);

    avg_bits = (int)(state->frame->lcu_stats[index].weight + 0.5);
  }

  if (avg_bits < 1) {
    avg_bits = 1;
  }

  return avg_bits;
}

static double qp_to_lambda(encoder_state_t* const state, int qp)
{
  const int shift_qp = 12;
  double lambda = 0.57 * pow(2.0, (qp - shift_qp) / 3.0);

  // NOTE: HM adjusts lambda for inter according to Hadamard usage in ME.
  //       SATD is currently always enabled for ME, so this has no effect.
  // bool hadamard_me = true;
  // if (!hadamard_me && state->frame->slicetype != KVZ_SLICE_I) {
  //   lambda *= 0.95;
  // }

  return lambda;
}

 void kvz_set_ctu_qp_lambda(encoder_state_t * const state, vector2d_t pos) {
  double bits = get_ctu_bits(state, pos);

  const encoder_control_t * const encoder = state->encoder_control;
  const int frame_allocation = state->encoder_control->cfg.frame_allocation;

  int index = pos.x + pos.y * state->encoder_control->in.width_in_lcu;
  lcu_stats_t* ctu = &state->frame->lcu_stats[index];
  double bpp = bits / ctu->pixels;

  double alpha;
  double beta;
  if (state->frame->is_irap && encoder->cfg.intra_bit_allocation) {
    pthread_mutex_lock(&state->frame->new_ratecontrol->intra_lock);
    alpha = state->frame->new_ratecontrol->intra_alpha;
    beta = state->frame->new_ratecontrol->intra_beta;
    pthread_mutex_unlock(&state->frame->new_ratecontrol->intra_lock);
  }
  else if(state->frame->num == 0) {
    alpha = state->frame->rc_alpha;
    beta = state->frame->rc_beta;
  }
  else {
    alpha = -state->frame->c_para[index] * state->frame->k_para[index];
    beta = state->frame->k_para[index] - 1;
  }

  double est_lambda;
  int est_qp;
  if (state->frame->is_irap && encoder->cfg.intra_bit_allocation) {
    double cost_per_pixel = (double)ctu->i_cost / ctu->pixels;
    cost_per_pixel = pow(cost_per_pixel, BETA1);
    est_lambda = alpha / 256.0 * pow(cost_per_pixel / bpp, beta);
    est_qp = state->frame->QP;
    double max_lambda = exp(((double)est_qp + 2.49 - 13.7122) / 4.2005);
    double min_lambda = exp(((double)est_qp - 2.49 - 13.7122) / 4.2005);
    est_lambda = CLIP(min_lambda, max_lambda, est_lambda);

    est_qp = lambda_to_qp(est_lambda);
  }
  else {
    // In case wpp is used the previous ctus may not be ready from above rows
    const int ctu_limit = encoder->cfg.wpp ? pos.y * encoder->in.width_in_lcu : 0;
    
    est_lambda = alpha * pow(bpp, beta) * (state->frame->is_irap ? 0.5 : 1);
    const double clip_lambda = state->frame->lambda;

    double clip_neighbor_lambda = -1;
    int clip_qp = -1;
    if (encoder->cfg.clip_neighbour || state->frame->num == 0) {
      for (int temp_index = index - 1; temp_index >= ctu_limit; --temp_index) {
        if (state->frame->lcu_stats[temp_index].lambda > 0) {
          clip_neighbor_lambda = state->frame->lcu_stats[temp_index].lambda;
          break;
        }
      }
      for (int temp_index = index - 1; temp_index >= ctu_limit; --temp_index) {
        if (state->frame->lcu_stats[temp_index].qp > -1) {
          clip_qp = state->frame->lcu_stats[temp_index].qp;
          break;
        }
      }
    }
    else {
      
      if (state->frame->lcu_stats[index].lambda > 0) {
        clip_neighbor_lambda = state->frame->previous_layer_state->frame->lcu_stats[index].lambda;
      }
      if (state->frame->lcu_stats[index].qp > 0) {
        clip_qp = state->frame->previous_layer_state->frame->lcu_stats[index].qp;
      }
    }


    if (clip_neighbor_lambda > 0) {
      est_lambda = CLIP(clip_neighbor_lambda * pow(2, -(1.0 + frame_allocation) / 3.0),
        clip_neighbor_lambda * pow(2.0, (1.0 + frame_allocation) / 3.0),
        est_lambda);
    }

    if (clip_lambda > 0) {
      est_lambda = CLIP(clip_lambda * pow(2, -(2.0 + frame_allocation) / 3.0),
        clip_lambda * pow(2.0, (1.0 + frame_allocation) / 3.0),
        est_lambda);
    }
    else {
      est_lambda = CLIP(10.0, 1000.0, est_lambda);
    }

    if (est_lambda < 0.1) {
      est_lambda = 0.1;
    }

    est_qp = lambda_to_qp(est_lambda);

    if( clip_qp > -1) {
      est_qp = CLIP(clip_qp - 1 - frame_allocation,
        clip_qp + 1 + frame_allocation,
        est_qp);
    }

    est_qp = CLIP(state->frame->QP - 2 - frame_allocation,
      state->frame->QP + 2 + frame_allocation,
      est_qp);
  }

  state->lambda = est_lambda;
  state->lambda_sqrt = sqrt(est_lambda);
  state->qp = est_qp;
  ctu->qp = est_qp;
  ctu->lambda = est_lambda;
  ctu->i_cost = 0;

  // Apply variance adaptive quantization
  if (encoder->cfg.vaq) {
    vector2d_t lcu = {
      pos.x + state->tile->lcu_offset_x,
      pos.y + state->tile->lcu_offset_y
    };
    int id = lcu.x + lcu.y * state->tile->frame->width_in_lcu;
    int aq_offset = round(state->frame->aq_offsets[id]);
    state->qp += aq_offset;
    // Maximum delta QP is clipped between [-26, 25] according to ITU T-REC-H.265 specification chapter 7.4.9.10 Transform unit semantics
    // Since this value will be later combined with qp_pred, clip to half of that instead to be safe
    state->qp = CLIP(state->frame->QP - 13, state->frame->QP + 12, state->qp);
    state->qp = CLIP_TO_QP(state->qp);
    state->lambda = qp_to_lambda(state, state->qp);
    state->lambda_sqrt = sqrt(state->lambda);

    //ctu->qp = state->qp;
    //ctu->lambda = state->lambda;
  }
}


static void update_pic_ck(encoder_state_t * const state, double bpp, double distortion, double lambda, int layer) {
  double new_k = 0, new_c;
  if(state->frame->num == 1) {
    new_k = log(distortion / state->frame->new_ratecontrol->intra_pic_distortion) /
      log(bpp / state->frame->new_ratecontrol->intra_pic_bpp);
    new_c = distortion / pow(bpp, new_k);
  }
  new_k = -bpp * lambda / distortion;
  new_c = distortion / pow(bpp, new_k);

  new_c = CLIP(+.1, 100.0, new_c);
  new_k = CLIP(-3.0, -0.001, new_k);

  if(state->frame->is_irap || state->frame->num <= (4 - state->encoder_control->cfg.frame_allocation)) {
    for(int i = 1; i < 5; i++) {
      state->frame->new_ratecontrol->pic_c_para[i] = new_c;
      state->frame->new_ratecontrol->pic_k_para[i] = new_k;
    }
  }
  else {
    state->frame->new_ratecontrol->pic_c_para[layer] = new_c;
    state->frame->new_ratecontrol->pic_k_para[layer] = new_k;
  }
}


static void update_ck(encoder_state_t * const state, int ctu_index, int layer)
{
  double bpp = (double)state->frame->lcu_stats[ctu_index].bits / state->frame->lcu_stats[ctu_index].pixels;
  double distortion = state->frame->lcu_stats[ctu_index].distortion;
  double lambda = state->frame->lcu_stats[ctu_index].lambda;

  double new_k = 0, new_c = -1;
  if (!state->frame->lcu_stats[ctu_index].skipped) {
    distortion = MAX(distortion, 0.0001);

    bpp = CLIP(0.0001, 10.0, bpp);
    new_k = -bpp * lambda / distortion;
    new_k = CLIP(-3.0, -0.001, new_k);
    new_c = distortion / pow(bpp, new_k);
    
    new_c = CLIP(+.1, 100.0, new_c);

    if (state->frame->is_irap || state->frame->num <= (4 - state->encoder_control->cfg.frame_allocation)) {
      for (int i = 1; i < 5; i++) {
        state->frame->new_ratecontrol->c_para[i][ctu_index] = new_c;
        state->frame->new_ratecontrol->k_para[i][ctu_index] = new_k;
      }
    }
    else {
      state->frame->new_ratecontrol->c_para[layer][ctu_index] = new_c;
      state->frame->new_ratecontrol->k_para[layer][ctu_index] = new_k;
    }
  }
}


void kvz_update_after_picture(encoder_state_t * const state) {
  double total_distortion = 0;
  double lambda = 0;
  int32_t pixels = (state->encoder_control->in.width * state->encoder_control->in.height);
  double pic_bpp = (double)state->frame->cur_frame_bits_coded / pixels;

  const encoder_control_t * const encoder = state->encoder_control;
  const int layer = encoder->cfg.gop[state->frame->gop_offset].layer - (state->frame->is_irap ? 1 : 0);

  if (state->frame->is_irap && encoder->cfg.intra_bit_allocation) {
    double lnbpp = log(pow(state->frame->icost / pixels, BETA1));
    pthread_mutex_lock(&state->frame->new_ratecontrol->intra_lock);
    double diff_lambda = state->frame->new_ratecontrol->intra_beta * log(state->frame->cur_frame_bits_coded) - log(state->frame->cur_pic_target_bits);

    diff_lambda = CLIP(-0.125, 0.125, 0.25*diff_lambda);

    state->frame->new_ratecontrol->intra_alpha *= exp(diff_lambda);
    state->frame->new_ratecontrol->intra_beta += diff_lambda / lnbpp;
    pthread_mutex_unlock(&state->frame->new_ratecontrol->intra_lock);
  }

  for(int y_ctu = 0; y_ctu < state->encoder_control->in.height_in_lcu; y_ctu++) {
    for (int x_ctu = 0; x_ctu < state->encoder_control->in.width_in_lcu; x_ctu++) {
      int ctu_distortion = 0;
      lcu_stats_t *ctu = kvz_get_lcu_stats(state, x_ctu, y_ctu);
      for (int y = y_ctu * 64; y < MIN((y_ctu + 1) * 64, state->tile->frame->height); y++) {
        for (int x = x_ctu * 64; x < MIN((x_ctu + 1) * 64, state->tile->frame->width); x++) {
          int temp = (int)state->tile->frame->source->y[x + y * state->encoder_control->in.width] -
            state->tile->frame->rec->y[x + y * state->encoder_control->in.width];
          ctu_distortion += temp * temp;
        }        
      }
      ctu->distortion = (double)ctu_distortion / ctu->pixels;
      total_distortion += (double)ctu_distortion / ctu->pixels;
      lambda += ctu->lambda / (state->encoder_control->in.width_in_lcu * state->encoder_control->in.height_in_lcu);
    }    
  }

  total_distortion /= (state->encoder_control->in.height_in_lcu * state->encoder_control->in.width_in_lcu);
  if (state->frame->is_irap) {
    pthread_mutex_lock(&state->frame->new_ratecontrol->intra_lock);
    for (int y_ctu = 0; y_ctu < state->encoder_control->in.height_in_lcu; y_ctu++) {
      for (int x_ctu = 0; x_ctu < state->encoder_control->in.width_in_lcu; x_ctu++) {
        lcu_stats_t *ctu = kvz_get_lcu_stats(state, x_ctu, y_ctu);
        state->frame->new_ratecontrol->intra_dis[x_ctu + y_ctu * state->encoder_control->in.width_in_lcu] =
          ctu->distortion;
        state->frame->new_ratecontrol->intra_bpp[x_ctu + y_ctu * state->encoder_control->in.width_in_lcu] =
          ctu->bits / ctu->pixels;
      }
    }
    state->frame->new_ratecontrol->intra_pic_distortion = total_distortion;
    state->frame->new_ratecontrol->intra_pic_bpp = pic_bpp;
    pthread_mutex_unlock(&state->frame->new_ratecontrol->intra_lock);
  }

  pthread_mutex_lock(&state->frame->new_ratecontrol->lambda_lock);
  state->frame->new_ratecontrol->previous_frame_lambda = lambda;
  state->frame->new_ratecontrol->previous_lambdas[layer] = lambda;
  pthread_mutex_unlock(&state->frame->new_ratecontrol->lambda_lock);

  update_pic_ck(state, pic_bpp, total_distortion, lambda, layer);
  if (state->frame->num <= 4 || state->frame->is_irap){
    for (int i = 1; i < 5; ++i) {
      pthread_rwlock_wrlock(&state->frame->new_ratecontrol->ck_ctu_lock[i]);
    }
  }
  else{
    pthread_rwlock_wrlock(&state->frame->new_ratecontrol->ck_ctu_lock[layer]);
  }
  for(int i = 0; i < state->encoder_control->in.width_in_lcu * state->encoder_control->in.height_in_lcu; i++) {
    update_ck(state, i, layer);
  }
  if (state->frame->num <= 4 || state->frame->is_irap){
    for (int i = 1; i < 5; ++i) {
      pthread_rwlock_unlock(&state->frame->new_ratecontrol->ck_ctu_lock[i]);
    }
  }
  else{
    pthread_rwlock_unlock(&state->frame->new_ratecontrol->ck_ctu_lock[layer]);
  }
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
      double qp = ctrl->cfg.qp;
      qp += gop->qp_offset;
      qp += CLIP(0.0, 3.0, qp * gop->qp_model_scale + gop->qp_model_offset);
      state->frame->QP = CLIP_TO_QP((int)(qp + 0.5));

    }
    else {
      state->frame->QP = CLIP_TO_QP(ctrl->cfg.qp + ctrl->cfg.intra_qp_offset);
    }

    state->frame->lambda = qp_to_lambda(state, state->frame->QP);
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
    state->lambda = qp_to_lambda(state, state->qp);
    state->lambda_sqrt = sqrt(state->lambda);

  }
  else if (ctrl->cfg.target_bitrate > 0) {
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

  // Apply variance adaptive quantization
  if (ctrl->cfg.vaq) {
    vector2d_t lcu = {
      pos.x + state->tile->lcu_offset_x,
      pos.y + state->tile->lcu_offset_y
    };
    int id = lcu.x + lcu.y * state->tile->frame->width_in_lcu;
    int aq_offset = round(state->frame->aq_offsets[id]);
    state->qp += aq_offset;    
    // Maximum delta QP is clipped between [-26, 25] according to ITU T-REC-H.265 specification chapter 7.4.9.10 Transform unit semantics
    // Since this value will be later combined with qp_pred, clip to half of that instead to be safe
    state->qp = CLIP(state->frame->QP - 13, state->frame->QP + 12, state->qp);
    state->qp = CLIP_TO_QP(state->qp);
    state->lambda = qp_to_lambda(state, state->qp);
    state->lambda_sqrt = sqrt(state->lambda);
  }
}
