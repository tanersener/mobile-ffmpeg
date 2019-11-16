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

#include "rdo.h"

#include <stdlib.h>
#include <string.h>

#include "cabac.h"
#include "context.h"
#include "encode_coding_tree.h"
#include "encoder.h"
#include "imagelist.h"
#include "inter.h"
#include "scalinglist.h"
#include "strategyselector.h"
#include "tables.h"
#include "transform.h"

#include "strategies/strategies-quant.h"


#define QUANT_SHIFT          14
#define SCAN_SET_SIZE        16
#define LOG2_SCAN_SET_SIZE    4
#define SBH_THRESHOLD         4

const uint32_t kvz_g_go_rice_range[5] = { 7, 14, 26, 46, 78 };
const uint32_t kvz_g_go_rice_prefix_len[5] = { 8, 7, 6, 5, 4 };

/**
 * Entropy bits to estimate coded bits in RDO / RDOQ (From HM 12.0)
 */
const uint32_t kvz_entropy_bits[128] =
{
  0x08000, 0x08000, 0x076da, 0x089a0, 0x06e92, 0x09340, 0x0670a, 0x09cdf, 0x06029, 0x0a67f, 0x059dd, 0x0b01f, 0x05413, 0x0b9bf, 0x04ebf, 0x0c35f,
  0x049d3, 0x0ccff, 0x04546, 0x0d69e, 0x0410d, 0x0e03e, 0x03d22, 0x0e9de, 0x0397d, 0x0f37e, 0x03619, 0x0fd1e, 0x032ee, 0x106be, 0x02ffa, 0x1105d,
  0x02d37, 0x119fd, 0x02aa2, 0x1239d, 0x02836, 0x12d3d, 0x025f2, 0x136dd, 0x023d1, 0x1407c, 0x021d2, 0x14a1c, 0x01ff2, 0x153bc, 0x01e2f, 0x15d5c,
  0x01c87, 0x166fc, 0x01af7, 0x1709b, 0x0197f, 0x17a3b, 0x0181d, 0x183db, 0x016d0, 0x18d7b, 0x01595, 0x1971b, 0x0146c, 0x1a0bb, 0x01354, 0x1aa5a,
  0x0124c, 0x1b3fa, 0x01153, 0x1bd9a, 0x01067, 0x1c73a, 0x00f89, 0x1d0da, 0x00eb7, 0x1da79, 0x00df0, 0x1e419, 0x00d34, 0x1edb9, 0x00c82, 0x1f759,
  0x00bda, 0x200f9, 0x00b3c, 0x20a99, 0x00aa5, 0x21438, 0x00a17, 0x21dd8, 0x00990, 0x22778, 0x00911, 0x23118, 0x00898, 0x23ab8, 0x00826, 0x24458,
  0x007ba, 0x24df7, 0x00753, 0x25797, 0x006f2, 0x26137, 0x00696, 0x26ad7, 0x0063f, 0x27477, 0x005ed, 0x27e17, 0x0059f, 0x287b6, 0x00554, 0x29156,
  0x0050e, 0x29af6, 0x004cc, 0x2a497, 0x0048d, 0x2ae35, 0x00451, 0x2b7d6, 0x00418, 0x2c176, 0x003e2, 0x2cb15, 0x003af, 0x2d4b5, 0x0037f, 0x2de55
};

// Entropy bits scaled so that 50% probability yields 1 bit.
const float kvz_f_entropy_bits[128] =
{
  1.0, 1.0,
  0.92852783203125, 1.0751953125,
  0.86383056640625, 1.150390625,
  0.80499267578125, 1.225555419921875,
  0.751251220703125, 1.300750732421875,
  0.702056884765625, 1.375946044921875,
  0.656829833984375, 1.451141357421875,
  0.615203857421875, 1.526336669921875,
  0.576751708984375, 1.601531982421875,
  0.54119873046875, 1.67669677734375,
  0.508209228515625, 1.75189208984375,
  0.47760009765625, 1.82708740234375,
  0.449127197265625, 1.90228271484375,
  0.422637939453125, 1.97747802734375,
  0.39788818359375, 2.05267333984375,
  0.37481689453125, 2.127838134765625,
  0.353240966796875, 2.203033447265625,
  0.33306884765625, 2.278228759765625,
  0.31414794921875, 2.353424072265625,
  0.29644775390625, 2.428619384765625,
  0.279815673828125, 2.5037841796875,
  0.26422119140625, 2.5789794921875,
  0.24957275390625, 2.6541748046875,
  0.235809326171875, 2.7293701171875,
  0.222869873046875, 2.8045654296875,
  0.210662841796875, 2.879730224609375,
  0.199188232421875, 2.954925537109375,
  0.188385009765625, 3.030120849609375,
  0.17822265625, 3.105316162109375,
  0.168609619140625, 3.180511474609375,
  0.1595458984375, 3.255706787109375,
  0.1510009765625, 3.33087158203125,
  0.1429443359375, 3.40606689453125,
  0.135345458984375, 3.48126220703125,
  0.128143310546875, 3.55645751953125,
  0.121368408203125, 3.63165283203125,
  0.114959716796875, 3.706817626953125,
  0.10888671875, 3.782012939453125,
  0.1031494140625, 3.857208251953125,
  0.09771728515625, 3.932403564453125,
  0.09259033203125, 4.007598876953125,
  0.0877685546875, 4.082794189453125,
  0.083160400390625, 4.157958984375,
  0.078826904296875, 4.233154296875,
  0.07470703125, 4.308349609375,
  0.070831298828125, 4.383544921875,
  0.067138671875, 4.458740234375,
  0.06365966796875, 4.533935546875,
  0.06036376953125, 4.609100341796875,
  0.057220458984375, 4.684295654296875,
  0.05426025390625, 4.759490966796875,
  0.05145263671875, 4.834686279296875,
  0.048797607421875, 4.909881591796875,
  0.046295166015625, 4.985076904296875,
  0.043914794921875, 5.06024169921875,
  0.0416259765625, 5.13543701171875,
  0.03948974609375, 5.21063232421875,
  0.0374755859375, 5.285858154296875,
  0.035552978515625, 5.360992431640625,
  0.033721923828125, 5.43621826171875,
  0.031982421875, 5.51141357421875,
  0.03033447265625, 5.586578369140625,
  0.028778076171875, 5.661773681640625,
  0.027313232421875, 5.736968994140625,
};


