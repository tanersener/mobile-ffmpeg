#ifndef CU_H_
#define CU_H_
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
 * Coding Unit data structure and related functions.
 */

#include "global.h" // IWYU pragma: keep
#include "image.h"
#include "kvazaar.h"


//Cu stuff
//////////////////////////////////////////////////////////////////////////
// CONSTANTS

typedef enum {
  CU_NOTSET = 0,
  CU_INTRA  = 1,
  CU_INTER  = 2,
  CU_PCM    = 3,
} cu_type_t;

typedef enum {
  SIZE_2Nx2N = 0,
  SIZE_2NxN  = 1,
  SIZE_Nx2N  = 2,
  SIZE_NxN   = 3,
  SIZE_2NxnU = 4,
  SIZE_2NxnD = 5,
  SIZE_nLx2N = 6,
  SIZE_nRx2N = 7,
} part_mode_t;

extern const uint8_t kvz_part_mode_num_parts[];
extern const uint8_t kvz_part_mode_offsets[][4][2];
extern const uint8_t kvz_part_mode_sizes[][4][2];

/**
 * \brief Get the x coordinate of a PU.
 *
 * \param part_mode   partition mode of the containing CU
 * \param cu_width    width of the containing CU
 * \param cu_x        x coordinate of the containing CU
 * \param i           number of the PU
 * \return            location of the left edge of the PU
 */
#define PU_GET_X(part_mode, cu_width, cu_x, i) \
  ((cu_x) + kvz_part_mode_offsets[(part_mode)][(i)][0] * (cu_width) / 4)

/**
 * \brief Get the y coordinate of a PU.
 *
 * \param part_mode   partition mode of the containing CU
 * \param cu_width    width of the containing CU
 * \param cu_y        y coordinate of the containing CU
 * \param i           number of the PU
 * \return            location of the top edge of the PU
 */
#define PU_GET_Y(part_mode, cu_width, cu_y, i) \
  ((cu_y) + kvz_part_mode_offsets[(part_mode)][(i)][1] * (cu_width) / 4)

/**
 * \brief Get the width of a PU.
 *
 * \param part_mode   partition mode of the containing CU
 * \param cu_width    width of the containing CU
 * \param i           number of the PU
 * \return            width of the PU
 */
#define PU_GET_W(part_mode, cu_width, i) \
  (kvz_part_mode_sizes[(part_mode)][(i)][0] * (cu_width) / 4)

/**
 * \brief Get the height of a PU.
 *
 * \param part_mode   partition mode of the containing CU
 * \param cu_width    width of the containing CU
 * \param i           number of the PU
 * \return            height of the PU
 */
#define PU_GET_H(part_mode, cu_width, i) \
  (kvz_part_mode_sizes[(part_mode)][(i)][1] * (cu_width) / 4)

//////////////////////////////////////////////////////////////////////////
// TYPES

typedef struct {
  int x;
  int y;
} vector2d_t;

/**
 * \brief Struct for CU info
 */
typedef struct
{
  uint8_t type      : 2; //!< \brief block type, one of cu_type_t values
  uint8_t depth     : 3; //!< \brief depth / size of this block
  uint8_t part_size : 3; //!< \brief partition mode, one of part_mode_t values
  uint8_t tr_depth  : 3; //!< \brief transform depth
  uint8_t skipped   : 1; //!< \brief flag to indicate this block is skipped
  uint8_t merged    : 1; //!< \brief flag to indicate this block is merged
  uint8_t merge_idx : 3; //!< \brief merge index

  uint16_t cbf;

  /**
   * \brief QP used for the CU.
   *
   * This is required for deblocking when per-LCU QPs are enabled.
   */
  uint8_t qp;

  union {
    struct {
      int8_t mode;
      int8_t mode_chroma;
      int8_t tr_skip;    //!< \brief transform skip flag
#if KVZ_SEL_ENCRYPTION
      int8_t mode_encry;
#endif
    } intra;
    struct {
      int16_t mv[2][2];  // \brief Motion vectors for L0 and L1
      uint8_t mv_ref[2]; // \brief Index of the L0 and L1 array.
      uint8_t mv_cand0 : 3; // \brief selected MV candidate
      uint8_t mv_cand1 : 3; // \brief selected MV candidate
      uint8_t mv_dir   : 2; // \brief Probably describes if mv_ref is L0, L1 or both (bi-pred)
    } inter;
  };
} cu_info_t;

