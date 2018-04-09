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

#include "image.h"

#include <limits.h>
#include <stdlib.h>

#include "strategies/strategies-ipol.h"
#include "strategies/strategies-picture.h"
#include "threads.h"

/**
* \brief Allocate a new image with 420.
* This function signature is part of the libkvz API.
* \return image pointer or NULL on failure
*/
kvz_picture * kvz_image_alloc_420(const int32_t width, const int32_t height)
{
  return kvz_image_alloc(KVZ_CSP_420, width, height);
}

/**
 * \brief Allocate a new image.
 * \return image pointer or NULL on failure
 */
kvz_picture * kvz_image_alloc(enum kvz_chroma_format chroma_format, const int32_t width, const int32_t height)
{
  //Assert that we have a well defined image
  assert((width % 2) == 0);
  assert((height % 2) == 0);

  kvz_picture *im = MALLOC(kvz_picture, 1);
  if (!im) return NULL;

  unsigned int luma_size = width * height;
  unsigned chroma_sizes[] = { 0, luma_size / 4, luma_size / 2, luma_size };
  unsigned chroma_size = chroma_sizes[chroma_format];

  im->chroma_format = chroma_format;

  //Allocate memory
  im->fulldata = MALLOC(kvz_pixel, (luma_size + 2 * chroma_size));
  if (!im->fulldata) {
    free(im);
    return NULL;
  }

  im->base_image = im;
  im->refcount = 1; //We give a reference to caller
  im->width = width;
  im->height = height;
  im->stride = width;
  im->chroma_format = chroma_format;

  im->y = im->data[COLOR_Y] = &im->fulldata[0];

  if (chroma_format == KVZ_CSP_400) {
    im->u = im->data[COLOR_U] = NULL;
    im->v = im->data[COLOR_V] = NULL;
  } else {
    im->u = im->data[COLOR_U] = &im->fulldata[luma_size];
    im->v = im->data[COLOR_V] = &im->fulldata[luma_size + chroma_size];
  }

  im->pts = 0;
  im->dts = 0;

  im->interlacing = KVZ_INTERLACING_NONE;

  return im;
}

/**
 * \brief Free an image.
 *
 * Decrement reference count of the image and deallocate associated memory
 * if no references exist any more.
 *
 * \param im image to free
 */
void kvz_image_free(kvz_picture *const im)
{
  if (im == NULL) return;

  int32_t new_refcount = KVZ_ATOMIC_DEC(&(im->refcount));
  if (new_refcount > 0) {
    // There are still references so we don't free the data yet.
    return;
  }

  if (im->base_image != im) {
    // Free our reference to the base image.
    kvz_image_free(im->base_image);
  } else {
    free(im->fulldata);
  }

  // Make sure freed data won't be used.
  im->base_image = NULL;
  im->fulldata = NULL;
  im->y = im->u = im->v = NULL;
  im->data[COLOR_Y] = im->data[COLOR_U] = im->data[COLOR_V] = NULL;
  free(im);
}

/**
 * \brief Get a new pointer to an image.
 *
 * Increment reference count and return the image.
 */
kvz_picture *kvz_image_copy_ref(kvz_picture *im)
{
  // The caller should have had another reference.
  assert(im->refcount > 0);
  KVZ_ATOMIC_INC(&(im->refcount));

  return im;
}

kvz_picture *kvz_image_make_subimage(kvz_picture *const orig_image,
                             const unsigned x_offset,
                             const unsigned y_offset,
                             const unsigned width,
                             const unsigned height)
{
  // Assert that we have a well defined image
  assert((width % 2) == 0);
  assert((height % 2) == 0);

  assert((x_offset % 2) == 0);
  assert((y_offset % 2) == 0);

  assert(x_offset + width <= orig_image->width);
  assert(y_offset + height <= orig_image->height);

  kvz_picture *im = MALLOC(kvz_picture, 1);
  if (!im) return NULL;

  im->base_image = kvz_image_copy_ref(orig_image->base_image);
  im->refcount = 1; // We give a reference to caller
  im->width = width;
  im->height = height;
  im->stride = orig_image->stride;
  im->chroma_format = orig_image->chroma_format;

  im->y = im->data[COLOR_Y] = &orig_image->y[x_offset + y_offset * orig_image->stride];
  if (orig_image->chroma_format != KVZ_CSP_400) {
    im->u = im->data[COLOR_U] = &orig_image->u[x_offset / 2 + y_offset / 2 * orig_image->stride / 2];
    im->v = im->data[COLOR_V] = &orig_image->v[x_offset / 2 + y_offset / 2 * orig_image->stride / 2];
  }

  im->pts = 0;
  im->dts = 0;

  return im;
}

