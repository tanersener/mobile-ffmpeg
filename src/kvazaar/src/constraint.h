#ifndef CONSTRAINT_H_
#define CONSTRAINT_H_
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

#include "ml_intra_cu_depth_pred.h"
#include "encoderstate.h"


 /* Constraint structure:
  * Each field corresponds to a constraint technique. The encoder tests if the constraint
  * pointer is allocated to apply the technique.
 */
typedef struct {
  // Structure used for the CTU depth prediction using Machine Learning in All Intra 
  ml_intra_ctu_pred_t * ml_intra_depth_ctu;
} constraint_t;


void * kvz_init_constraint(encoder_state_t* state, const encoder_control_t * const);
void kvz_constraint_free(encoder_state_t* state);

#endif