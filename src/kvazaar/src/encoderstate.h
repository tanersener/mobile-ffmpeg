#ifndef ENCODERSTATE_H_
#define ENCODERSTATE_H_
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
 * Top level of the encoder implementation.
 */

#include "bitstream.h"
#include "cabac.h"
#include "cu.h"
#include "encoder.h"
#include "global.h" // IWYU pragma: keep
#include "image.h"
#include "imagelist.h"
#include "kvazaar.h"
#include "tables.h"
#include "threadqueue.h"
#include "videoframe.h"
#include "extras/crypto.h"


typedef enum {
  ENCODER_STATE_TYPE_INVALID = 'i',
  ENCODER_STATE_TYPE_MAIN = 'M',
  ENCODER_STATE_TYPE_SLICE = 'S',
  ENCODER_STATE_TYPE_TILE = 'T',
  ENCODER_STATE_TYPE_WAVEFRONT_ROW = 'W',
} encoder_state_type;


typedef struct lcu_stats_t {
  //! \brief Number of bits that were spent
  uint32_t bits;

  //! \brief Weight of the LCU for rate control
  double weight;

  //! \brief Lambda value which was used for this LCU
  double lambda;

  //! \brief Rate control alpha parameter
  double rc_alpha;

  //! \brief Rate control beta parameter
  double rc_beta;
} lcu_stats_t;


typedef struct encoder_state_config_frame_t {
  /**
   * \brief Frame-level lambda.
   *
   * Use state->lambda or state->lambda_sqrt for cost computations.
   *
   * \see encoder_state_t::lambda
   * \see encoder_state_t::lambda_sqrt
   */
  double lambda;

  int32_t num;       /*!< \brief Frame number */
  int32_t poc;       /*!< \brief Picture order count */
  int8_t gop_offset; /*!< \brief Offset in the gop structure */
  int32_t irap_poc;  /*!< \brief POC of the associated IRAP picture */

  /**
   * \brief Frame-level quantization parameter
   *
   * \see encoder_state_t::qp
   */
  int8_t QP;
  //! \brief quantization factor
  double QP_factor;

  //! Current pictures available for references
  image_list_t *ref;
  int8_t ref_list;

  //! L0 and L1 reference index list
  uint8_t ref_LX[2][16];
  //! L0 reference index list size
  uint8_t ref_LX_size[2];

  bool is_irap;
  uint8_t pictype;
  enum kvz_slice_type slicetype;

  //! Total number of bits written.
  uint64_t total_bits_coded;

  //! Number of bits written in the current GOP.
  uint64_t cur_gop_bits_coded;

  //! Number of bits targeted for the current GOP.
  double cur_gop_target_bits;

  //! Number of bits targeted for the current picture.
  double cur_pic_target_bits;

  // Parameters used in rate control
  double rc_alpha;
  double rc_beta;

  /**
   * \brief Indicates that this encoder state is ready for encoding the
   * next frame i.e. kvz_encoder_prepare has been called.
   */
  bool prepared;

  /**
   * \brief Indicates that the previous frame has been encoded and the
   * encoded data written and the encoding the next frame has not been
   * started yet.
   */
  bool done;

  /**
   * \brief Information about the coded LCUs.
   *
   * Used for rate control.
   */
  lcu_stats_t *lcu_stats;

  /**
   * \brief Whether next NAL is the first NAL in the access unit.
   */
  bool first_nal;

} encoder_state_config_frame_t;

typedef struct encoder_state_config_tile_t {
  //Current sub-frame
  videoframe_t *frame;
  
  int32_t id;

  //Tile: offset in LCU for current encoder_state in global coordinates
  int32_t lcu_offset_x;
  int32_t lcu_offset_y;

  //Tile: offset in pixels
  int32_t offset_x;
  int32_t offset_y;

  //Position of the first element in tile scan in global coordinates
  int32_t lcu_offset_in_ts;
  
  // This is a buffer for the non-loopfiltered bottom pixels of every LCU-row
  // in the tile. They are packed such that each LCU-row index maps to the
  // y-coordinate.
  yuv_t *hor_buf_search;
  // This is a buffer for the non-loopfiltered rightmost pixels of every
  // LCU-column. They are packed such that each LCU-column index maps to the
  // x-coordinate.
  yuv_t *ver_buf_search;

  // This is a buffer for the deblocked bottom pixels of every LCU in the
  // tile. They are packed such that each LCU-row index maps to the
  // y-coordinate.
  yuv_t *hor_buf_before_sao;

  // This is a buffer for the deblocked right pixels of every LCU in the
  // tile. They are packed such that each LCU-column index maps to the
  // x-coordinate.
  yuv_t *ver_buf_before_sao;

  //Jobs for each individual LCU of a wavefront row.
  threadqueue_job_t **wf_jobs;

} encoder_state_config_tile_t;

typedef struct encoder_state_config_slice_t {
  int32_t id;
  
  //Global coordinates
  int32_t start_in_ts;
  int32_t end_in_ts;
  
  //Global coordinates
  int32_t start_in_rs;
  int32_t end_in_rs;
} encoder_state_config_slice_t;