#define CU_GET_MV_CAND(cu_info_ptr, reflist) \
  (((reflist) == 0) ? (cu_info_ptr)->inter.mv_cand0 : (cu_info_ptr)->inter.mv_cand1)

#define CU_SET_MV_CAND(cu_info_ptr, reflist, value) \
  do { \
    if ((reflist) == 0) { \
      (cu_info_ptr)->inter.mv_cand0 = (value); \
    } else { \
      (cu_info_ptr)->inter.mv_cand1 = (value); \
    } \
  } while (0)

#define CHECKPOINT_CU(prefix_str, cu) CHECKPOINT(prefix_str " type=%d depth=%d part_size=%d tr_depth=%d coded=%d " \
  "skipped=%d merged=%d merge_idx=%d cbf.y=%d cbf.u=%d cbf.v=%d " \
  "intra[0].cost=%u intra[0].bitcost=%u intra[0].mode=%d intra[0].mode_chroma=%d intra[0].tr_skip=%d " \
  "intra[1].cost=%u intra[1].bitcost=%u intra[1].mode=%d intra[1].mode_chroma=%d intra[1].tr_skip=%d " \
  "intra[2].cost=%u intra[2].bitcost=%u intra[2].mode=%d intra[2].mode_chroma=%d intra[2].tr_skip=%d " \
  "intra[3].cost=%u intra[3].bitcost=%u intra[3].mode=%d intra[3].mode_chroma=%d intra[3].tr_skip=%d " \
  "inter.cost=%u inter.bitcost=%u inter.mv[0]=%d inter.mv[1]=%d inter.mvd[0]=%d inter.mvd[1]=%d " \
  "inter.mv_cand=%d inter.mv_ref=%d inter.mv_dir=%d inter.mode=%d" \
  , (cu).type, (cu).depth, (cu).part_size, (cu).tr_depth, (cu).coded, \
  (cu).skipped, (cu).merged, (cu).merge_idx, (cu).cbf.y, (cu).cbf.u, (cu).cbf.v, \
  (cu).intra[0].cost, (cu).intra[0].bitcost, (cu).intra[0].mode, (cu).intra[0].mode_chroma, (cu).intra[0].tr_skip, \
  (cu).intra[1].cost, (cu).intra[1].bitcost, (cu).intra[1].mode, (cu).intra[1].mode_chroma, (cu).intra[1].tr_skip, \
  (cu).intra[2].cost, (cu).intra[2].bitcost, (cu).intra[2].mode, (cu).intra[2].mode_chroma, (cu).intra[2].tr_skip, \
  (cu).intra[3].cost, (cu).intra[3].bitcost, (cu).intra[3].mode, (cu).intra[3].mode_chroma, (cu).intra[3].tr_skip, \
  (cu).inter.cost, (cu).inter.bitcost, (cu).inter.mv[0], (cu).inter.mv[1], (cu).inter.mvd[0], (cu).inter.mvd[1], \
  (cu).inter.mv_cand, (cu).inter.mv_ref, (cu).inter.mv_dir, (cu).inter.mode)

typedef struct cu_array_t {
  struct cu_array_t *base; //!< \brief base cu array or NULL
  cu_info_t *data;  //!< \brief cu array
  int32_t width;    //!< \brief width of the array in pixels
  int32_t height;   //!< \brief height of the array in pixels
  int32_t stride;   //!< \brief stride of the array in pixels
  int32_t refcount; //!< \brief number of references to this cu_array
} cu_array_t;

cu_info_t* kvz_cu_array_at(cu_array_t *cua, unsigned x_px, unsigned y_px);
const cu_info_t* kvz_cu_array_at_const(const cu_array_t *cua, unsigned x_px, unsigned y_px);

cu_array_t * kvz_cu_array_alloc(const int width, const int height);
cu_array_t * kvz_cu_subarray(cu_array_t *base,
                             const unsigned x_offset,
                             const unsigned y_offset,
                             const unsigned width,
                             const unsigned height);
void kvz_cu_array_free(cu_array_t **cua_ptr);
cu_array_t * kvz_cu_array_copy_ref(cu_array_t* cua);


/**
 * \brief Return the 7 lowest-order bits of the pixel coordinate.
 *
 * The 7 lower-order bits correspond to the distance from the left or top edge
 * of the containing LCU.
 */
#define SUB_SCU(xy) ((xy) & (LCU_WIDTH - 1))