// This struct is for passing data to kvz_rdoq_sign_hiding
struct sh_rates_t {
  // Bit cost of increasing rate by one.
  int32_t inc[32 * 32];
  // Bit cost of decreasing rate by one.
  int32_t dec[32 * 32];
  // Bit cost of going from zero to one.
  int32_t sig_coeff_inc[32 * 32];
  // Coeff minus quantized coeff.
  int32_t quant_delta[32 * 32];
};


/**
 * \brief Calculate actual (or really close to actual) bitcost for coding
 * coefficients.
 *
 * \param coeff coefficient array
 * \param width coeff block width
 * \param type data type (0 == luma)
 *
 * \returns bits needed to code input coefficients
 */
static INLINE uint32_t get_coeff_cabac_cost(
    const encoder_state_t * const state,
    const coeff_t *coeff,
    int32_t width,
    int32_t type,
    int8_t scan_mode)
{
  // Make sure there are coeffs present
  bool found = false;
  for (int i = 0; i < width*width; i++) {
    if (coeff[i] != 0) {
      found = 1;
      break;
    }
  }
  if (!found) return 0;

  // Take a copy of the CABAC so that we don't overwrite the contexts when
  // counting the bits.
  cabac_data_t cabac_copy;
  memcpy(&cabac_copy, &state->cabac, sizeof(cabac_copy));

  // Clear bytes and bits and set mode to "count"
  cabac_copy.only_count = 1;
  cabac_copy.num_buffered_bytes = 0;
  cabac_copy.bits_left = 23;

  // Execute the coding function.
  // It is safe to drop the const modifier since state won't be modified
  // when cabac.only_count is set.
  kvz_encode_coeff_nxn((encoder_state_t*) state,
                       &cabac_copy,
                       coeff,
                       width,
                       type,
                       scan_mode,
                       0);

  return (23 - cabac_copy.bits_left) + (cabac_copy.num_buffered_bytes << 3);
}

/**
 * \brief Estimate bitcost for coding coefficients.
 *
 * \param coeff   coefficient array
 * \param width   coeff block width
 * \param type    data type (0 == luma)
 *
 * \returns       number of bits needed to code coefficients
 */
uint32_t kvz_get_coeff_cost(const encoder_state_t * const state,
                            const coeff_t *coeff,
                            int32_t width,
                            int32_t type,
                            int8_t scan_mode)
{
  if (state->qp >= state->encoder_control->cfg.fast_residual_cost_limit) {
    return get_coeff_cabac_cost(state, coeff, width, type, scan_mode);

  } else {
    // Estimate coeff coding cost based on QP and sum of absolute coeffs.
    // const uint32_t sum = kvz_coeff_abs_sum(coeff, width * width);
    // return (uint32_t)(sum * (state->qp * COEFF_COST_QP_FACTOR + COEFF_COST_BIAS) + 0.5);
    return kvz_fast_coeff_cost(coeff, width, state->qp);
  }
}

#define COEF_REMAIN_BIN_REDUCTION 3
/** Calculates the cost for specific absolute transform level
 * \param abs_level scaled quantized level
 * \param ctx_num_one current ctxInc for coeff_abs_level_greater1 (1st bin of coeff_abs_level_minus1 in AVC)
 * \param ctx_num_abs current ctxInc for coeff_abs_level_greater2 (remaining bins of coeff_abs_level_minus1 in AVC)
 * \param abs_go_rice Rice parameter for coeff_abs_level_minus3
 * \returns cost of given absolute transform level
 * From HM 12.0
 */
INLINE int32_t kvz_get_ic_rate(encoder_state_t * const state,
                    uint32_t abs_level,
                    uint16_t ctx_num_one,
                    uint16_t ctx_num_abs,
                    uint16_t abs_go_rice,
                    uint32_t c1_idx,
                    uint32_t c2_idx,
                    int8_t type)
{
  cabac_data_t * const cabac = &state->cabac;
  int32_t rate = 1 << CTX_FRAC_BITS;
  uint32_t base_level  =  (c1_idx < C1FLAG_NUMBER)? (2 + (c2_idx < C2FLAG_NUMBER)) : 1;
  cabac_ctx_t *base_one_ctx = (type == 0) ? &(cabac->ctx.cu_one_model_luma[0]) : &(cabac->ctx.cu_one_model_chroma[0]);
  cabac_ctx_t *base_abs_ctx = (type == 0) ? &(cabac->ctx.cu_abs_model_luma[0]) : &(cabac->ctx.cu_abs_model_chroma[0]);

  if ( abs_level >= base_level ) {
    int32_t symbol     = abs_level - base_level;
    int32_t length;
    if (symbol < (COEF_REMAIN_BIN_REDUCTION << abs_go_rice)) {
      length = symbol>>abs_go_rice;
      rate += (length+1+abs_go_rice) * (1 << CTX_FRAC_BITS);
    } else {
      length = abs_go_rice;
      symbol  = symbol - ( COEF_REMAIN_BIN_REDUCTION << abs_go_rice);
      while (symbol >= (1<<length)) {
        symbol -=  (1<<(length++));
      }
      rate += (COEF_REMAIN_BIN_REDUCTION+length+1-abs_go_rice+length) * (1 << CTX_FRAC_BITS);
    }
    if (c1_idx < C1FLAG_NUMBER) {
      rate += CTX_ENTROPY_BITS(&base_one_ctx[ctx_num_one],1);

      if (c2_idx < C2FLAG_NUMBER) {
        rate += CTX_ENTROPY_BITS(&base_abs_ctx[ctx_num_abs],1);
      }
    }
  }
  else if( abs_level == 1 ) {
    rate += CTX_ENTROPY_BITS(&base_one_ctx[ctx_num_one],0);
  } else if( abs_level == 2 ) {
    rate += CTX_ENTROPY_BITS(&base_one_ctx[ctx_num_one],1);
    rate += CTX_ENTROPY_BITS(&base_abs_ctx[ctx_num_abs],0);
  }

  return rate;
}

