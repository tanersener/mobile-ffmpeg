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

/*
* \file
*/

#include "strategies/avx2/quant-avx2.h"

#if COMPILE_INTEL_AVX2 && defined X86_64
#include <immintrin.h>
#include <stdlib.h>

#include "cu.h"
#include "encoder.h"
#include "encoderstate.h"
#include "kvazaar.h"
#include "rdo.h"
#include "scalinglist.h"
#include "strategies/generic/quant-generic.h"
#include "strategies/strategies-quant.h"
#include "strategyselector.h"
#include "tables.h"
#include "transform.h"


/**
 * \brief quantize transformed coefficents
 *
 */
void kvz_quant_flat_avx2(const encoder_state_t * const state, coeff_t *coef, coeff_t *q_coef, int32_t width,
  int32_t height, int8_t type, int8_t scan_idx, int8_t block_type)
{
  const encoder_control_t * const encoder = state->encoder_control;
  const uint32_t log2_block_size = kvz_g_convert_to_bit[width] + 2;
  const uint32_t * const scan = kvz_g_sig_last_scan[scan_idx][log2_block_size - 1];

  int32_t qp_scaled = kvz_get_scaled_qp(type, state->qp, (encoder->bitdepth - 8) * 6);
  const uint32_t log2_tr_size = kvz_g_convert_to_bit[width] + 2;
  const int32_t scalinglist_type = (block_type == CU_INTRA ? 0 : 3) + (int8_t)("\0\3\1\2"[type]);
  const int32_t *quant_coeff = encoder->scaling_list.quant_coeff[log2_tr_size - 2][scalinglist_type][qp_scaled % 6];
  const int32_t transform_shift = MAX_TR_DYNAMIC_RANGE - encoder->bitdepth - log2_tr_size; //!< Represents scaling through forward transform
  const int32_t q_bits = QUANT_SHIFT + qp_scaled / 6 + transform_shift;
  const int32_t add = ((state->frame->slicetype == KVZ_SLICE_I) ? 171 : 85) << (q_bits - 9);
  const int32_t q_bits8 = q_bits - 8;

  assert(quant_coeff[0] <= (1 << 15) - 1 && quant_coeff[0] >= -(1 << 15)); //Assuming flat values to fit int16_t

  uint32_t ac_sum = 0;

  __m256i v_ac_sum = _mm256_setzero_si256();
  __m256i v_quant_coeff = _mm256_set1_epi16(quant_coeff[0]);

  for (int32_t n = 0; n < width * height; n += 16) {

    __m256i v_level = _mm256_loadu_si256((__m256i*)&(coef[n]));
    __m256i v_sign = _mm256_cmpgt_epi16(_mm256_setzero_si256(), v_level);
    v_sign = _mm256_or_si256(v_sign, _mm256_set1_epi16(1));

    v_level = _mm256_abs_epi16(v_level);
    __m256i low_a = _mm256_unpacklo_epi16(v_level, _mm256_set1_epi16(0));
    __m256i high_a = _mm256_unpackhi_epi16(v_level, _mm256_set1_epi16(0));

    __m256i low_b = _mm256_unpacklo_epi16(v_quant_coeff, _mm256_set1_epi16(0));
    __m256i high_b = _mm256_unpackhi_epi16(v_quant_coeff, _mm256_set1_epi16(0));

    __m256i v_level32_a = _mm256_madd_epi16(low_a, low_b);
    __m256i v_level32_b = _mm256_madd_epi16(high_a, high_b);

    v_level32_a = _mm256_add_epi32(v_level32_a, _mm256_set1_epi32(add));
    v_level32_b = _mm256_add_epi32(v_level32_b, _mm256_set1_epi32(add));

    v_level32_a = _mm256_srai_epi32(v_level32_a, q_bits);
    v_level32_b = _mm256_srai_epi32(v_level32_b, q_bits);

    v_level = _mm256_packs_epi32(v_level32_a, v_level32_b);
    v_level = _mm256_sign_epi16(v_level, v_sign);

    _mm256_storeu_si256((__m256i*)&(q_coef[n]), v_level);

    v_ac_sum = _mm256_add_epi32(v_ac_sum, v_level32_a);
    v_ac_sum = _mm256_add_epi32(v_ac_sum, v_level32_b);
  }

  __m128i temp = _mm_add_epi32(_mm256_castsi256_si128(v_ac_sum), _mm256_extracti128_si256(v_ac_sum, 1));
  temp = _mm_add_epi32(temp, _mm_shuffle_epi32(temp, _MM_SHUFFLE(1, 0, 3, 2)));
  temp = _mm_add_epi32(temp, _mm_shuffle_epi32(temp, _MM_SHUFFLE(0, 1, 0, 1)));
  ac_sum += _mm_cvtsi128_si32(temp);

  if (!encoder->cfg.signhide_enable || ac_sum < 2) return;

  int32_t delta_u[LCU_WIDTH*LCU_WIDTH >> 2];

  for (int32_t n = 0; n < width * height; n += 16) {

    __m256i v_level = _mm256_loadu_si256((__m256i*)&(coef[n]));

    v_level = _mm256_abs_epi16(v_level);
    __m256i low_a = _mm256_unpacklo_epi16(v_level, _mm256_set1_epi16(0));
    __m256i high_a = _mm256_unpackhi_epi16(v_level, _mm256_set1_epi16(0));

    __m256i low_b = _mm256_unpacklo_epi16(v_quant_coeff, _mm256_set1_epi16(0));
    __m256i high_b = _mm256_unpackhi_epi16(v_quant_coeff, _mm256_set1_epi16(0));

    __m256i v_level32_a = _mm256_madd_epi16(low_a, low_b);
    __m256i v_level32_b = _mm256_madd_epi16(high_a, high_b);

    v_level32_a = _mm256_add_epi32(v_level32_a, _mm256_set1_epi32(add));
    v_level32_b = _mm256_add_epi32(v_level32_b, _mm256_set1_epi32(add));

    v_level32_a = _mm256_srai_epi32(v_level32_a, q_bits);
    v_level32_b = _mm256_srai_epi32(v_level32_b, q_bits);

    v_level = _mm256_packs_epi32(v_level32_a, v_level32_b);

    __m256i v_coef = _mm256_loadu_si256((__m256i*)&(coef[n]));
    __m256i v_coef_a = _mm256_unpacklo_epi16(_mm256_abs_epi16(v_coef), _mm256_set1_epi16(0));
    __m256i v_coef_b = _mm256_unpackhi_epi16(_mm256_abs_epi16(v_coef), _mm256_set1_epi16(0));
    __m256i v_quant_coeff_a = _mm256_unpacklo_epi16(v_quant_coeff, _mm256_set1_epi16(0));
    __m256i v_quant_coeff_b = _mm256_unpackhi_epi16(v_quant_coeff, _mm256_set1_epi16(0));
    v_coef_a = _mm256_madd_epi16(v_coef_a, v_quant_coeff_a);
    v_coef_b = _mm256_madd_epi16(v_coef_b, v_quant_coeff_b);
    v_coef_a = _mm256_sub_epi32(v_coef_a, _mm256_slli_epi32(_mm256_unpacklo_epi16(v_level, _mm256_set1_epi16(0)), q_bits) );
    v_coef_b = _mm256_sub_epi32(v_coef_b, _mm256_slli_epi32(_mm256_unpackhi_epi16(v_level, _mm256_set1_epi16(0)), q_bits) );
    v_coef_a = _mm256_srai_epi32(v_coef_a, q_bits8);
    v_coef_b = _mm256_srai_epi32(v_coef_b, q_bits8);
    
    _mm_storeu_si128((__m128i*)&(delta_u[n+0*4]), _mm256_castsi256_si128(v_coef_a));
    _mm_storeu_si128((__m128i*)&(delta_u[n+2*4]), _mm256_extracti128_si256(v_coef_a, 1));
    _mm_storeu_si128((__m128i*)&(delta_u[n+1*4]), _mm256_castsi256_si128(v_coef_b));
    _mm_storeu_si128((__m128i*)&(delta_u[n+3*4]), _mm256_extracti128_si256(v_coef_b, 1));
  }

  if (ac_sum >= 2) {
#define SCAN_SET_SIZE 16
#define LOG2_SCAN_SET_SIZE 4
    int32_t n, last_cg = -1, abssum = 0, subset, subpos;
    for (subset = (width*height - 1) >> LOG2_SCAN_SET_SIZE; subset >= 0; subset--) {
      int32_t first_nz_pos_in_cg = SCAN_SET_SIZE, last_nz_pos_in_cg = -1;
      subpos = subset << LOG2_SCAN_SET_SIZE;
      abssum = 0;

      // Find last coeff pos
      for (n = SCAN_SET_SIZE - 1; n >= 0; n--)  {
        if (q_coef[scan[n + subpos]])  {
          last_nz_pos_in_cg = n;
          break;
        }
      }

      // First coeff pos
      for (n = 0; n <SCAN_SET_SIZE; n++) {
        if (q_coef[scan[n + subpos]]) {
          first_nz_pos_in_cg = n;
          break;
        }
      }

      // Sum all kvz_quant coeffs between first and last
      for (n = first_nz_pos_in_cg; n <= last_nz_pos_in_cg; n++) {
        abssum += q_coef[scan[n + subpos]];
      }

      if (last_nz_pos_in_cg >= 0 && last_cg == -1) {
        last_cg = 1;
      }

      if (last_nz_pos_in_cg - first_nz_pos_in_cg >= 4) {
        int32_t signbit = (q_coef[scan[subpos + first_nz_pos_in_cg]] > 0 ? 0 : 1);
        if (signbit != (abssum & 0x1)) { // compare signbit with sum_parity
          int32_t min_cost_inc = 0x7fffffff, min_pos = -1, cur_cost = 0x7fffffff;
          int16_t final_change = 0, cur_change = 0;
          for (n = (last_cg == 1 ? last_nz_pos_in_cg : SCAN_SET_SIZE - 1); n >= 0; n--) {
            uint32_t blkPos = scan[n + subpos];
            if (q_coef[blkPos] != 0) {
              if (delta_u[blkPos] > 0) {
                cur_cost = -delta_u[blkPos];
                cur_change = 1;
              }
              else if (n == first_nz_pos_in_cg && abs(q_coef[blkPos]) == 1) {
                cur_cost = 0x7fffffff;
              }
              else {
                cur_cost = delta_u[blkPos];
                cur_change = -1;
              }
            }
            else if (n < first_nz_pos_in_cg && ((coef[blkPos] >= 0) ? 0 : 1) != signbit) {
              cur_cost = 0x7fffffff;
            }
            else {
              cur_cost = -delta_u[blkPos];
              cur_change = 1;
            }

            if (cur_cost < min_cost_inc) {
              min_cost_inc = cur_cost;
              final_change = cur_change;
              min_pos = blkPos;
            }
          } // CG loop

          if (q_coef[min_pos] == 32767 || q_coef[min_pos] == -32768) {
            final_change = -1;
          }

          if (coef[min_pos] >= 0) q_coef[min_pos] += final_change;
          else q_coef[min_pos] -= final_change;
        } // Hide
      }
      if (last_cg == 1) last_cg = 0;
    }

#undef SCAN_SET_SIZE
#undef LOG2_SCAN_SET_SIZE
  }
}