#define LCU_CU_WIDTH 16
#define LCU_T_CU_WIDTH (LCU_CU_WIDTH + 1)
#define LCU_CU_OFFSET (LCU_T_CU_WIDTH + 1)
#define SCU_WIDTH (LCU_WIDTH / LCU_CU_WIDTH)

// Width from top left of the LCU, so +1 for ref buffer size.
#define LCU_REF_PX_WIDTH (LCU_WIDTH + LCU_WIDTH / 2)

/**
 * Top and left intra reference pixels for LCU.
 * - Intra needs maximum of 32 to the right and down from LCU border.
 * - First pixel is the top-left pixel.
 */
typedef struct {
  kvz_pixel y[LCU_REF_PX_WIDTH + 1];
  kvz_pixel u[LCU_REF_PX_WIDTH / 2 + 1];
  kvz_pixel v[LCU_REF_PX_WIDTH / 2 + 1];
} lcu_ref_px_t;

/**
 * \brief Coefficients of an LCU
 *
 * Coefficients inside a single TU are stored in row-major order. TUs
 * themselves are stored in a zig-zag order, so that the coefficients of
 * a TU are contiguous in memory.
 *
 * Example storage order for a 32x32 pixel TU tree
 *
 \verbatim

   +------+------+------+------+---------------------------+
   |   0  |  16  |  64  |  80  |                           |
   |   -  |   -  |   -  |   -  |                           |
   |  15  |  31  |  79  |  95  |                           |
   +------+------+------+------+                           |
   |  32  |  48  |  96  | 112  |                           |
   |   -  |   -  |   -  |   -  |                           |
   |  47  |  63  | 111  | 127  |                           |
   +------+------+------+------+         256 - 511         |
   | 128  | 144  | 192  | 208  |                           |
   |   -  |   -  |   -  |   -  |                           |
   | 143  | 159  | 207  | 223  |                           |
   +------+------+------+------+                           |
   | 160  | 176  | 224  | 240  |                           |
   |   -  |   -  |   -  |   -  |                           |
   | 175  | 191  | 239  | 255  |                           |
   +------+------+------+------+-------------+------+------+
   | 512  | 528  |             |             | 832  | 848  |
   |   -  |   -  |             |             |   -  |   -  |
   | 527  | 543  |             |             | 847  | 863  |
   +------+------+  576 - 639  |  768 - 831  +------+------+
   | 544  | 560  |             |             | 864  | 880  |
   |   -  |   -  |             |             |   -  |   -  |
   | 559  | 575  |             |             | 879  | 895  |
   +------+------+-------------+-------------+------+------+
   |             |             |             |             |
   |             |             |             |             |
   |             |             |             |             |
   |  640 - 703  |  704 - 767  |  896 - 959  |  960 - 1023 |
   |             |             |             |             |
   |             |             |             |             |
   |             |             |             |             |
   +-------------+-------------+-------------+-------------+

 \endverbatim
 */
typedef ALIGNED(8) struct {
  coeff_t y[LCU_LUMA_SIZE];
  coeff_t u[LCU_CHROMA_SIZE];
  coeff_t v[LCU_CHROMA_SIZE];
} lcu_coeff_t;


typedef struct {
  lcu_ref_px_t top_ref;  //!< Reference pixels from adjacent LCUs.
  lcu_ref_px_t left_ref; //!< Reference pixels from adjacent LCUs.
  lcu_yuv_t ref; //!< LCU reference pixels
  lcu_yuv_t rec; //!< LCU reconstructed pixels
  /**
   * We get the coefficients as a byproduct of doing reconstruction during the
   * search. It might be more efficient to recalculate the final coefficients
   * once we know the final modes rather than copying them.
   */
  lcu_coeff_t coeff; //!< LCU coefficients

  /**
   * A 17x17 CU array, plus the top right reference CU.
   * - Top reference CUs at indices [0,16] (row 0).
   * - Left reference CUs at indices 17*n where n is in [0,16] (column 0).
   * - All CUs of this LCU at indices 17*y + x where x,y are in [1,16].
   * - Top right reference CU at the last index.
   *
   * The figure below shows how the indices map to CU locations.
   *
   \verbatim

       .-- left reference CUs
       v
        0 |   1   2  . . .  16 | 289 <-- top reference CUs
     -----+--------------------+----
       17 |  18  19  . . .  33 |
       34 |  35  36  . . .  50 <-- this LCU
        . |   .   .  .       . |
        . |   .   .    .     . |
        . |   .   .      .   . |
      272 | 273 274  . . . 288 |
     -----+--------------------+----

   \endverbatim
   */
  cu_info_t cu[LCU_T_CU_WIDTH * LCU_T_CU_WIDTH + 1];
} lcu_t;