/** Get the best level in RD sense
 * \param coded_cost reference to coded cost
 * \param coded_cost0 reference to cost when coefficient is 0
 * \param coded_cost_sig reference to cost of significant coefficient
 * \param level_double reference to unscaled quantized level
 * \param max_abs_level scaled quantized level
 * \param ctx_num_sig current ctxInc for coeff_abs_significant_flag
 * \param ctx_num_one current ctxInc for coeff_abs_level_greater1 (1st bin of coeff_abs_level_minus1 in AVC)
 * \param ctx_num_abs current ctxInc for coeff_abs_level_greater2 (remaining bins of coeff_abs_level_minus1 in AVC)
 * \param abs_go_rice current Rice parameter for coeff_abs_level_minus3
 * \param q_bits quantization step size
 * \param temp correction factor
 * \param last indicates if the coefficient is the last significant
 * \returns best quantized transform level for given scan position
 * This method calculates the best quantized transform level for a given scan position.
 * From HM 12.0
 */
INLINE uint32_t kvz_get_coded_level ( encoder_state_t * const state, double *coded_cost, double *coded_cost0, double *coded_cost_sig,
                           int32_t level_double, uint32_t max_abs_level,
                           uint16_t ctx_num_sig, uint16_t ctx_num_one, uint16_t ctx_num_abs,
                           uint16_t abs_go_rice,
                           uint32_t c1_idx, uint32_t c2_idx,
                           int32_t q_bits,double temp, int8_t last, int8_t type)
{
  cabac_data_t * const cabac = &state->cabac;
  double cur_cost_sig   = 0;
  uint32_t best_abs_level = 0;
  int32_t abs_level;
  int32_t min_abs_level;
  cabac_ctx_t* base_sig_model = type?(cabac->ctx.cu_sig_model_chroma):(cabac->ctx.cu_sig_model_luma);

  if( !last && max_abs_level < 3 ) {
    *coded_cost_sig = state->lambda * CTX_ENTROPY_BITS(&base_sig_model[ctx_num_sig], 0);
    *coded_cost     = *coded_cost0 + *coded_cost_sig;
    if (max_abs_level == 0) return best_abs_level;
  } else {
    *coded_cost = MAX_DOUBLE;
  }

  if( !last ) {
    cur_cost_sig = state->lambda * CTX_ENTROPY_BITS(&base_sig_model[ctx_num_sig], 1);
  }

  min_abs_level    = ( max_abs_level > 1 ? max_abs_level - 1 : 1 );
  for (abs_level = max_abs_level; abs_level >= min_abs_level ; abs_level-- ) {
    double err       = (double)(level_double - ( abs_level * (1 << q_bits) ) );
    double cur_cost  = err * err * temp + state->lambda *
                       kvz_get_ic_rate( state, abs_level, ctx_num_one, ctx_num_abs,
                                    abs_go_rice, c1_idx, c2_idx, type);
    cur_cost        += cur_cost_sig;

    if( cur_cost < *coded_cost ) {
      best_abs_level  = abs_level;
      *coded_cost     = cur_cost;
      *coded_cost_sig = cur_cost_sig;
    }
  }

  return best_abs_level;
}


/** Calculates the cost of signaling the last significant coefficient in the block
 * \param pos_x X coordinate of the last significant coefficient
 * \param pos_y Y coordinate of the last significant coefficient
 * \returns cost of last significant coefficient
 * \param uiWidth width of the transform unit (TU)
 *
 * From HM 12.0
*/
static double get_rate_last(const encoder_state_t * const state,
                            const uint32_t  pos_x, const uint32_t pos_y,
                            int32_t* last_x_bits, int32_t* last_y_bits)
{
  uint32_t ctx_x   = g_group_idx[pos_x];
  uint32_t ctx_y   = g_group_idx[pos_y];
  double uiCost = last_x_bits[ ctx_x ] + last_y_bits[ ctx_y ];
  if( ctx_x > 3 ) {
    uiCost += CTX_FRAC_ONE_BIT * ((ctx_x - 2) >> 1);
  }
  if( ctx_y > 3 ) {
    uiCost += CTX_FRAC_ONE_BIT * ((ctx_y - 2) >> 1);
  }
  return state->lambda * uiCost;
}

static void calc_last_bits(encoder_state_t * const state, int32_t width, int32_t height, int8_t type,
                           int32_t* last_x_bits, int32_t* last_y_bits)
{
  cabac_data_t * const cabac = &state->cabac;
  int32_t bits_x = 0, bits_y = 0;
  int32_t blk_size_offset_x, blk_size_offset_y, shiftX, shiftY;
  int32_t ctx;

  cabac_ctx_t *base_ctx_x = (type ? cabac->ctx.cu_ctx_last_x_chroma : cabac->ctx.cu_ctx_last_x_luma);
  cabac_ctx_t *base_ctx_y = (type ? cabac->ctx.cu_ctx_last_y_chroma : cabac->ctx.cu_ctx_last_y_luma);

  blk_size_offset_x = type ? 0: (kvz_g_convert_to_bit[ width ] *3 + ((kvz_g_convert_to_bit[ width ] +1)>>2));
  blk_size_offset_y = type ? 0: (kvz_g_convert_to_bit[ height ]*3 + ((kvz_g_convert_to_bit[ height ]+1)>>2));
  shiftX = type ? kvz_g_convert_to_bit[ width  ] :((kvz_g_convert_to_bit[ width  ]+3)>>2);
  shiftY = type ? kvz_g_convert_to_bit[ height ] :((kvz_g_convert_to_bit[ height ]+3)>>2);


  for (ctx = 0; ctx < g_group_idx[ width - 1 ]; ctx++) {
    int32_t ctx_offset = blk_size_offset_x + (ctx >>shiftX);
    last_x_bits[ ctx ] = bits_x + CTX_ENTROPY_BITS(&base_ctx_x[ ctx_offset ],0);
    bits_x += CTX_ENTROPY_BITS(&base_ctx_x[ ctx_offset ],1);
  }
  last_x_bits[ctx] = bits_x;
  for (ctx = 0; ctx < g_group_idx[ height - 1 ]; ctx++) {
    int32_t ctx_offset = blk_size_offset_y + (ctx >>shiftY);
    last_y_bits[ ctx ] = bits_y + CTX_ENTROPY_BITS(&base_ctx_y[ ctx_offset ],0);
    bits_y +=  CTX_ENTROPY_BITS(&base_ctx_y[ ctx_offset ],1);
  }
  last_y_bits[ctx] = bits_y;
}

