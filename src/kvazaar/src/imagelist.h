#ifndef PICTURE_LIST_H_
#define PICTURE_LIST_H_
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
 * \ingroup DataStructures
 * \file
 *  Container for a list of reference pictures.
 */

#include "cu.h"
#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"


/**
 * \brief Struct which contains array of picture structs
 */
typedef struct
{
  struct kvz_picture* *images;          //!< \brief Pointer to array of picture pointers.
  cu_array_t* *cu_arrays;
  int32_t *pocs;
  uint8_t (*ref_LXs)[2][16]; //!< L0 and L1 reference index list for each image
  uint32_t size;       //!< \brief Array size.
  uint32_t used_size;


} image_list_t;

image_list_t * kvz_image_list_alloc(int size);
int kvz_image_list_resize(image_list_t *list, unsigned size);
int kvz_image_list_destroy(image_list_t *list);
int kvz_image_list_add(image_list_t *list, kvz_picture *im, cu_array_t* cua, int32_t poc, uint8_t ref_LX[2][16]);
int kvz_image_list_rem(image_list_t *list, unsigned n);

int kvz_image_list_copy_contents(image_list_t *target, image_list_t *source);

enum { REF_PIC_LIST_0 = 0, REF_PIC_LIST_1 = 1, REF_PIC_LIST_X = 100 };

#endif //PICTURE_LIST_H_