static INLINE __m128i get_residual_4x1_avx2(const kvz_pixel *a_in, const kvz_pixel *b_in){
  __m128i a = _mm_cvtsi32_si128(*(int32_t*)a_in);
  __m128i b = _mm_cvtsi32_si128(*(int32_t*)b_in);
  __m128i diff = _mm_sub_epi16(_mm_cvtepu8_epi16(a), _mm_cvtepu8_epi16(b) );
  return diff;
}

static INLINE __m128i get_residual_8x1_avx2(const kvz_pixel *a_in, const kvz_pixel *b_in){
  __m128i a = _mm_cvtsi64_si128(*(int64_t*)a_in);
  __m128i b = _mm_cvtsi64_si128(*(int64_t*)b_in);
  __m128i diff = _mm_sub_epi16(_mm_cvtepu8_epi16(a), _mm_cvtepu8_epi16(b) );
  return diff;
}

static INLINE int32_t get_quantized_recon_4x1_avx2(int16_t *residual, const kvz_pixel *pred_in){
  __m128i res = _mm_loadl_epi64((__m128i*)residual);
  __m128i pred = _mm_cvtsi32_si128(*(int32_t*)pred_in);
  __m128i rec = _mm_add_epi16(res, _mm_cvtepu8_epi16(pred));
  return _mm_cvtsi128_si32(_mm_packus_epi16(rec, rec));
}