/**
 * \brief Select which coefficient to change for sign hiding, and change it.
 *
 * When sign hiding is enabled, the last sign bit of the last coefficient is
 * calculated from the parity of the other coefficients. If the parity is not
 * correct, one coefficient has to be changed by one. This function uses
 * tables generated during RDOQ to select the best coefficient to change.
 */
void kvz_rdoq_sign_hiding(
    const encoder_state_t *const state,
    const int32_t qp_scaled,
    const uint32_t *const scan2raster,
    const struct sh_rates_t *const sh_rates,
    const int32_t last_pos,
    const coeff_t *const coeffs,
    coeff_t *const quant_coeffs)
{
  const encoder_control_t * const ctrl = state->encoder_control;

  int inv_quant = kvz_g_inv_quant_scales[qp_scaled % 6];
  // This somehow scales quant_delta into fractional bits. Instead of the bits
  // being multiplied by lambda, the residual is divided by it, or something
  // like that.
  const int64_t rd_factor = (inv_quant * inv_quant * (1 << (2 * (qp_scaled / 6)))
                      / state->lambda / 16 / (1 << (2 * (ctrl->bitdepth - 8))) + 0.5);
  const int last_cg = (last_pos - 1) >> LOG2_SCAN_SET_SIZE;

  for (int32_t cg_scan = last_cg; cg_scan >= 0; cg_scan--) {
    const int32_t cg_coeff_scan = cg_scan << LOG2_SCAN_SET_SIZE;
    
    // Find positions of first and last non-zero coefficients in the CG.
    int32_t last_nz_scan = -1;
    for (int32_t coeff_i = SCAN_SET_SIZE - 1; coeff_i >= 0; --coeff_i) {
      if (quant_coeffs[scan2raster[coeff_i + cg_coeff_scan]]) {
        last_nz_scan = coeff_i;
        break;
      }
    }
    int32_t first_nz_scan = SCAN_SET_SIZE;
    for (int32_t coeff_i = 0; coeff_i <= last_nz_scan; coeff_i++) {
      if (quant_coeffs[scan2raster[coeff_i + cg_coeff_scan]]) {
        first_nz_scan = coeff_i;
        break;
      }
    }

    if (last_nz_scan - first_nz_scan < SBH_THRESHOLD) {
      continue;
    }

    const int32_t signbit = quant_coeffs[scan2raster[cg_coeff_scan + first_nz_scan]] <= 0;
    unsigned abs_coeff_sum = 0;
    for (int32_t coeff_scan = first_nz_scan; coeff_scan <= last_nz_scan; coeff_scan++) {
      abs_coeff_sum += quant_coeffs[scan2raster[coeff_scan + cg_coeff_scan]];
    }
    if (signbit == (abs_coeff_sum & 0x1)) {
      // Sign already matches with the parity, no need to modify coefficients.
      continue;
    }

    // Otherwise, search for the best coeff to change by one and change it.

    struct {
      int64_t cost;
      int pos;
      int change;
    } current, best = { MAX_INT64, 0, 0 };

    const int last_coeff_scan = (cg_scan == last_cg ? last_nz_scan : SCAN_SET_SIZE - 1);
    for (int coeff_scan = last_coeff_scan; coeff_scan >= 0; --coeff_scan) {
      current.pos = scan2raster[coeff_scan + cg_coeff_scan];
      // Shift the calculation back into original precision to avoid
      // changing the bitstream.
#     define PRECISION_INC (15 - CTX_FRAC_BITS)
      int64_t quant_cost_in_bits = rd_factor * sh_rates->quant_delta[current.pos];

      coeff_t abs_coeff = abs(quant_coeffs[current.pos]);

      if (abs_coeff != 0) {
        // Choose between incrementing and decrementing a non-zero coeff.

        int64_t inc_bits = sh_rates->inc[current.pos];
        int64_t dec_bits = sh_rates->dec[current.pos];
        if (abs_coeff == 1) {
          // We save sign bit and sig_coeff goes to zero.
          dec_bits -= CTX_FRAC_ONE_BIT + sh_rates->sig_coeff_inc[current.pos];
        }
        if (cg_scan == last_cg && last_nz_scan == coeff_scan && abs_coeff == 1) {
          // Changing the last non-zero bit in the last cg to zero.
          // This might save a lot of bits if the next bits are already
          // zeros, or just a coupple fractional bits if they are not.
          // TODO: Check if calculating the real savings makes sense.
          dec_bits -= 4 * CTX_FRAC_ONE_BIT;
        }

        inc_bits = -quant_cost_in_bits + inc_bits * (1 << PRECISION_INC);
        dec_bits = quant_cost_in_bits + dec_bits * (1 << PRECISION_INC);

        if (inc_bits < dec_bits) {
          current.change = 1;
          current.cost = inc_bits;
        } else {
          current.change = -1;
          current.cost = dec_bits;

          if (coeff_scan == first_nz_scan && abs_coeff == 1) {
            // Don't turn first non-zero coeff into zero.
            // Seems kind of arbitrary. It's probably because it could lead to
            // breaking SBH_THRESHOLD.
            current.cost = MAX_INT64;
          }
        }
      } else {
        // Try incrementing a zero coeff.

        // Add sign bit, other bits and sig_coeff goes to one.
        int bits = CTX_FRAC_ONE_BIT + sh_rates->inc[current.pos] + sh_rates->sig_coeff_inc[current.pos];
        current.cost = -llabs(quant_cost_in_bits) + bits * (1 << PRECISION_INC);
        current.change = 1;

        if (coeff_scan < first_nz_scan) {
          if (((coeffs[current.pos] >= 0) ? 0 : 1) != signbit) {
            current.cost = MAX_INT64;
          }
        }
      }

      if (current.cost < best.cost) {
        best = current;
      }
    }

    if (quant_coeffs[best.pos] == 32767 || quant_coeffs[best.pos] == -32768) {
      best.change = -1;
    }

    if (coeffs[best.pos] >= 0) {
      quant_coeffs[best.pos] += best.change;
    } else {
      quant_coeffs[best.pos] -= best.change;
    }
  }
}


