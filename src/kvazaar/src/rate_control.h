#ifndef RATE_CONTROL_H_
#define RATE_CONTROL_H_
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

/**
 * \ingroup Control
 * \file
 * \brief Functions related to rate control.
 */

#include "global.h" // IWYU pragma: keep

#include "encoderstate.h"
#include "pthread.h"

typedef struct kvz_rc_data {
  double *c_para[KVZ_MAX_GOP_LAYERS];
  double *k_para[KVZ_MAX_GOP_LAYERS];
  double pic_c_para[KVZ_MAX_GOP_LAYERS];
  double pic_k_para[KVZ_MAX_GOP_LAYERS];
  double previous_lambdas[KVZ_MAX_GOP_LAYERS + 1];
  double previous_frame_lambda;
  double *intra_bpp;
  double *intra_dis;
  double intra_pic_distortion;
  double intra_pic_bpp;

  double intra_alpha;
  double intra_beta;

  pthread_rwlock_t ck_ctu_lock[KVZ_MAX_GOP_LAYERS];
  pthread_mutex_t ck_frame_lock;
  pthread_mutex_t lambda_lock;
  pthread_mutex_t intra_lock;
} kvz_rc_data;

kvz_rc_data * kvz_get_rc_data(const encoder_control_t * const encoder);
void kvz_free_rc_data();

void kvz_set_picture_lambda_and_qp(encoder_state_t * const state);

void kvz_set_lcu_lambda_and_qp(encoder_state_t * const state,
                               vector2d_t pos);

void kvz_set_ctu_qp_lambda(encoder_state_t * const state, vector2d_t pos);
void kvz_update_after_picture(encoder_state_t * const state);
void kvz_estimate_pic_lambda(encoder_state_t * const state);

#endif // RATE_CONTROL_H_