static INLINE int64_t get_quantized_recon_8x1_avx2(int16_t *residual, const kvz_pixel *pred_in){
  __m128i res = _mm_loadu_si128((__m128i*)residual);
  __m128i pred = _mm_cvtsi64_si128(*(int64_t*)pred_in);
  __m128i rec = _mm_add_epi16(res, _mm_cvtepu8_epi16(pred));
  return _mm_cvtsi128_si64(_mm_packus_epi16(rec, rec));
}

static void get_residual_avx2(const kvz_pixel *ref_in, const kvz_pixel *pred_in, int16_t *residual, int width, int in_stride){

  __m128i diff = _mm_setzero_si128();
  switch (width) {
    case 4:
      diff = get_residual_4x1_avx2(ref_in + 0 * in_stride, pred_in + 0 * in_stride);
      _mm_storel_epi64((__m128i*)&(residual[0]), diff);
      diff = get_residual_4x1_avx2(ref_in + 1 * in_stride, pred_in + 1 * in_stride);
      _mm_storel_epi64((__m128i*)&(residual[4]), diff);
      diff = get_residual_4x1_avx2(ref_in + 2 * in_stride, pred_in + 2 * in_stride);
      _mm_storel_epi64((__m128i*)&(residual[8]), diff);
      diff = get_residual_4x1_avx2(ref_in + 3 * in_stride, pred_in + 3 * in_stride);
      _mm_storel_epi64((__m128i*)&(residual[12]), diff);
    break;
    case 8:
      diff = get_residual_8x1_avx2(&ref_in[0 * in_stride], &pred_in[0 * in_stride]);
      _mm_storeu_si128((__m128i*)&(residual[0]), diff);
      diff = get_residual_8x1_avx2(&ref_in[1 * in_stride], &pred_in[1 * in_stride]);
      _mm_storeu_si128((__m128i*)&(residual[8]), diff);
      diff = get_residual_8x1_avx2(&ref_in[2 * in_stride], &pred_in[2 * in_stride]);
      _mm_storeu_si128((__m128i*)&(residual[16]), diff);
      diff = get_residual_8x1_avx2(&ref_in[3 * in_stride], &pred_in[3 * in_stride]);
      _mm_storeu_si128((__m128i*)&(residual[24]), diff);
      diff = get_residual_8x1_avx2(&ref_in[4 * in_stride], &pred_in[4 * in_stride]);
      _mm_storeu_si128((__m128i*)&(residual[32]), diff);
      diff = get_residual_8x1_avx2(&ref_in[5 * in_stride], &pred_in[5 * in_stride]);
      _mm_storeu_si128((__m128i*)&(residual[40]), diff);
      diff = get_residual_8x1_avx2(&ref_in[6 * in_stride], &pred_in[6 * in_stride]);
      _mm_storeu_si128((__m128i*)&(residual[48]), diff);
      diff = get_residual_8x1_avx2(&ref_in[7 * in_stride], &pred_in[7 * in_stride]);
      _mm_storeu_si128((__m128i*)&(residual[56]), diff);
    break;
    default:
      for (int y = 0; y < width; ++y) {
        for (int x = 0; x < width; x+=16) {
          diff = get_residual_8x1_avx2(&ref_in[x + y * in_stride], &pred_in[x + y * in_stride]);
          _mm_storeu_si128((__m128i*)&residual[x + y * width], diff);
          diff = get_residual_8x1_avx2(&ref_in[(x+8) + y * in_stride], &pred_in[(x+8) + y * in_stride]);
          _mm_storeu_si128((__m128i*)&residual[(x+8) + y * width], diff);
        }
      }
    break;
  }
}