void kvz_cu_array_copy_from_lcu(cu_array_t* dst, int dst_x, int dst_y, const lcu_t *src);

/**
 * \brief Return pointer to the top right reference CU.
 */
#define LCU_GET_TOP_RIGHT_CU(lcu) \
  (&(lcu)->cu[LCU_T_CU_WIDTH * LCU_T_CU_WIDTH])

/**
 * \brief Return pointer to the CU containing a given pixel.
 *
 * \param lcu   pointer to the containing LCU
 * \param x_px  x-coordinate relative to the upper left corner of the LCU
 * \param y_px  y-coordinate relative to the upper left corner of the LCU
 * \return      pointer to the CU at coordinates (x_px, y_px)
 */
#define LCU_GET_CU_AT_PX(lcu, x_px, y_px) \
  (&(lcu)->cu[LCU_CU_OFFSET + ((x_px) >> 2) + ((y_px) >> 2) * LCU_T_CU_WIDTH])


/**
 * \brief  Copy a part of a coeff_t array to another.
 *
 * \param width  Size of the block to be copied in pixels.
 * \param src    Pointer to the source array.
 * \param dest   Pointer to the destination array.
 */
static INLINE void copy_coeffs(const coeff_t *__restrict src,
                               coeff_t *__restrict dest,
                               size_t width)
{
  memcpy(dest, src, width * width * sizeof(coeff_t));
}


/**
 * \brief  Convert (x, y) coordinates to z-order index.
 *
 * Only works for widths and coordinates divisible by four. Width must be
 * a power of two in range [4..64].
 *
 * \param width   size of the containing block
 * \param x       x-coordinate
 * \param y       y-coordinate
 * \return        index in z-order
 */
static INLINE unsigned xy_to_zorder(unsigned width, unsigned x, unsigned y)
{
  assert(width % 4 == 0 && width >= 4 && width <= 64);
  assert(x % 4 == 0 && x < width);
  assert(y % 4 == 0 && y < width);

  unsigned result = 0;

  switch (width) {
    case 64:
      result += x / 32 * (32*32);
      result += y / 32 * (64*32);
      x %= 32;
      y %= 32;
      // fallthrough
    case 32:
      result += x / 16 * (16*16);
      result += y / 16 * (32*16);
      x %= 16;
      y %= 16;
      // fallthrough
    case 16:
      result += x / 8 * ( 8*8);
      result += y / 8 * (16*8);
      x %= 8;
      y %= 8;
      // fallthrough
    case 8:
      result += x / 4 * (4*4);
      result += y / 4 * (8*4);
      // fallthrough
    case 4:
      break;
  }

  return result;
}


