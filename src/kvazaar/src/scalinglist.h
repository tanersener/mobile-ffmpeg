#ifndef SCALINGLIST_H_
#define SCALINGLIST_H_
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
 * Scaling list initialization.
 */

#include <stdio.h>

#include "global.h" // IWYU pragma: keep


typedef struct {
        int8_t   enable;
        int32_t  scaling_list_dc   [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];
  const int32_t *scaling_list_coeff[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];
  const int32_t *quant_coeff[4][6][6];
  const int32_t *de_quant_coeff  [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM][SCALING_LIST_REM_NUM];
  const double *error_scale[4][6][6];
} scaling_list_t;

extern const uint8_t kvz_g_scaling_list_num[4];
extern const uint16_t kvz_g_scaling_list_size[4];

const int32_t *kvz_scalinglist_get_default(const uint32_t size_id, const uint32_t list_id);

void kvz_scalinglist_init(scaling_list_t * const scaling_list);
void kvz_scalinglist_destroy(scaling_list_t * const scaling_list);

int  kvz_scalinglist_parse(scaling_list_t * const scaling_list, FILE *fp);
void kvz_scalinglist_process(scaling_list_t *scaling_list, uint8_t bitdepth);






#endif