static void get_quantized_recon_avx2(int16_t *residual, const kvz_pixel *pred_in, int in_stride, kvz_pixel *rec_out, int out_stride, int width){

  switch (width) {
    case 4:
      *(int32_t*)&(rec_out[0 * out_stride]) = get_quantized_recon_4x1_avx2(residual + 0 * width, pred_in + 0 * in_stride);
      *(int32_t*)&(rec_out[1 * out_stride]) = get_quantized_recon_4x1_avx2(residual + 1 * width, pred_in + 1 * in_stride);
      *(int32_t*)&(rec_out[2 * out_stride]) = get_quantized_recon_4x1_avx2(residual + 2 * width, pred_in + 2 * in_stride);
      *(int32_t*)&(rec_out[3 * out_stride]) = get_quantized_recon_4x1_avx2(residual + 3 * width, pred_in + 3 * in_stride);
      break;
    case 8:
      *(int64_t*)&(rec_out[0 * out_stride]) = get_quantized_recon_8x1_avx2(residual + 0 * width, pred_in + 0 * in_stride);
      *(int64_t*)&(rec_out[1 * out_stride]) = get_quantized_recon_8x1_avx2(residual + 1 * width, pred_in + 1 * in_stride);
      *(int64_t*)&(rec_out[2 * out_stride]) = get_quantized_recon_8x1_avx2(residual + 2 * width, pred_in + 2 * in_stride);
      *(int64_t*)&(rec_out[3 * out_stride]) = get_quantized_recon_8x1_avx2(residual + 3 * width, pred_in + 3 * in_stride);
      *(int64_t*)&(rec_out[4 * out_stride]) = get_quantized_recon_8x1_avx2(residual + 4 * width, pred_in + 4 * in_stride);
      *(int64_t*)&(rec_out[5 * out_stride]) = get_quantized_recon_8x1_avx2(residual + 5 * width, pred_in + 5 * in_stride);
      *(int64_t*)&(rec_out[6 * out_stride]) = get_quantized_recon_8x1_avx2(residual + 6 * width, pred_in + 6 * in_stride);
      *(int64_t*)&(rec_out[7 * out_stride]) = get_quantized_recon_8x1_avx2(residual + 7 * width, pred_in + 7 * in_stride);
      break;
    default:
      for (int y = 0; y < width; ++y) {
        for (int x = 0; x < width; x += 16) {
          *(int64_t*)&(rec_out[x + y * out_stride]) = get_quantized_recon_8x1_avx2(residual + x + y * width, pred_in + x + y  * in_stride);
          *(int64_t*)&(rec_out[(x + 8) + y * out_stride]) = get_quantized_recon_8x1_avx2(residual + (x + 8) + y * width, pred_in + (x + 8) + y  * in_stride);
        }
      }
      break;
  }
}