yuv_t * kvz_yuv_t_alloc(int luma_size, int chroma_size)
{
  yuv_t *yuv = (yuv_t *)malloc(sizeof(*yuv));
  yuv->size = luma_size;

  // Get buffers with separate mallocs in order to take advantage of
  // automatic buffer overrun checks.
  yuv->y = (kvz_pixel *)malloc(luma_size * sizeof(*yuv->y));
  if (chroma_size == 0) {
    yuv->u = NULL;
    yuv->v = NULL;
  } else {
    yuv->u = (kvz_pixel *)malloc(chroma_size * sizeof(*yuv->u));
    yuv->v = (kvz_pixel *)malloc(chroma_size * sizeof(*yuv->v));
  }
  
  return yuv;
}

void kvz_yuv_t_free(yuv_t *yuv)
{
  if (yuv) {
    FREE_POINTER(yuv->y);
    FREE_POINTER(yuv->u);
    FREE_POINTER(yuv->v);
  }
  FREE_POINTER(yuv);
}

hi_prec_buf_t * kvz_hi_prec_buf_t_alloc(int luma_size)
{
  // Get buffers with separate mallocs in order to take advantage of
  // automatic buffer overrun checks.
  hi_prec_buf_t *yuv = (hi_prec_buf_t *)malloc(sizeof(*yuv));
  yuv->y = (int16_t *)malloc(luma_size * sizeof(*yuv->y));
  yuv->u = (int16_t *)malloc(luma_size / 2 * sizeof(*yuv->u));
  yuv->v = (int16_t *)malloc(luma_size / 2 * sizeof(*yuv->v));
  yuv->size = luma_size;

  return yuv;
}

void kvz_hi_prec_buf_t_free(hi_prec_buf_t * yuv)
{
  free(yuv->y);
  free(yuv->u);
  free(yuv->v);
  free(yuv);
}


/**
 * \brief Diagonally interpolate SAD outside the frame.
 *
 * \param data1   Starting point of the first picture.
 * \param data2   Starting point of the second picture.
 * \param width   Width of the region for which SAD is calculated.
 * \param height  Height of the region for which SAD is calculated.
 * \param width  Width of the pixel array.
 *
 * \returns Sum of Absolute Differences
 */
static unsigned cor_sad(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                        int block_width, int block_height, unsigned pic_stride)
{
  kvz_pixel ref = *ref_data;
  int x, y;
  unsigned sad = 0;

  for (y = 0; y < block_height; ++y) {
    for (x = 0; x < block_width; ++x) {
      sad += abs(pic_data[y * pic_stride + x] - ref);
    }
  }

  return sad;
}

/**
 * \brief Vertically interpolate SAD outside the frame.
 *
 * \param data1   Starting point of the first picture.
 * \param data2   Starting point of the second picture.
 * \param width   Width of the region for which SAD is calculated.
 * \param height  Height of the region for which SAD is calculated.
 * \param width  Width of the pixel array.
 *
 * \returns Sum of Absolute Differences
 */
static unsigned ver_sad(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                        int block_width, int block_height, unsigned pic_stride)
{
  int x, y;
  unsigned sad = 0;

  for (y = 0; y < block_height; ++y) {
    for (x = 0; x < block_width; ++x) {
      sad += abs(pic_data[y * pic_stride + x] - ref_data[x]);
    }
  }

  return sad;
}

/**
 * \brief Horizontally interpolate SAD outside the frame.
 *
 * \param data1   Starting point of the first picture.
 * \param data2   Starting point of the second picture.
 * \param width   Width of the region for which SAD is calculated.
 * \param height  Height of the region for which SAD is calculated.
 * \param width   Width of the pixel array.
 *
 * \returns Sum of Absolute Differences
 */