/** RDOQ with CABAC
 * \returns void
 * Rate distortion optimized quantization for entropy
 * coding engines using probability models like CABAC
 * From HM 12.0
 */
void kvz_rdoq(encoder_state_t * const state, coeff_t *coef, coeff_t *dest_coeff, int32_t width,
           int32_t height, int8_t type, int8_t scan_mode, int8_t block_type, int8_t tr_depth)
{
  const encoder_control_t * const encoder = state->encoder_control;
  cabac_data_t * const cabac = &state->cabac;
  uint32_t log2_tr_size      = kvz_g_convert_to_bit[ width ] + 2;
  int32_t  transform_shift   = MAX_TR_DYNAMIC_RANGE - encoder->bitdepth - log2_tr_size;  // Represents scaling through forward transform
  uint16_t go_rice_param     = 0;
  uint32_t log2_block_size   = kvz_g_convert_to_bit[ width ] + 2;
  int32_t  scalinglist_type= (block_type == CU_INTRA ? 0 : 3) + (int8_t)("\0\3\1\2"[type]);

  int32_t qp_scaled = kvz_get_scaled_qp(type, state->qp, (encoder->bitdepth - 8) * 6);
  
  int32_t q_bits = QUANT_SHIFT + qp_scaled/6 + transform_shift;

  const int32_t *quant_coeff  = encoder->scaling_list.quant_coeff[log2_tr_size-2][scalinglist_type][qp_scaled%6];
  const double *err_scale     = encoder->scaling_list.error_scale[log2_tr_size-2][scalinglist_type][qp_scaled%6];

  double block_uncoded_cost = 0;
  
  double cost_coeff [ 32 * 32 ];
  double cost_sig   [ 32 * 32 ];
  double cost_coeff0[ 32 * 32 ];

  struct sh_rates_t sh_rates;

  const uint32_t *scan_cg = g_sig_last_scan_cg[log2_block_size - 2][scan_mode];
  const uint32_t cg_size = 16;
  const int32_t  shift = 4 >> 1;
  const uint32_t num_blk_side = width >> shift;
  double   cost_coeffgroup_sig[ 64 ];
  uint32_t sig_coeffgroup_flag[ 64 ];

  uint16_t    ctx_set    = 0;
  int16_t     c1         = 1;
  int16_t     c2         = 0;
  double      base_cost  = 0;

  uint32_t    c1_idx     = 0;
  uint32_t    c2_idx     = 0;
  int32_t     base_level;

  const uint32_t *scan = kvz_g_sig_last_scan[ scan_mode ][ log2_block_size - 1 ];

  int32_t cg_last_scanpos = -1;
  int32_t last_scanpos = -1;

  uint32_t cg_num = width * height >> 4;

  // Explicitly tell the only possible numbers of elements to be zeroed.
  // Hope the compiler is able to utilize this information.
  switch (cg_num) {
    case  1: FILL_ARRAY(sig_coeffgroup_flag, 0,  1); break;
    case  4: FILL_ARRAY(sig_coeffgroup_flag, 0,  4); break;
    case 16: FILL_ARRAY(sig_coeffgroup_flag, 0, 16); break;
    case 64: FILL_ARRAY(sig_coeffgroup_flag, 0, 64); break;
    default: assert(0 && "There should be 1, 4, 16 or 64 coefficient groups");
  }

  cabac_ctx_t *base_coeff_group_ctx = &(cabac->ctx.cu_sig_coeff_group_model[type]);
  cabac_ctx_t *baseCtx              = (type == 0) ? &(cabac->ctx.cu_sig_model_luma[0]) : &(cabac->ctx.cu_sig_model_chroma[0]);
  cabac_ctx_t *base_one_ctx = (type == 0) ? &(cabac->ctx.cu_one_model_luma[0]) : &(cabac->ctx.cu_one_model_chroma[0]);

  struct {
    double coded_level_and_dist;
    double uncoded_dist;
    double sig_cost;
    double sig_cost_0;
    int32_t nnz_before_pos0;
  } rd_stats;

  //Find last cg and last scanpos
  int32_t cg_scanpos;
  for (cg_scanpos = (cg_num - 1); cg_scanpos >= 0; cg_scanpos--)
  {
    for (int32_t scanpos_in_cg = (cg_size - 1); scanpos_in_cg >= 0; scanpos_in_cg--)
    {
      int32_t  scanpos        = cg_scanpos*cg_size + scanpos_in_cg;
      uint32_t blkpos         = scan[scanpos];
      int32_t q               = quant_coeff[blkpos];
      int32_t level_double    = coef[blkpos];
      level_double            = MIN(abs(level_double) * q, MAX_INT - (1 << (q_bits - 1)));
      uint32_t max_abs_level  = (level_double + (1 << (q_bits - 1))) >> q_bits;

      if (max_abs_level > 0) {
        last_scanpos    = scanpos;
        ctx_set         = (scanpos > 0 && type == 0) ? 2 : 0;
        cg_last_scanpos = cg_scanpos;
        sh_rates.sig_coeff_inc[blkpos] = 0;
        break;
      }
      dest_coeff[blkpos] = 0;
    }
    if (last_scanpos != -1) break;
  }

  if (last_scanpos == -1) {
    return;
  }

  for (; cg_scanpos >= 0; cg_scanpos--) cost_coeffgroup_sig[cg_scanpos] = 0;

  int32_t last_x_bits[32], last_y_bits[32];
  calc_last_bits(state, width, height, type, last_x_bits, last_y_bits);

  for (int32_t cg_scanpos = cg_last_scanpos; cg_scanpos >= 0; cg_scanpos--) {
    uint32_t cg_blkpos  = scan_cg[cg_scanpos];
    uint32_t cg_pos_y   = cg_blkpos / num_blk_side;
    uint32_t cg_pos_x   = cg_blkpos - (cg_pos_y * num_blk_side);

    int32_t pattern_sig_ctx = kvz_context_calc_pattern_sig_ctx(sig_coeffgroup_flag,
                                                           cg_pos_x, cg_pos_y, width);

    FILL(rd_stats, 0);
    for (int32_t scanpos_in_cg = cg_size - 1; scanpos_in_cg >= 0; scanpos_in_cg--)  {
      int32_t  scanpos = cg_scanpos*cg_size + scanpos_in_cg;
      if (scanpos > last_scanpos) continue;
      uint32_t blkpos         = scan[scanpos];
      int32_t q               = quant_coeff[blkpos];
      double temp             = err_scale[blkpos];
      int32_t level_double    = coef[blkpos];
      level_double            = MIN(abs(level_double) * q , MAX_INT - (1 << (q_bits - 1)));
      uint32_t max_abs_level  = (level_double + (1 << (q_bits - 1))) >> q_bits;

      double err              = (double)level_double;
      cost_coeff0[scanpos]    = err * err * temp; 
      block_uncoded_cost      += cost_coeff0[ scanpos ];
      //===== coefficient level estimation =====
      int32_t  level;
      uint16_t  one_ctx = 4 * ctx_set + c1;
      uint16_t  abs_ctx = ctx_set + c2;

      if( scanpos == last_scanpos ) {
        level            = kvz_get_coded_level(state, &cost_coeff[ scanpos ], &cost_coeff0[ scanpos ], &cost_sig[ scanpos ],
                                             level_double, max_abs_level, 0, one_ctx, abs_ctx, go_rice_param,
                                             c1_idx, c2_idx, q_bits, temp, 1, type );
      } else {
        uint32_t  pos_y    = blkpos >> log2_block_size;
        uint32_t  pos_x    = blkpos - ( pos_y << log2_block_size );
        uint16_t  ctx_sig  = (uint16_t)kvz_context_get_sig_ctx_inc(pattern_sig_ctx, scan_mode, pos_x, pos_y,
                                                     log2_block_size, type);
        level              = kvz_get_coded_level(state, &cost_coeff[ scanpos ], &cost_coeff0[ scanpos ], &cost_sig[ scanpos ],
                                             level_double, max_abs_level, ctx_sig, one_ctx, abs_ctx, go_rice_param,
                                             c1_idx, c2_idx, q_bits, temp, 0, type );
        if (encoder->cfg.signhide_enable) {
          int greater_than_zero = CTX_ENTROPY_BITS(&baseCtx[ctx_sig], 1);
          int zero = CTX_ENTROPY_BITS(&baseCtx[ctx_sig], 0);
          sh_rates.sig_coeff_inc[blkpos] = greater_than_zero - zero;
        }
      }

      if (encoder->cfg.signhide_enable) {
        sh_rates.quant_delta[blkpos] = (level_double - level * (1 << q_bits)) >> (q_bits - 8);
        if (level > 0) {
          int32_t rate_now  = kvz_get_ic_rate(state, level, one_ctx, abs_ctx, go_rice_param, c1_idx, c2_idx, type);
          int32_t rate_up   = kvz_get_ic_rate(state, level + 1, one_ctx, abs_ctx, go_rice_param, c1_idx, c2_idx, type);
          int32_t rate_down = kvz_get_ic_rate(state, level - 1, one_ctx, abs_ctx, go_rice_param, c1_idx, c2_idx, type);
          sh_rates.inc[blkpos] = rate_up - rate_now;
          sh_rates.dec[blkpos] = rate_down - rate_now;
        } else { // level == 0
          sh_rates.inc[blkpos]   = CTX_ENTROPY_BITS(&base_one_ctx[one_ctx], 0);
        }
      }
      dest_coeff[blkpos] = (coeff_t)level;
      base_cost         += cost_coeff[scanpos];

      base_level = (c1_idx < C1FLAG_NUMBER) ? (2 + (c2_idx < C2FLAG_NUMBER)) : 1;
      if (level >= base_level) {
        if(level  > 3*(1<<go_rice_param)) {
          go_rice_param = MIN(go_rice_param + 1, 4);
        }
      }
      if (level >= 1) c1_idx ++;

      //===== update bin model =====
      if (level > 1) {
        c1 = 0;
        c2 += (c2 < 2);
        c2_idx ++;
      } else if( (c1 < 3) && (c1 > 0) && level) {
        c1++;
      }

      //===== context set update =====
      if ((scanpos % SCAN_SET_SIZE == 0) && scanpos > 0) {
        c2                = 0;
        go_rice_param     = 0;

        c1_idx   = 0;
        c2_idx   = 0;
        ctx_set = (scanpos == SCAN_SET_SIZE || type != 0) ? 0 : 2;
        if( c1 == 0 ) {
          ctx_set++;
        }
        c1 = 1;
      }

      rd_stats.sig_cost += cost_sig[scanpos];
      if ( scanpos_in_cg == 0 ) {
        rd_stats.sig_cost_0 = cost_sig[scanpos];
      }
      if ( dest_coeff[blkpos] )  {
        sig_coeffgroup_flag[cg_blkpos] = 1;
        rd_stats.coded_level_and_dist   += cost_coeff[scanpos] - cost_sig[scanpos];
        rd_stats.uncoded_dist           += cost_coeff0[scanpos];
        if ( scanpos_in_cg != 0 ) {
          rd_stats.nnz_before_pos0++;
        }
      }
    } //end for (scanpos_in_cg)

    if( cg_scanpos ) {
      if (sig_coeffgroup_flag[cg_blkpos] == 0) {
        uint32_t ctx_sig  = kvz_context_get_sig_coeff_group(sig_coeffgroup_flag, cg_pos_x,
                                                        cg_pos_y, width);
        cost_coeffgroup_sig[cg_scanpos] = state->lambda *CTX_ENTROPY_BITS(&base_coeff_group_ctx[ctx_sig],0);
        base_cost += cost_coeffgroup_sig[cg_scanpos]  - rd_stats.sig_cost;
      } else {
        if (cg_scanpos < cg_last_scanpos){
          double cost_zero_cg;
          uint32_t ctx_sig;
          if (rd_stats.nnz_before_pos0 == 0) {
            base_cost -= rd_stats.sig_cost_0;
            rd_stats.sig_cost -= rd_stats.sig_cost_0;
          }
          // rd-cost if SigCoeffGroupFlag = 0, initialization
          cost_zero_cg = base_cost;

          // add SigCoeffGroupFlag cost to total cost
          ctx_sig = kvz_context_get_sig_coeff_group(sig_coeffgroup_flag, cg_pos_x,
            cg_pos_y, width);

          cost_coeffgroup_sig[cg_scanpos] = state->lambda * CTX_ENTROPY_BITS(&base_coeff_group_ctx[ctx_sig], 1);
          base_cost += cost_coeffgroup_sig[cg_scanpos];
          cost_zero_cg += state->lambda * CTX_ENTROPY_BITS(&base_coeff_group_ctx[ctx_sig], 0);

          // try to convert the current coeff group from non-zero to all-zero
          cost_zero_cg += rd_stats.uncoded_dist;          // distortion for resetting non-zero levels to zero levels
          cost_zero_cg -= rd_stats.coded_level_and_dist;  // distortion and level cost for keeping all non-zero levels
          cost_zero_cg -= rd_stats.sig_cost;              // sig cost for all coeffs, including zero levels and non-zerl levels

          // if we can save cost, change this block to all-zero block
          if (cost_zero_cg < base_cost) {

            sig_coeffgroup_flag[cg_blkpos] = 0;
            base_cost = cost_zero_cg;

            cost_coeffgroup_sig[cg_scanpos] = state->lambda * CTX_ENTROPY_BITS(&base_coeff_group_ctx[ctx_sig], 0);

            // reset coeffs to 0 in this block
            for (int32_t scanpos_in_cg = cg_size - 1; scanpos_in_cg >= 0; scanpos_in_cg--) {
              int32_t  scanpos = cg_scanpos*cg_size + scanpos_in_cg;
              uint32_t blkpos = scan[scanpos];
              if (dest_coeff[blkpos]){
                dest_coeff[blkpos] = 0;
                cost_coeff[scanpos] = cost_coeff0[scanpos];
                cost_sig[scanpos] = 0;
              }
            }
          } // end if ( cost_all_zeros < base_cost )
        }
      } // end if if (sig_coeffgroup_flag[ cg_blkpos ] == 0)
    } else {
      sig_coeffgroup_flag[cg_blkpos] = 1;
    }
  } //end for (cg_scanpos)

  //===== estimate last position =====
  double  best_cost        = 0;
  int32_t ctx_cbf          = 0;
  int8_t found_last        = 0;
  int32_t best_last_idx_p1 = 0;

  if( block_type != CU_INTRA && !type/* && pcCU->getTransformIdx( uiAbsPartIdx ) == 0*/ ) {
    best_cost  = block_uncoded_cost +   state->lambda * CTX_ENTROPY_BITS(&(cabac->ctx.cu_qt_root_cbf_model),0);
    base_cost +=   state->lambda * CTX_ENTROPY_BITS(&(cabac->ctx.cu_qt_root_cbf_model),1);
  } else {
    cabac_ctx_t* base_cbf_model = type?(cabac->ctx.qt_cbf_model_chroma):(cabac->ctx.qt_cbf_model_luma);
    ctx_cbf    = ( type ? tr_depth : !tr_depth);
    best_cost  = block_uncoded_cost +  state->lambda * CTX_ENTROPY_BITS(&base_cbf_model[ctx_cbf],0);
    base_cost +=   state->lambda * CTX_ENTROPY_BITS(&base_cbf_model[ctx_cbf],1);
  }

  for ( int32_t cg_scanpos = cg_last_scanpos; cg_scanpos >= 0; cg_scanpos--) {
    uint32_t cg_blkpos = scan_cg[cg_scanpos];
    base_cost -= cost_coeffgroup_sig[cg_scanpos];

    if (sig_coeffgroup_flag[ cg_blkpos ]) {
      for ( int32_t scanpos_in_cg = cg_size - 1; scanpos_in_cg >= 0; scanpos_in_cg--) {
        int32_t  scanpos = cg_scanpos*cg_size + scanpos_in_cg;
        if (scanpos > last_scanpos) continue;
        uint32_t blkpos  = scan[scanpos];

        if( dest_coeff[ blkpos ] ) {
          uint32_t   pos_y = blkpos >> log2_block_size;
          uint32_t   pos_x = blkpos - ( pos_y << log2_block_size );

          double cost_last = (scan_mode == SCAN_VER) ? get_rate_last(state, pos_y, pos_x,last_x_bits,last_y_bits) : get_rate_last(state, pos_x, pos_y, last_x_bits,last_y_bits );
          double totalCost = base_cost + cost_last - cost_sig[ scanpos ];

          if( totalCost < best_cost ) {
            best_last_idx_p1 = scanpos + 1;
            best_cost        = totalCost;
          }
          if( dest_coeff[ blkpos ] > 1 ) {
            found_last = 1;
            break;
          }
          base_cost -= cost_coeff[scanpos];
          base_cost += cost_coeff0[scanpos];
        } else {
          base_cost -= cost_sig[scanpos];
        }
      } //end for
      if (found_last) break;
    } // end if (sig_coeffgroup_flag[ cg_blkpos ])
  } // end for

  uint32_t abs_sum = 0;
  for ( int32_t scanpos = 0; scanpos < best_last_idx_p1; scanpos++) {
    int32_t blkPos     = scan[scanpos];
    int32_t level      = dest_coeff[blkPos];
    abs_sum            += level;
    dest_coeff[blkPos] = (coeff_t)(( coef[blkPos] < 0 ) ? -level : level);
  }
  //===== clean uncoded coefficients =====
  for ( int32_t scanpos = best_last_idx_p1; scanpos <= last_scanpos; scanpos++) {
    dest_coeff[scan[scanpos]] = 0;
  }

  if (encoder->cfg.signhide_enable && abs_sum >= 2) {
    kvz_rdoq_sign_hiding(state, qp_scaled, scan, &sh_rates, best_last_idx_p1, coef, dest_coeff);
  }
}