/**
* \brief Quantize residual and get both the reconstruction and coeffs.
*
* \param width  Transform width.
* \param color  Color.
* \param scan_order  Coefficient scan order.
* \param use_trskip  Whether transform skip is used.
* \param stride  Stride for ref_in, pred_in and rec_out.
* \param ref_in  Reference pixels.
* \param pred_in  Predicted pixels.
* \param rec_out  Reconstructed pixels.
* \param coeff_out  Coefficients used for reconstruction of rec_out.
*
* \returns  Whether coeff_out contains any non-zero coefficients.
*/
int kvz_quantize_residual_avx2(encoder_state_t *const state,
  const cu_info_t *const cur_cu, const int width, const color_t color,
  const coeff_scan_order_t scan_order, const int use_trskip,
  const int in_stride, const int out_stride,
  const kvz_pixel *const ref_in, const kvz_pixel *const pred_in,
  kvz_pixel *rec_out, coeff_t *coeff_out)
{
  // Temporary arrays to pass data to and from kvz_quant and transform functions.
  int16_t residual[TR_MAX_WIDTH * TR_MAX_WIDTH];
  coeff_t coeff[TR_MAX_WIDTH * TR_MAX_WIDTH];

  int has_coeffs = 0;

  assert(width <= TR_MAX_WIDTH);
  assert(width >= TR_MIN_WIDTH);

  // Get residual. (ref_in - pred_in -> residual)
  get_residual_avx2(ref_in, pred_in, residual, width, in_stride);

  // Transform residual. (residual -> coeff)
  if (use_trskip) {
    kvz_transformskip(state->encoder_control, residual, coeff, width);
  }
  else {
    kvz_transform2d(state->encoder_control, residual, coeff, width, (color == COLOR_Y ? 0 : 65535));
  }

  // Quantize coeffs. (coeff -> coeff_out)
  if (state->encoder_control->cfg.rdoq_enable &&
      (width > 4 || !state->encoder_control->cfg.rdoq_skip))
  {
    int8_t tr_depth = cur_cu->tr_depth - cur_cu->depth;
    tr_depth += (cur_cu->part_size == SIZE_NxN ? 1 : 0);
    kvz_rdoq(state, coeff, coeff_out, width, width, (color == COLOR_Y ? 0 : 2),
      scan_order, cur_cu->type, tr_depth);
  } else {
    kvz_quant(state, coeff, coeff_out, width, width, (color == COLOR_Y ? 0 : 2),
      scan_order, cur_cu->type);
  }

  // Check if there are any non-zero coefficients.
  for (int i = 0; i < width * width; i += 8) {
    __m128i v_quant_coeff = _mm_loadu_si128((__m128i*)&(coeff_out[i]));
    has_coeffs = !_mm_testz_si128(_mm_set1_epi8(0xFF), v_quant_coeff);
    if(has_coeffs) break;
  }

  // Do the inverse quantization and transformation and the reconstruction to
  // rec_out.
  if (has_coeffs) {

    // Get quantized residual. (coeff_out -> coeff -> residual)
    kvz_dequant(state, coeff_out, coeff, width, width, (color == COLOR_Y ? 0 : (color == COLOR_U ? 2 : 3)), cur_cu->type);
    if (use_trskip) {
      kvz_itransformskip(state->encoder_control, residual, coeff, width);
    }
    else {
      kvz_itransform2d(state->encoder_control, residual, coeff, width, (color == COLOR_Y ? 0 : 65535));
    }

    // Get quantized reconstruction. (residual + pred_in -> rec_out)
    get_quantized_recon_avx2(residual, pred_in, in_stride, rec_out, out_stride, width);
  }
  else if (rec_out != pred_in) {
    // With no coeffs and rec_out == pred_int we skip copying the coefficients
    // because the reconstruction is just the prediction.
    int y, x;

    for (y = 0; y < width; ++y) {
      for (x = 0; x < width; ++x) {
        rec_out[x + y * out_stride] = pred_in[x + y * in_stride];
      }
    }
  }

  return has_coeffs;
}

