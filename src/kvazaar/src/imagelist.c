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

#include "imagelist.h"

#include <stdio.h>
#include <stdlib.h>

#include "image.h"
#include "threads.h"


/**
 * \brief Allocate memory for image_list
 * \param size  initial array size
 * \return image_list pointer, NULL on failure
 */
image_list_t * kvz_image_list_alloc(int size)
{
  image_list_t *list = (image_list_t *)malloc(sizeof(image_list_t));
  list->size      = size;
  list->images    = malloc(sizeof(kvz_picture*)  * size);
  list->cu_arrays = malloc(sizeof(cu_array_t*)   * size);
  list->pocs      = malloc(sizeof(int32_t)       * size);
  list->ref_LXs   = malloc(sizeof(*list->ref_LXs) * size);
  list->used_size = 0;

  return list;
}

/**
 * \brief Resize image_list array
 * \param list  image_list pointer
 * \param size  new array size
 * \return 1 on success, 0 on failure
 */
int kvz_image_list_resize(image_list_t *list, unsigned size)
{
  list->images = (kvz_picture**)realloc(list->images, sizeof(kvz_picture*) * size);
  list->cu_arrays = (cu_array_t**)realloc(list->cu_arrays, sizeof(cu_array_t*) * size);
  list->pocs = realloc(list->pocs, sizeof(int32_t) * size);
  list->ref_LXs = realloc(list->ref_LXs, sizeof(*list->ref_LXs) * size);
  list->size = size;
  return size == 0 || (list->images && list->cu_arrays && list->pocs);
}

/**
 * \brief Free memory allocated to the picture_list
 * \param list image_list pointer
 * \return 1 on success, 0 on failure
 */
int kvz_image_list_destroy(image_list_t *list)
{
  unsigned int i;
  if (list->used_size > 0) {
    for (i = 0; i < list->used_size; ++i) {
      kvz_image_free(list->images[i]);
      list->images[i] = NULL;
      kvz_cu_array_free(&list->cu_arrays[i]);
      list->cu_arrays[i] = NULL;
      list->pocs[i] = 0;
      for (int j = 0; j < 16; j++) {
        list->ref_LXs[i][0][j] = 0;
        list->ref_LXs[i][1][j] = 0;
      }
    }
  }

  if (list->size > 0) {
    free(list->images);
    free(list->cu_arrays);
    free(list->pocs);
    free(list->ref_LXs);
  }
  list->images = NULL;
  list->cu_arrays = NULL;
  list->pocs = NULL;
  list->ref_LXs = NULL;
  free(list);
  return 1;
}

/**
 * \brief Add picture to the front of the picturelist
 * \param pic picture pointer to add
 * \param picture_list list to use
 * \return 1 on success
 */
int kvz_image_list_add(image_list_t *list, kvz_picture *im, cu_array_t *cua, int32_t poc, uint8_t ref_LX[2][16])
{
  int i = 0;
  if (KVZ_ATOMIC_INC(&(im->refcount)) == 1) {
    fprintf(stderr, "Tried to add an unreferenced picture. This is a bug!\n");
    assert(0); //Stop for debugging
    return 0;
  }
  
  if (KVZ_ATOMIC_INC(&(cua->refcount)) == 1) {
    fprintf(stderr, "Tried to add an unreferenced cu_array. This is a bug!\n");
    assert(0); //Stop for debugging
    return 0;
  }

  if (list->size == list->used_size) {
    unsigned new_size = MAX(list->size + 1, list->size * 2);
    if (!kvz_image_list_resize(list, new_size)) return 0;
  }
  
  for (i = list->used_size; i > 0; i--) {
    list->images[i] = list->images[i - 1];
    list->cu_arrays[i] = list->cu_arrays[i - 1];
    list->pocs[i] = list->pocs[i - 1];
    for (int j = 0; j < 16; j++) {
      list->ref_LXs[i][0][j] = list->ref_LXs[i - 1][0][j];
      list->ref_LXs[i][1][j] = list->ref_LXs[i - 1][1][j];
    }
  }

  list->images[0] = im;
  list->cu_arrays[0] = cua;
  list->pocs[0] = poc;
  for (int j = 0; j < 16; j++) {
    list->ref_LXs[0][0][j] = ref_LX[0][j];
    list->ref_LXs[0][1][j] = ref_LX[1][j];
  }
  
  list->used_size++;
  return 1;
}

/**
 * \brief Remove picture from picturelist
 * \param list list to use
 * \param n index to remove
 * \return 1 on success
 */
int kvz_image_list_rem(image_list_t * const list, const unsigned n)
{
  // Must be within list boundaries
  if (n >= list->used_size)
  {
    return 0;
  }

  kvz_image_free(list->images[n]);

  kvz_cu_array_free(&list->cu_arrays[n]);

  // The last item is easy to remove
  if (n == list->used_size - 1) {
    list->images[n] = NULL;
    list->cu_arrays[n] = NULL;
    list->pocs[n] = 0;
    for (int j = 0; j < 16; j++) {
      list->ref_LXs[n][0][j] = 0;
      list->ref_LXs[n][1][j] = 0;
    }
    list->used_size--;
  } else {
    int i = n;
    // Shift all following pics one backward in the list
    for (i = n; i < list->used_size - 1; ++i) {
      list->images[i] = list->images[i + 1];
      list->cu_arrays[i] = list->cu_arrays[i + 1];
      list->pocs[i] = list->pocs[i + 1];
      for (int j = 0; j < 16; j++) {
        list->ref_LXs[i][0][j] = list->ref_LXs[i + 1][0][j];
        list->ref_LXs[i][1][j] = list->ref_LXs[i + 1][1][j];
      }
    }
    list->images[list->used_size - 1] = NULL;
    list->cu_arrays[list->used_size - 1] = NULL;
    list->pocs[list->used_size - 1] = 0;
    for (int j = 0; j < 16; j++) {
      list->ref_LXs[list->used_size - 1][0][j] = 0;
      list->ref_LXs[list->used_size - 1][1][j] = 0;
    }
    list->used_size--;
  }

  return 1;
}

int kvz_image_list_copy_contents(image_list_t *target, image_list_t *source) {
  int i;
  while (target->used_size > 0) {
    kvz_image_list_rem(target, 0);
  }
  
  for (i = source->used_size - 1; i >= 0; --i) {
    kvz_image_list_add(target, source->images[i], source->cu_arrays[i], source->pocs[i], source->ref_LXs[i]);
  }
  return 1;
}