static unsigned hor_sad(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                        int block_width, int block_height, unsigned pic_stride, unsigned ref_stride)
{
  int x, y;
  unsigned sad = 0;

  for (y = 0; y < block_height; ++y) {
    for (x = 0; x < block_width; ++x) {
      sad += abs(pic_data[y * pic_stride + x] - ref_data[y * ref_stride]);
    }
  }

  return sad;
}


/**
 * \brief  Handle special cases of comparing blocks that are not completely
 *         inside the frame.
 *
 * \param pic  First frame.
 * \param ref  Second frame.
 * \param pic_x  X coordinate of the first block.
 * \param pic_y  Y coordinate of the first block.
 * \param ref_x  X coordinate of the second block.
 * \param ref_y  Y coordinate of the second block.
 * \param block_width  Width of the blocks.
 * \param block_height  Height of the blocks.
 */
static unsigned image_interpolated_sad(const kvz_picture *pic, const kvz_picture *ref,
                                 int pic_x, int pic_y, int ref_x, int ref_y,
                                 int block_width, int block_height)
{
  kvz_pixel *pic_data, *ref_data;

  int left, right, top, bottom;
  int result = 0;

  // Change the movement vector to point right next to the frame. This doesn't
  // affect the result but removes some special cases.
  if (ref_x > ref->width)            ref_x = ref->width;
  if (ref_y > ref->height)           ref_y = ref->height;
  if (ref_x + block_width < 0)  ref_x = -block_width;
  if (ref_y + block_height < 0) ref_y = -block_height;

  // These are the number of pixels by how far the movement vector points
  // outside the frame. They are always >= 0. If all of them are 0, the
  // movement vector doesn't point outside the frame.
  left   = (ref_x < 0) ? -ref_x : 0;
  top    = (ref_y < 0) ? -ref_y : 0;
  right  = (ref_x + block_width  > ref->width)  ? ref_x + block_width  - ref->width  : 0;
  bottom = (ref_y + block_height > ref->height) ? ref_y + block_height - ref->height : 0;

  // Center picture to the current block and reference to the point where
  // movement vector is pointing to. That point might be outside the buffer,
  // but that is ok because we project the movement vector to the buffer
  // before dereferencing the pointer.
  pic_data = &pic->y[pic_y * pic->stride + pic_x];
  ref_data = &ref->y[ref_y * ref->stride + ref_x];

  // The handling of movement vectors that point outside the picture is done
  // in the following way.
  // - Correct the index of ref_data so that it points to the top-left
  //   of the area we want to compare against.
  // - Correct the index of pic_data to point inside the current block, so
  //   that we compare the right part of the block to the ref_data.
  // - Reduce block_width and block_height so that the the size of the area
  //   being compared is correct.
  if (top && left) {
    result += cor_sad(pic_data,
                      &ref_data[top * ref->stride + left],
                      left, top, pic->stride);
    result += ver_sad(&pic_data[left],
                      &ref_data[top * ref->stride + left],
                      block_width - left, top, pic->stride);
    result += hor_sad(&pic_data[top * pic->stride],
                      &ref_data[top * ref->stride + left],
                      left, block_height - top, pic->stride, ref->stride);
    result += kvz_reg_sad(&pic_data[top * pic->stride + left],
                      &ref_data[top * ref->stride + left],
                      block_width - left, block_height - top, pic->stride, ref->stride);
  } else if (top && right) {
    result += ver_sad(pic_data,
                      &ref_data[top * ref->stride],
                      block_width - right, top, pic->stride);
    result += cor_sad(&pic_data[block_width - right],
                      &ref_data[top * ref->stride + (block_width - right - 1)],
                      right, top, pic->stride);
    result += kvz_reg_sad(&pic_data[top * pic->stride],
                      &ref_data[top * ref->stride],
                      block_width - right, block_height - top, pic->stride, ref->stride);
    result += hor_sad(&pic_data[top * pic->stride + (block_width - right)],
                      &ref_data[top * ref->stride + (block_width - right - 1)],
                      right, block_height - top, pic->stride, ref->stride);
  } else if (bottom && left) {
    result += hor_sad(pic_data,
                      &ref_data[left],
                      left, block_height - bottom, pic->stride, ref->stride);
    result += kvz_reg_sad(&pic_data[left],
                      &ref_data[left],
                      block_width - left, block_height - bottom, pic->stride, ref->stride);
    result += cor_sad(&pic_data[(block_height - bottom) * pic->stride],
                      &ref_data[(block_height - bottom - 1) * ref->stride + left],
                      left, bottom, pic->stride);
    result += ver_sad(&pic_data[(block_height - bottom) * pic->stride + left],
                      &ref_data[(block_height - bottom - 1) * ref->stride + left],
                      block_width - left, bottom, pic->stride);
  } else if (bottom && right) {
    result += kvz_reg_sad(pic_data,
                      ref_data,
                      block_width - right, block_height - bottom, pic->stride, ref->stride);
    result += hor_sad(&pic_data[block_width - right],
                      &ref_data[block_width - right - 1],
                      right, block_height - bottom, pic->stride, ref->stride);
    result += ver_sad(&pic_data[(block_height - bottom) * pic->stride],
                      &ref_data[(block_height - bottom - 1) * ref->stride],
                      block_width - right, bottom, pic->stride);
    result += cor_sad(&pic_data[(block_height - bottom) * pic->stride + block_width - right],
                      &ref_data[(block_height - bottom - 1) * ref->stride + block_width - right - 1],
                      right, bottom, pic->stride);
  } else if (top) {
    result += ver_sad(pic_data,
                      &ref_data[top * ref->stride],
                      block_width, top, pic->stride);
    result += kvz_reg_sad(&pic_data[top * pic->stride],
                      &ref_data[top * ref->stride],
                      block_width, block_height - top, pic->stride, ref->stride);
  } else if (bottom) {
    result += kvz_reg_sad(pic_data,
                      ref_data,
                      block_width, block_height - bottom, pic->stride, ref->stride);
    result += ver_sad(&pic_data[(block_height - bottom) * pic->stride],
                      &ref_data[(block_height - bottom - 1) * ref->stride],
                      block_width, bottom, pic->stride);
  } else if (left) {
    result += hor_sad(pic_data,
                      &ref_data[left],
                      left, block_height, pic->stride, ref->stride);
    result += kvz_reg_sad(&pic_data[left],
                      &ref_data[left],
                      block_width - left, block_height, pic->stride, ref->stride);
  } else if (right) {
    result += kvz_reg_sad(pic_data,
                      ref_data,
                      block_width - right, block_height, pic->stride, ref->stride);
    result += hor_sad(&pic_data[block_width - right],
                      &ref_data[block_width - right - 1],
                      right, block_height, pic->stride, ref->stride);
  } else {
    result += kvz_reg_sad(pic_data, ref_data, block_width, block_height, pic->stride, ref->stride);
  }

  return result;
}


