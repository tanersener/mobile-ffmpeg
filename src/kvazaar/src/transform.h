#ifndef TRANSFORM_H_
#define TRANSFORM_H_
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
 * \ingroup Reconstruction
 * \file
 * Quantization and transform functions.
 */

#include "cu.h"
#include "encoder.h"
#include "encoderstate.h"
#include "global.h" // IWYU pragma: keep


extern const uint8_t kvz_g_chroma_scale[58];
extern const int16_t kvz_g_inv_quant_scales[6];

void kvz_transformskip(const encoder_control_t *encoder, int16_t *block,int16_t *coeff, int8_t block_size);
void kvz_itransformskip(const encoder_control_t *encoder, int16_t *block,int16_t *coeff, int8_t block_size);

void kvz_transform2d(const encoder_control_t *encoder, int16_t *block,int16_t *coeff, int8_t block_size, int32_t mode);
void kvz_itransform2d(const encoder_control_t *encoder, int16_t *block,int16_t *coeff, int8_t block_size, int32_t mode);

int32_t kvz_get_scaled_qp(int8_t type, int8_t qp, int8_t qp_offset);

void kvz_quantize_lcu_residual(encoder_state_t *state,
                               bool luma,
                               bool chroma,
                               int32_t x,
                               int32_t y,
                               uint8_t depth,
                               cu_info_t *cur_cu,
                               lcu_t* lcu);

#endif