typedef struct encoder_state_config_wfrow_t {
  //Row in tile coordinates of the wavefront
  int32_t lcu_offset_y;
} encoder_state_config_wfrow_t;

typedef struct lcu_order_element {
  //This it used for leaf of the encoding tree. All is relative to the tile.
  int id;
  int index;
  struct encoder_state_t *encoder_state;
  vector2d_t position;
  vector2d_t position_px; //Top-left
  vector2d_t size;
  int first_column;
  int first_row;
  int last_column;
  int last_row;
  
  struct lcu_order_element *above;
  struct lcu_order_element *below;
  struct lcu_order_element *left;
  struct lcu_order_element *right;
} lcu_order_element_t;

typedef struct encoder_state_t {
  const encoder_control_t *encoder_control;
  encoder_state_type type;

  //List of children, the last item of this list is a pseudo-encoder with encoder_control = NULL
  //Use for (i = 0; encoder_state->children[i].encoder_control; ++i) {
  struct encoder_state_t *children;
  struct encoder_state_t *parent;
  
  //Pointer to the encoder_state of the previous frame
  struct encoder_state_t *previous_encoder_state;
  
  encoder_state_config_frame_t  *frame;
  encoder_state_config_tile_t   *tile;
  encoder_state_config_slice_t  *slice;
  encoder_state_config_wfrow_t  *wfrow;
  
  int is_leaf; //A leaf encoder state is one which should encode LCUs...
  lcu_order_element_t *lcu_order;
  uint32_t lcu_order_count;
  
  bitstream_t stream;
  cabac_data_t cabac;

  // Crypto stuff
  crypto_handle_t *crypto_hdl;
  uint32_t crypto_prev_pos;

  uint32_t stats_bitstream_length; //Bitstream length written in bytes

  //! \brief Lambda for SSE
  double lambda;
  //! \brief Lambda for SAD and SATD
  double lambda_sqrt;
  //! \brief Quantization parameter for the current LCU
  int8_t qp;

  /**
   * \brief Whether a QP delta value must be coded for the current LCU.
   */
  bool must_code_qp_delta;

  /**
   * \brief Reference for computing QP delta for the next LCU that is coded
   * next. Updated whenever a QP delta is coded.
   */
  int8_t ref_qp;

  /**
   * \brief Coeffs for the LCU.
   */
  lcu_coeff_t *coeff;

  //Jobs to wait for
  threadqueue_job_t * tqj_recon_done; //Reconstruction is done
  threadqueue_job_t * tqj_bitstream_written; //Bitstream is written
} encoder_state_t;

void kvz_encode_one_frame(encoder_state_t * const state, kvz_picture* frame);

void kvz_encoder_prepare(encoder_state_t *state);


int kvz_encoder_state_match_children_of_previous_frame(encoder_state_t * const state);

coeff_scan_order_t kvz_get_scan_order(int8_t cu_type, int intra_mode, int depth);

void kvz_encoder_create_ref_lists(const encoder_state_t *const state);

lcu_stats_t* kvz_get_lcu_stats(encoder_state_t *state, int lcu_x, int lcu_y);


/**
 * Whether the parameter sets should be written with the current frame.
 */
static INLINE bool encoder_state_must_write_vps(const encoder_state_t *state)
{
  const int32_t frame = state->frame->num;
  const int32_t vps_period = state->encoder_control->cfg.vps_period;

  return (vps_period >  0 && frame % vps_period == 0) ||
         (vps_period >= 0 && frame == 0);
}

static const uint8_t g_group_idx[32] = {
  0, 1, 2, 3, 4, 4, 5, 5, 6, 6,
  6, 6, 7, 7, 7, 7, 8, 8, 8, 8,
  8, 8, 8, 8, 9, 9, 9, 9, 9, 9,
  9, 9 };

static const uint8_t g_min_in_group[10] = {
  0, 1, 2, 3, 4, 6, 8, 12, 16, 24 };


#define C1FLAG_NUMBER 8 // maximum number of largerThan1 flag coded in one chunk
#define C2FLAG_NUMBER 1 // maximum number of largerThan2 flag coded in one chunk

//Get the data for vertical buffer position at the left of LCU identified by the position in pixel
#define OFFSET_VER_BUF(position_x, position_y, cur_pic, i) ((position_y) + i + ((position_x)/LCU_WIDTH - 1) * (cur_pic)->height)
#define OFFSET_VER_BUF_C(position_x, position_y, cur_pic, i) ((position_y/2) + i + ((position_x)/LCU_WIDTH - 1) * (cur_pic)->height / 2)

//Get the data for horizontal buffer position at the top of LCU identified by the position in pixel
#define OFFSET_HOR_BUF(position_x, position_y, cur_pic, i) ((position_x) + i + ((position_y)/LCU_WIDTH - 1) * (cur_pic)->width)
#define OFFSET_HOR_BUF_C(position_x, position_y, cur_pic, i) ((position_x/2) + i + ((position_y)/LCU_WIDTH - 1) * (cur_pic)->width / 2)
  
/** @} */

#endif //ENCODERSTATE_H_