/**
* \brief Calculate interpolated SAD between two blocks.
*
* \param pic        Image for the block we are trying to find.
* \param ref        Image where we are trying to find the block.
*
* \returns          Sum of absolute differences
*/
unsigned kvz_image_calc_sad(const kvz_picture *pic,
                            const kvz_picture *ref,
                            int pic_x,
                            int pic_y,
                            int ref_x,
                            int ref_y,
                            int block_width,
                            int block_height)
{
  assert(pic_x >= 0 && pic_x <= pic->width - block_width);
  assert(pic_y >= 0 && pic_y <= pic->height - block_height);

  if (ref_x >= 0 && ref_x <= ref->width  - block_width &&
      ref_y >= 0 && ref_y <= ref->height - block_height)
  {
    // Reference block is completely inside the frame, so just calculate the
    // SAD directly. This is the most common case, which is why it's first.
    const kvz_pixel *pic_data = &pic->y[pic_y * pic->stride + pic_x];
    const kvz_pixel *ref_data = &ref->y[ref_y * ref->stride + ref_x];
    return kvz_reg_sad(pic_data, ref_data, block_width, block_height, pic->stride, ref->stride)>>(KVZ_BIT_DEPTH-8);
  } else {
    // Call a routine that knows how to interpolate pixels outside the frame.
    return image_interpolated_sad(pic, ref, pic_x, pic_y, ref_x, ref_y, block_width, block_height) >> (KVZ_BIT_DEPTH - 8);
  }
}