#define CHECKPOINT_LCU(prefix_str, lcu) do { \
  CHECKPOINT_CU(prefix_str " cu[0]", (lcu).cu[0]); \
  CHECKPOINT_CU(prefix_str " cu[1]", (lcu).cu[1]); \
  CHECKPOINT_CU(prefix_str " cu[2]", (lcu).cu[2]); \
  CHECKPOINT_CU(prefix_str " cu[3]", (lcu).cu[3]); \
  CHECKPOINT_CU(prefix_str " cu[4]", (lcu).cu[4]); \
  CHECKPOINT_CU(prefix_str " cu[5]", (lcu).cu[5]); \
  CHECKPOINT_CU(prefix_str " cu[6]", (lcu).cu[6]); \
  CHECKPOINT_CU(prefix_str " cu[7]", (lcu).cu[7]); \
  CHECKPOINT_CU(prefix_str " cu[8]", (lcu).cu[8]); \
  CHECKPOINT_CU(prefix_str " cu[9]", (lcu).cu[9]); \
  CHECKPOINT_CU(prefix_str " cu[10]", (lcu).cu[10]); \
  CHECKPOINT_CU(prefix_str " cu[11]", (lcu).cu[11]); \
  CHECKPOINT_CU(prefix_str " cu[12]", (lcu).cu[12]); \
  CHECKPOINT_CU(prefix_str " cu[13]", (lcu).cu[13]); \
  CHECKPOINT_CU(prefix_str " cu[14]", (lcu).cu[14]); \
  CHECKPOINT_CU(prefix_str " cu[15]", (lcu).cu[15]); \
  CHECKPOINT_CU(prefix_str " cu[16]", (lcu).cu[16]); \
  CHECKPOINT_CU(prefix_str " cu[17]", (lcu).cu[17]); \
  CHECKPOINT_CU(prefix_str " cu[18]", (lcu).cu[18]); \
  CHECKPOINT_CU(prefix_str " cu[19]", (lcu).cu[19]); \
  CHECKPOINT_CU(prefix_str " cu[20]", (lcu).cu[20]); \
  CHECKPOINT_CU(prefix_str " cu[21]", (lcu).cu[21]); \
  CHECKPOINT_CU(prefix_str " cu[22]", (lcu).cu[22]); \
  CHECKPOINT_CU(prefix_str " cu[23]", (lcu).cu[23]); \
  CHECKPOINT_CU(prefix_str " cu[24]", (lcu).cu[24]); \
  CHECKPOINT_CU(prefix_str " cu[25]", (lcu).cu[25]); \
  CHECKPOINT_CU(prefix_str " cu[26]", (lcu).cu[26]); \
  CHECKPOINT_CU(prefix_str " cu[27]", (lcu).cu[27]); \
  CHECKPOINT_CU(prefix_str " cu[28]", (lcu).cu[28]); \
  CHECKPOINT_CU(prefix_str " cu[29]", (lcu).cu[29]); \
  CHECKPOINT_CU(prefix_str " cu[30]", (lcu).cu[30]); \
  CHECKPOINT_CU(prefix_str " cu[31]", (lcu).cu[31]); \
  CHECKPOINT_CU(prefix_str " cu[32]", (lcu).cu[32]); \
  CHECKPOINT_CU(prefix_str " cu[33]", (lcu).cu[33]); \
  CHECKPOINT_CU(prefix_str " cu[34]", (lcu).cu[34]); \
  CHECKPOINT_CU(prefix_str " cu[35]", (lcu).cu[35]); \
  CHECKPOINT_CU(prefix_str " cu[36]", (lcu).cu[36]); \
  CHECKPOINT_CU(prefix_str " cu[37]", (lcu).cu[37]); \
  CHECKPOINT_CU(prefix_str " cu[38]", (lcu).cu[38]); \
  CHECKPOINT_CU(prefix_str " cu[39]", (lcu).cu[39]); \
  CHECKPOINT_CU(prefix_str " cu[40]", (lcu).cu[40]); \
  CHECKPOINT_CU(prefix_str " cu[41]", (lcu).cu[41]); \
  CHECKPOINT_CU(prefix_str " cu[42]", (lcu).cu[42]); \
  CHECKPOINT_CU(prefix_str " cu[43]", (lcu).cu[43]); \
  CHECKPOINT_CU(prefix_str " cu[44]", (lcu).cu[44]); \
  CHECKPOINT_CU(prefix_str " cu[45]", (lcu).cu[45]); \
  CHECKPOINT_CU(prefix_str " cu[46]", (lcu).cu[46]); \
  CHECKPOINT_CU(prefix_str " cu[47]", (lcu).cu[47]); \
  CHECKPOINT_CU(prefix_str " cu[48]", (lcu).cu[48]); \
  CHECKPOINT_CU(prefix_str " cu[49]", (lcu).cu[49]); \
  CHECKPOINT_CU(prefix_str " cu[50]", (lcu).cu[50]); \
  CHECKPOINT_CU(prefix_str " cu[51]", (lcu).cu[51]); \
  CHECKPOINT_CU(prefix_str " cu[52]", (lcu).cu[52]); \
  CHECKPOINT_CU(prefix_str " cu[53]", (lcu).cu[53]); \
  CHECKPOINT_CU(prefix_str " cu[54]", (lcu).cu[54]); \
  CHECKPOINT_CU(prefix_str " cu[55]", (lcu).cu[55]); \
  CHECKPOINT_CU(prefix_str " cu[56]", (lcu).cu[56]); \
  CHECKPOINT_CU(prefix_str " cu[57]", (lcu).cu[57]); \
  CHECKPOINT_CU(prefix_str " cu[58]", (lcu).cu[58]); \
  CHECKPOINT_CU(prefix_str " cu[59]", (lcu).cu[59]); \
  CHECKPOINT_CU(prefix_str " cu[60]", (lcu).cu[60]); \
  CHECKPOINT_CU(prefix_str " cu[61]", (lcu).cu[61]); \
  CHECKPOINT_CU(prefix_str " cu[62]", (lcu).cu[62]); \
  CHECKPOINT_CU(prefix_str " cu[63]", (lcu).cu[63]); \
  CHECKPOINT_CU(prefix_str " cu[64]", (lcu).cu[64]); \
  CHECKPOINT_CU(prefix_str " cu[65]", (lcu).cu[65]); \
  CHECKPOINT_CU(prefix_str " cu[66]", (lcu).cu[66]); \
  CHECKPOINT_CU(prefix_str " cu[67]", (lcu).cu[67]); \
  CHECKPOINT_CU(prefix_str " cu[68]", (lcu).cu[68]); \
  CHECKPOINT_CU(prefix_str " cu[69]", (lcu).cu[69]); \
  CHECKPOINT_CU(prefix_str " cu[70]", (lcu).cu[70]); \
  CHECKPOINT_CU(prefix_str " cu[71]", (lcu).cu[71]); \
  CHECKPOINT_CU(prefix_str " cu[72]", (lcu).cu[72]); \
  CHECKPOINT_CU(prefix_str " cu[73]", (lcu).cu[73]); \
  CHECKPOINT_CU(prefix_str " cu[74]", (lcu).cu[74]); \
  CHECKPOINT_CU(prefix_str " cu[75]", (lcu).cu[75]); \
  CHECKPOINT_CU(prefix_str " cu[76]", (lcu).cu[76]); \
  CHECKPOINT_CU(prefix_str " cu[77]", (lcu).cu[77]); \
  CHECKPOINT_CU(prefix_str " cu[78]", (lcu).cu[78]); \
  CHECKPOINT_CU(prefix_str " cu[79]", (lcu).cu[79]); \
  CHECKPOINT_CU(prefix_str " cu[80]", (lcu).cu[80]); \
  CHECKPOINT_CU(prefix_str " cu[81]", (lcu).cu[81]); \
} while(0)