void kvz_quant_avx2(const encoder_state_t * const state, coeff_t *coef, coeff_t *q_coef, int32_t width,
  int32_t height, int8_t type, int8_t scan_idx, int8_t block_type)
{
  if (state->encoder_control->scaling_list.enable){
    kvz_quant_generic(state, coef, q_coef, width, height, type, scan_idx, block_type);
  }
  else {
    kvz_quant_flat_avx2(state, coef, q_coef, width, height, type, scan_idx, block_type);
  }
}

/**
 * \brief inverse quantize transformed and quantized coefficents
 *
 */
void kvz_dequant_avx2(const encoder_state_t * const state, coeff_t *q_coef, coeff_t *coef, int32_t width, int32_t height,int8_t type, int8_t block_type)
{
  const encoder_control_t * const encoder = state->encoder_control;
  int32_t shift,add,coeff_q;
  int32_t n;
  int32_t transform_shift = 15 - encoder->bitdepth - (kvz_g_convert_to_bit[ width ] + 2);

  int32_t qp_scaled = kvz_get_scaled_qp(type, state->qp, (encoder->bitdepth-8)*6);

  shift = 20 - QUANT_SHIFT - transform_shift;

  if (encoder->scaling_list.enable)
  {
    uint32_t log2_tr_size = kvz_g_convert_to_bit[ width ] + 2;
    int32_t scalinglist_type = (block_type == CU_INTRA ? 0 : 3) + (int8_t)("\0\3\1\2"[type]);

    const int32_t *dequant_coef = encoder->scaling_list.de_quant_coeff[log2_tr_size-2][scalinglist_type][qp_scaled%6];
    shift += 4;

    if (shift >qp_scaled / 6) {
      add = 1 << (shift - qp_scaled/6 - 1);

      for (n = 0; n < width * height; n++) {
        coeff_q = ((q_coef[n] * dequant_coef[n]) + add ) >> (shift -  qp_scaled/6);
        coef[n] = (coeff_t)CLIP(-32768,32767,coeff_q);
      }
    } else {
      for (n = 0; n < width * height; n++) {
        // Clip to avoid possible overflow in following shift left operation
        coeff_q   = CLIP(-32768, 32767, q_coef[n] * dequant_coef[n]);
        coef[n] = (coeff_t)CLIP(-32768, 32767, coeff_q << (qp_scaled/6 - shift));
      }
    }
  } else {
    int32_t scale = kvz_g_inv_quant_scales[qp_scaled%6] << (qp_scaled/6);
    add = 1 << (shift-1);

    __m256i v_scale = _mm256_set1_epi32(scale);
    __m256i v_add = _mm256_set1_epi32(add);

    for (n = 0; n < width*height; n+=16) {
      __m128i temp0 = _mm_loadu_si128((__m128i*)&(q_coef[n]));
      __m128i temp1 = _mm_loadu_si128((__m128i*)&(q_coef[n + 8]));
      __m256i v_coeff_q_lo = _mm256_cvtepi16_epi32(_mm_unpacklo_epi64(temp0, temp1));
      __m256i v_coeff_q_hi = _mm256_cvtepi16_epi32(_mm_unpackhi_epi64(temp0, temp1));
      v_coeff_q_lo = _mm256_mullo_epi32(v_coeff_q_lo, v_scale);
      v_coeff_q_hi = _mm256_mullo_epi32(v_coeff_q_hi, v_scale);
      v_coeff_q_lo = _mm256_add_epi32(v_coeff_q_lo, v_add);
      v_coeff_q_hi = _mm256_add_epi32(v_coeff_q_hi, v_add);
      v_coeff_q_lo = _mm256_srai_epi32(v_coeff_q_lo, shift);
      v_coeff_q_hi = _mm256_srai_epi32(v_coeff_q_hi, shift);
      v_coeff_q_lo = _mm256_packs_epi32(v_coeff_q_lo, v_coeff_q_hi);
      _mm_storeu_si128((__m128i*)&(coef[n]), _mm256_castsi256_si128(v_coeff_q_lo) );
      _mm_storeu_si128((__m128i*)&(coef[n + 8]), _mm256_extracti128_si256(v_coeff_q_lo, 1) );
    }
  }
}