/**
 * Calculate cost of actual motion vectors using CABAC coding
 */
uint32_t kvz_get_mvd_coding_cost_cabac(const encoder_state_t *state,
                                       const cabac_data_t* cabac,
                                       const int32_t mvd_hor,
                                       const int32_t mvd_ver)
{
  cabac_data_t cabac_copy = *cabac;
  cabac_copy.only_count = 1;

  // It is safe to drop const here because cabac->only_count is set.
  kvz_encode_mvd((encoder_state_t*) state, &cabac_copy, mvd_hor, mvd_ver);

  uint32_t bitcost =
    ((23 - cabac_copy.bits_left) + (cabac_copy.num_buffered_bytes << 3)) -
    ((23 - cabac->bits_left)     + (cabac->num_buffered_bytes << 3));

  return bitcost;
}

/** MVD cost calculation with CABAC
* \returns int
* Calculates Motion Vector cost and related costs using CABAC coding
*/
uint32_t kvz_calc_mvd_cost_cabac(const encoder_state_t * state,
                                 int x,
                                 int y,
                                 int mv_shift,
                                 int16_t mv_cand[2][2],
                                 inter_merge_cand_t merge_cand[MRG_MAX_NUM_CANDS],
                                 int16_t num_cand,
                                 int32_t ref_idx,
                                 uint32_t *bitcost)
{
  cabac_data_t state_cabac_copy;
  cabac_data_t* cabac;
  uint32_t merge_idx;
  vector2d_t mvd = { 0, 0 };
  int8_t merged = 0;
  int8_t cur_mv_cand = 0;

  x *= 1 << mv_shift;
  y *= 1 << mv_shift;

  // Check every candidate to find a match
  for (merge_idx = 0; merge_idx < (uint32_t)num_cand; merge_idx++) {
    if (merge_cand[merge_idx].dir == 3) continue;
    if (merge_cand[merge_idx].mv[merge_cand[merge_idx].dir - 1][0] == x &&
      merge_cand[merge_idx].mv[merge_cand[merge_idx].dir - 1][1] == y &&
      state->frame->ref_LX[merge_cand[merge_idx].dir - 1][
        merge_cand[merge_idx].ref[merge_cand[merge_idx].dir - 1]
      ] == ref_idx)
    {
      merged = 1;
      break;
    }
  }

  // Store cabac state and contexts
  memcpy(&state_cabac_copy, &state->cabac, sizeof(cabac_data_t));

  // Clear bytes and bits and set mode to "count"
  state_cabac_copy.only_count = 1;
  state_cabac_copy.num_buffered_bytes = 0;
  state_cabac_copy.bits_left = 23;

  cabac = &state_cabac_copy;

  if (!merged) {
    vector2d_t mvd1 = {
      x - mv_cand[0][0],
      y - mv_cand[0][1],
    };
    vector2d_t mvd2 = {
      x - mv_cand[1][0],
      y - mv_cand[1][1],
    };
    uint32_t cand1_cost = kvz_get_mvd_coding_cost_cabac(state, cabac, mvd1.x, mvd1.y);
    uint32_t cand2_cost = kvz_get_mvd_coding_cost_cabac(state, cabac, mvd2.x, mvd2.y);

    // Select candidate 1 if it has lower cost
    if (cand2_cost < cand1_cost) {
      cur_mv_cand = 1;
      mvd = mvd2;
    } else {
      mvd = mvd1;
    }
  }

  cabac->cur_ctx = &(cabac->ctx.cu_merge_flag_ext_model);

  CABAC_BIN(cabac, merged, "MergeFlag");
  num_cand = state->encoder_control->cfg.max_merge;
  if (merged) {
    if (num_cand > 1) {
      int32_t ui;
      for (ui = 0; ui < num_cand - 1; ui++) {
        int32_t symbol = (ui != merge_idx);
        if (ui == 0) {
          cabac->cur_ctx = &(cabac->ctx.cu_merge_idx_ext_model);
          CABAC_BIN(cabac, symbol, "MergeIndex");
        } else {
          CABAC_BIN_EP(cabac, symbol, "MergeIndex");
        }
        if (symbol == 0) break;
      }
    }
  } else {
    uint32_t ref_list_idx;
    uint32_t j;
    int ref_list[2] = { 0, 0 };
    for (j = 0; j < state->frame->ref->used_size; j++) {
      if (state->frame->ref->pocs[j] < state->frame->poc) {
        ref_list[0]++;
      } else {
        ref_list[1]++;
      }
    }

    //ToDo: bidir mv support
    for (ref_list_idx = 0; ref_list_idx < 2; ref_list_idx++) {
      if (/*cur_cu->inter.mv_dir*/ 1 & (1 << ref_list_idx)) {
        if (ref_list[ref_list_idx] > 1) {
          // parseRefFrmIdx
          int32_t ref_frame = ref_idx;

          cabac->cur_ctx = &(cabac->ctx.cu_ref_pic_model[0]);
          CABAC_BIN(cabac, (ref_frame != 0), "ref_idx_lX");

          if (ref_frame > 0) {
            int32_t i;
            int32_t ref_num = ref_list[ref_list_idx] - 2;

            cabac->cur_ctx = &(cabac->ctx.cu_ref_pic_model[1]);
            ref_frame--;

            for (i = 0; i < ref_num; ++i) {
              const uint32_t symbol = (i == ref_frame) ? 0 : 1;

              if (i == 0) {
                CABAC_BIN(cabac, symbol, "ref_idx_lX");
              } else {
                CABAC_BIN_EP(cabac, symbol, "ref_idx_lX");
              }
              if (symbol == 0) break;
            }
          }
        }

        // ToDo: Bidir vector support
        if (!(state->frame->ref_list == REF_PIC_LIST_1 && /*cur_cu->inter.mv_dir == 3*/ 0)) {
          // It is safe to drop const here because cabac->only_count is set.
          kvz_encode_mvd((encoder_state_t*) state, cabac, mvd.x, mvd.y);
        }

        // Signal which candidate MV to use
        kvz_cabac_write_unary_max_symbol(
            cabac,
            cabac->ctx.mvp_idx_model,
            cur_mv_cand,
            1,
            AMVP_MAX_NUM_CANDS - 1);
      }
    }
  }

  *bitcost = (23 - state_cabac_copy.bits_left) + (state_cabac_copy.num_buffered_bytes << 3);

  // Store bitcost before restoring cabac
  return *bitcost * (uint32_t)(state->lambda_sqrt + 0.5);
}