#define NUM_CBF_DEPTHS 5
static const uint16_t cbf_masks[NUM_CBF_DEPTHS] = { 0x1f, 0x0f, 0x07, 0x03, 0x1 };

/**
 * Check if CBF in a given level >= depth is true.
 */
static INLINE int cbf_is_set(uint16_t cbf, int depth, color_t plane)
{
  return (cbf & (cbf_masks[depth] << (NUM_CBF_DEPTHS * plane))) != 0;
}

/**
 * Check if CBF in a given level >= depth is true.
 */
static INLINE int cbf_is_set_any(uint16_t cbf, int depth)
{
  return cbf_is_set(cbf, depth, COLOR_Y) ||
         cbf_is_set(cbf, depth, COLOR_U) ||
         cbf_is_set(cbf, depth, COLOR_V);
}

/**
 * Set CBF in a level to true.
 */
static INLINE void cbf_set(uint16_t *cbf, int depth, color_t plane)
{
  // Return value of the bit corresponding to the level.
  *cbf |= (0x10 >> depth) << (NUM_CBF_DEPTHS * plane);
}

/**
 * Set CBF in a level to true if it is set at a lower level in any of
 * the child_cbfs.
 */
static INLINE void cbf_set_conditionally(uint16_t *cbf, uint16_t child_cbfs[3], int depth, color_t plane)
{
  bool child_cbf_set = cbf_is_set(child_cbfs[0], depth + 1, plane) ||
                       cbf_is_set(child_cbfs[1], depth + 1, plane) ||
                       cbf_is_set(child_cbfs[2], depth + 1, plane);
  if (child_cbf_set) {
    cbf_set(cbf, depth, plane);
  }
}

/**
 * Set CBF in a levels <= depth to false.
 */
static INLINE void cbf_clear(uint16_t *cbf, int depth, color_t plane)
{
  *cbf &= ~(cbf_masks[depth] << (NUM_CBF_DEPTHS * plane));
}

/**
 * Copy cbf flags.
 */
static INLINE void cbf_copy(uint16_t *cbf, uint16_t src, color_t plane)
{
  cbf_clear(cbf, 0, plane);
  *cbf |= src & (cbf_masks[0] << (NUM_CBF_DEPTHS * plane));
}

#define GET_SPLITDATA(CU,curDepth) ((CU)->depth > curDepth)
#define SET_SPLITDATA(CU,flag) { (CU)->split=(flag); }

#endif