/**
* \brief Calculate interpolated SATD between two blocks.
*
* \param pic        Image for the block we are trying to find.
* \param ref        Image where we are trying to find the block.
*/
unsigned kvz_image_calc_satd(const kvz_picture *pic,
                             const kvz_picture *ref,
                             int pic_x,
                             int pic_y,
                             int ref_x,
                             int ref_y,
                             int block_width,
                             int block_height)
{
  assert(pic_x >= 0 && pic_x <= pic->width - block_width);
  assert(pic_y >= 0 && pic_y <= pic->height - block_height);

  if (ref_x >= 0 && ref_x <= ref->width  - block_width &&
      ref_y >= 0 && ref_y <= ref->height - block_height)
  {
    // Reference block is completely inside the frame, so just calculate the
    // SAD directly. This is the most common case, which is why it's first.
    const kvz_pixel *pic_data = &pic->y[pic_y * pic->stride + pic_x];
    const kvz_pixel *ref_data = &ref->y[ref_y * ref->stride + ref_x];
    return kvz_satd_any_size(block_width,
                             block_height,
                             pic_data,
                             pic->stride,
                             ref_data,
                             ref->stride) >> (KVZ_BIT_DEPTH - 8);
  } else {
    // Extrapolate pixels from outside the frame.
    kvz_extended_block block;
    kvz_get_extended_block(pic_x,
                           pic_y,
                           ref_x - pic_x,
                           ref_y - pic_y,
                           0,
                           0,
                           ref->y,
                           ref->width,
                           ref->height,
                           0,
                           block_width,
                           block_height,
                           &block);

    const kvz_pixel *pic_data = &pic->y[pic_y * pic->stride + pic_x];

    unsigned satd = kvz_satd_any_size(block_width,
                                      block_height,
                                      pic_data,
                                      pic->stride,
                                      block.buffer,
                                      block.stride) >> (KVZ_BIT_DEPTH - 8);

    if (block.malloc_used) {
      FREE_POINTER(block.buffer);
    }

    return satd;
  }
}




/**
 * \brief BLock Image Transfer from one buffer to another.
 *
 * It's a stupidly simple loop that copies pixels.
 *
 * \param orig  Start of the originating buffer.
 * \param dst  Start of the destination buffer.
 * \param width  Width of the copied region.
 * \param height  Height of the copied region.
 * \param orig_stride  Width of a row in the originating buffer.
 * \param dst_stride  Width of a row in the destination buffer.
 *
 * This should be inlined, but it's defined here for now to see if Visual
 * Studios LTCG will inline it.
 */
#define BLIT_PIXELS_CASE(n) case n:\
  for (y = 0; y < n; ++y) {\
    memcpy(&dst[y*dst_stride], &orig[y*orig_stride], n * sizeof(kvz_pixel));\
  }\
  break;

void kvz_pixels_blit(const kvz_pixel * const orig, kvz_pixel * const dst,
                         const unsigned width, const unsigned height,
                         const unsigned orig_stride, const unsigned dst_stride)
{
  unsigned y;
  //There is absolutely no reason to have a width greater than the source or the destination stride.
  assert(width <= orig_stride);
  assert(width <= dst_stride);

#ifdef CHECKPOINTS
  char *buffer = malloc((3 * width + 1) * sizeof(char));
  for (y = 0; y < height; ++y) {
    int p;
    for (p = 0; p < width; ++p) {
      sprintf((buffer + 3*p), "%02X ", orig[y*orig_stride]);
    }
    buffer[3*width] = 0;
    CHECKPOINT("kvz_pixels_blit_avx2: %04d: %s", y, buffer);
  }
  FREE_POINTER(buffer);
#endif //CHECKPOINTS

  if (width == orig_stride && width == dst_stride) {
    memcpy(dst, orig, width * height * sizeof(kvz_pixel));
    return;
  }

  int nxn_width = (width == height) ? width : 0;
  switch (nxn_width) {
    BLIT_PIXELS_CASE(4)
    BLIT_PIXELS_CASE(8)
    BLIT_PIXELS_CASE(16)
    BLIT_PIXELS_CASE(32)
    BLIT_PIXELS_CASE(64)
  default:

    if (orig == dst) {
      //If we have the same array, then we should have the same stride
      assert(orig_stride == dst_stride);
      return;
    }
    assert(orig != dst || orig_stride == dst_stride);

    for (y = 0; y < height; ++y) {
      memcpy(&dst[y*dst_stride], &orig[y*orig_stride], width * sizeof(kvz_pixel));
    }
    break;
  }
}
