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

#include "constraint.h"

 /**
  * \brief Allocate the constraint_t structure.
  *
  * \param state   encoder state
  * \return the pointer of constraint_t structure
  */
void * kvz_init_constraint(encoder_state_t* state, const encoder_control_t * const encoder) {
  constraint_t* constr = NULL;
  // Allocate the constraint_t strucutre
  constr = MALLOC(constraint_t, 1);
  if (!constr) {
    fprintf(stderr, "Memory allocation failed!\n");
    assert(0);
  }

  // Allocate the ml_intra_ctu_pred_t structure
  constr->ml_intra_depth_ctu = NULL;
  if (encoder->cfg.ml_pu_depth_intra) // TODO: Change this by a new param !!
  {
    constr->ml_intra_depth_ctu = kvz_init_ml_intra_depth_const();
  }
  return constr;
}

/**
 * \brief Deallocate the constraint_t structure.
 *
 * \param state   encoder state
 */
void kvz_constraint_free(encoder_state_t* state) {
  constraint_t* constr = state->constraint;
  if (constr->ml_intra_depth_ctu) 
  {
    kvz_end_ml_intra_depth_const(constr->ml_intra_depth_ctu);
  }
  FREE_POINTER(constr);
}