static uint32_t coeff_abs_sum_avx2(const coeff_t *coeffs, const size_t length)
{
  assert(length % 8 == 0);

  __m256i total = _mm256_abs_epi32(_mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*) coeffs)));

  for (int i = 8; i < length; i += 8) {
    __m256i temp = _mm256_abs_epi32(_mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*) &coeffs[i])));
    total = _mm256_add_epi32(total, temp);
  }

  __m128i result128 = _mm_add_epi32(
    _mm256_castsi256_si128(total),
    _mm256_extractf128_si256(total, 1)
  );

  uint32_t parts[4];
  _mm_storeu_si128((__m128i*) parts, result128);

  return parts[0] + parts[1] + parts[2] + parts[3];
}

#endif //COMPILE_INTEL_AVX2 && defined X86_64

int kvz_strategy_register_quant_avx2(void* opaque, uint8_t bitdepth)
{
  bool success = true;

#if COMPILE_INTEL_AVX2 && defined X86_64
  success &= kvz_strategyselector_register(opaque, "quant", "avx2", 40, &kvz_quant_avx2);
  if (bitdepth == 8) {
    success &= kvz_strategyselector_register(opaque, "quantize_residual", "avx2", 40, &kvz_quantize_residual_avx2);
    success &= kvz_strategyselector_register(opaque, "dequant", "avx2", 40, &kvz_dequant_avx2);
  }
  success &= kvz_strategyselector_register(opaque, "coeff_abs_sum", "avx2", 0, &coeff_abs_sum_avx2);
#endif //COMPILE_INTEL_AVX2 && defined X86_64

  return success;
}
