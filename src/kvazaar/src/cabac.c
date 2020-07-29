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

#include "cabac.h"

#include "encoder.h"
#include "encoderstate.h"
#include "extras/crypto.h"
#include "kvazaar.h"

const uint8_t kvz_g_auc_next_state_mps[128] =
{
    2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,
   18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,
   34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,
   50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,
   66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,
   82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,
   98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
  114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 124, 125, 126, 127
};

const uint8_t kvz_g_auc_next_state_lps[128] =
{
   1,  0,  0,  1,  2,  3,  4,  5,  4,  5,  8,  9,  8,  9,  10,  11,
  12, 13, 14, 15, 16, 17, 18, 19, 18, 19, 22, 23, 22, 23,  24,  25,
  26, 27, 26, 27, 30, 31, 30, 31, 32, 33, 32, 33, 36, 37,  36,  37,
  38, 39, 38, 39, 42, 43, 42, 43, 44, 45, 44, 45, 46, 47,  48,  49,
  48, 49, 50, 51, 52, 53, 52, 53, 54, 55, 54, 55, 56, 57,  58,  59,
  58, 59, 60, 61, 60, 61, 60, 61, 62, 63, 64, 65, 64, 65,  66,  67,
  66, 67, 66, 67, 68, 69, 68, 69, 70, 71, 70, 71, 70, 71,  72,  73,
  72, 73, 72, 73, 74, 75, 74, 75, 74, 75, 76, 77, 76, 77, 126, 127
};

const uint8_t kvz_g_auc_lpst_table[64][4] =
{
  {128, 176, 208, 240},  {128, 167, 197, 227},  {128, 158, 187, 216},  {123, 150, 178, 205},  {116, 142, 169, 195},
  {111, 135, 160, 185},  {105, 128, 152, 175},  {100, 122, 144, 166},  { 95, 116, 137, 158},  { 90, 110, 130, 150},
  { 85, 104, 123, 142},  { 81,  99, 117, 135},  { 77,  94, 111, 128},  { 73,  89, 105, 122},  { 69,  85, 100, 116},
  { 66,  80,  95, 110},  { 62,  76,  90, 104},  { 59,  72,  86,  99},  { 56,  69,  81,  94},  { 53,  65,  77,  89},
  { 51,  62,  73,  85},  { 48,  59,  69,  80},  { 46,  56,  66,  76},  { 43,  53,  63,  72},  { 41,  50,  59,  69},
  { 39,  48,  56,  65},  { 37,  45,  54,  62},  { 35,  43,  51,  59},  { 33,  41,  48,  56},  { 32,  39,  46,  53},
  { 30,  37,  43,  50},  { 29,  35,  41,  48},  { 27,  33,  39,  45},  { 26,  31,  37,  43},  { 24,  30,  35,  41},
  { 23,  28,  33,  39},  { 22,  27,  32,  37},  { 21,  26,  30,  35},  { 20,  24,  29,  33},  { 19,  23,  27,  31},
  { 18,  22,  26,  30},  { 17,  21,  25,  28},  { 16,  20,  23,  27},  { 15,  19,  22,  25},  { 14,  18,  21,  24},
  { 14,  17,  20,  23},  { 13,  16,  19,  22},  { 12,  15,  18,  21},  { 12,  14,  17,  20},  { 11,  14,  16,  19},
  { 11,  13,  15,  18},  { 10,  12,  15,  17},  { 10,  12,  14,  16},  {  9,  11,  13,  15},  {  9,  11,  12,  14},
  {  8,  10,  12,  14},  {  8,   9,  11,  13},  {  7,   9,  11,  12},  {  7,   9,  10,  12},  {  7,   8,  10,  11},
  {  6,   8,   9,  11},  {  6,   7,   9,  10},  {  6,   7,   8,   9},  {  2,   2,   2,   2}
};

const uint8_t kvz_g_auc_renorm_table[32] =
{
  6, 5, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

/**
 * \brief Initialize struct cabac_data.
 */
void kvz_cabac_start(cabac_data_t * const data)
{
  data->low = 0;
  data->range = 510;
  data->bits_left = 23;
  data->num_buffered_bytes = 0;
  data->buffered_byte = 0xff;
  data->only_count = 0; // By default, write bits out
}

/**
 * \brief
 */
void kvz_cabac_encode_bin(cabac_data_t * const data, const uint32_t bin_value)
{
  uint32_t lps;


  lps = kvz_g_auc_lpst_table[CTX_STATE(data->cur_ctx)][(data->range >> 6) & 3];
  data->range -= lps;

  // Not the Most Probable Symbol?
  if ((bin_value ? 1 : 0) != CTX_MPS(data->cur_ctx)) {
    int num_bits = kvz_g_auc_renorm_table[lps >> 3];
    data->low = (data->low + data->range) << num_bits;
    data->range = lps << num_bits;

    CTX_UPDATE_LPS(data->cur_ctx);

    data->bits_left -= num_bits;
  } else {
    CTX_UPDATE_MPS(data->cur_ctx);
    if (data->range >= 256) return;

    data->low <<= 1;
    data->range <<= 1;
    data->bits_left--;
  }

  if (data->bits_left < 12) {
    kvz_cabac_write(data);
  }
}

/**
 * \brief
 */
void kvz_cabac_write(cabac_data_t * const data)
{
  uint32_t lead_byte = data->low >> (24 - data->bits_left);
  data->bits_left += 8;
  data->low &= 0xffffffffu >> data->bits_left;

  // Binary counter mode
  if(data->only_count) {
    data->num_buffered_bytes++;
    return;
  }

  if (lead_byte == 0xff) {
    data->num_buffered_bytes++;
  } else {
    if (data->num_buffered_bytes > 0) {
      uint32_t carry = lead_byte >> 8;
      uint32_t byte = data->buffered_byte + carry;
      data->buffered_byte = lead_byte & 0xff;
      kvz_bitstream_put_byte(data->stream, byte);

      byte = (0xff + carry) & 0xff;
      while (data->num_buffered_bytes > 1) {
        kvz_bitstream_put_byte(data->stream, byte);
        data->num_buffered_bytes--;
      }
    } else {
      data->num_buffered_bytes = 1;
      data->buffered_byte = lead_byte;
    }
  }
}

/**
 * \brief
 */
void kvz_cabac_finish(cabac_data_t * const data)
{
  assert(data->bits_left <= 32);

  if (data->low >> (32 - data->bits_left)) {
    kvz_bitstream_put_byte(data->stream, data->buffered_byte + 1);
    while (data->num_buffered_bytes > 1) {
      kvz_bitstream_put_byte(data->stream, 0);
      data->num_buffered_bytes--;
    }
    data->low -= 1 << (32 - data->bits_left);
  } else {
    if (data->num_buffered_bytes > 0) {
      kvz_bitstream_put_byte(data->stream, data->buffered_byte);
    }
    while (data->num_buffered_bytes > 1) {
      kvz_bitstream_put_byte(data->stream, 0xff);
      data->num_buffered_bytes--;
    }
  }

  {
    uint8_t bits = (uint8_t)(24 - data->bits_left);
    kvz_bitstream_put(data->stream, data->low >> 8, bits);
  }
}

/*!
  \brief Encode terminating bin
  \param binValue bin value
*/
void kvz_cabac_encode_bin_trm(cabac_data_t * const data, const uint8_t bin_value)
{
  data->range -= 2;
  if(bin_value) {
    data->low += data->range;
    data->low <<= 7;
    data->range = 2 << 7;
    data->bits_left -= 7;
  } else if (data->range >= 256) {
    return;
  } else {
    data->low <<= 1;
    data->range <<= 1;
    data->bits_left--;
  }

  if (data->bits_left < 12) {
    kvz_cabac_write(data);
  }
}

/**
 * \brief
 */
void kvz_cabac_encode_bin_ep(cabac_data_t * const data, const uint32_t bin_value)
{
  data->low <<= 1;
  if (bin_value) {
    data->low += data->range;
  }
  data->bits_left--;

  if (data->bits_left < 12) {
    kvz_cabac_write(data);
  }
}

/**
 * \brief
 */
void kvz_cabac_encode_bins_ep(cabac_data_t * const data, uint32_t bin_values, int num_bins)
{
  uint32_t pattern;

  while (num_bins > 8) {
    num_bins -= 8;
    pattern = bin_values >> num_bins;
    data->low <<= 8;
    data->low += data->range * pattern;
    bin_values -= pattern << num_bins;
    data->bits_left -= 8;

    if(data->bits_left < 12) {
      kvz_cabac_write(data);
    }
  }

  data->low <<= num_bins;
  data->low += data->range * bin_values;
  data->bits_left -= num_bins;

  if (data->bits_left < 12) {
    kvz_cabac_write(data);
  }
}

/**
 * \brief Coding of coeff_abs_level_minus3.
 * \param symbol Value of coeff_abs_level_minus3.
 * \param r_param Reference to Rice parameter.
 */
void kvz_cabac_write_coeff_remain(cabac_data_t * const cabac, const uint32_t symbol, const uint32_t r_param)
{
  int32_t code_number = symbol;
  uint32_t length;

  if (code_number < (3 << r_param)) {
    length = code_number >> r_param;
    CABAC_BINS_EP(cabac, (1 << (length + 1)) - 2 , length + 1, "coeff_abs_level_remaining");
    CABAC_BINS_EP(cabac, (code_number % (1 << r_param)), r_param, "coeff_abs_level_remaining");
  } else {
    length = r_param;
    code_number = code_number - (3 << r_param);
    while (code_number >= (1 << length)) {
      code_number -= 1 << length;
      ++length;
    }
    CABAC_BINS_EP(cabac, (1 << (3 + length + 1 - r_param)) - 2, 3 + length + 1 - r_param, "coeff_abs_level_remaining");
    CABAC_BINS_EP(cabac, code_number, length, "coeff_abs_level_remaining");
  }
}

void kvz_cabac_write_coeff_remain_encry(struct encoder_state_t * const state, cabac_data_t * const cabac,const uint32_t symbol, const uint32_t r_param, int32_t base_level)
{
 int32_t codeNumber  = (int32_t)symbol;
 uint32_t length;

 if (codeNumber < (3 << r_param)) {
   length = codeNumber>>r_param;
   CABAC_BINS_EP(cabac, (1 << (length + 1)) - 2 , length + 1, "coeff_abs_level_remaining");
   //m_pcBinIf->encodeBinsEP( (1<<(length+1))-2 , length+1);
   uint32_t Suffix = (codeNumber%(1<<r_param));

   if(!r_param)
    CABAC_BINS_EP(cabac, Suffix, r_param, "coeff_abs_level_remaining");
    //m_pcBinIf->encodeBinsEP(Suffix, r_param);
   if(r_param==1) {
     if(!(( base_level ==2 )&& (codeNumber==4 || codeNumber==5) ) ) {
       uint32_t key = kvz_crypto_get_key(state->crypto_hdl, 1);
       state->crypto_prev_pos  = ( Suffix + ( state->crypto_prev_pos^key ) ) & 1;
       CABAC_BINS_EP(cabac, state->crypto_prev_pos, 1, "coeff_abs_level_remaining");
       //m_pcBinIf->encodeBinsEP(m_prev_pos, 1);
     } else {
       CABAC_BINS_EP(cabac, Suffix, 1, "coeff_abs_level_remaining");
       //m_pcBinIf->encodeBinsEP(Suffix, 1);
     }
   }
   else
    if(r_param==2) {
       if( base_level ==1) {
         uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 2);
         state->crypto_prev_pos  = ( Suffix + ( state->crypto_prev_pos^key ) ) & 3;
         CABAC_BINS_EP(cabac, state->crypto_prev_pos, 2, "coeff_abs_level_remaining");
         //m_pcBinIf->encodeBinsEP(m_prev_pos, 2);
       } else
         if( base_level ==2) {
           if(codeNumber<=7 || codeNumber>=12) {
             uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 2);
             state->crypto_prev_pos  = ( Suffix + ( state->crypto_prev_pos^key ) ) & 3;
             CABAC_BINS_EP(cabac, state->crypto_prev_pos, 2, "coeff_abs_level_remaining");
             //m_pcBinIf->encodeBinsEP(m_prev_pos, 2);
           }
           else
             if(codeNumber<10) {
                uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 1);
                state->crypto_prev_pos  = (( (Suffix&1) + ( state->crypto_prev_pos^key )) & 1);
                CABAC_BINS_EP(cabac, state->crypto_prev_pos, 2, "coeff_abs_level_remaining");
                //m_pcBinIf->encodeBinsEP(m_prev_pos, 2);
             } else
               CABAC_BINS_EP(cabac, Suffix, 2, "coeff_abs_level_remaining");
               //m_pcBinIf->encodeBinsEP(Suffix, 2);
         } else { //base_level=3
           if(codeNumber<=7 || codeNumber>11) {
             uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 2);
             state->crypto_prev_pos  = (Suffix + ( state->crypto_prev_pos^key ) ) & 3;
             CABAC_BINS_EP(cabac, state->crypto_prev_pos, 2, "coeff_abs_level_remaining");
             //m_pcBinIf->encodeBinsEP(m_prev_pos, 2);
           } else {
             uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 1);
             state->crypto_prev_pos  = ((Suffix&2))+(( (Suffix&1) + ( state->crypto_prev_pos^key)) & 1);
             CABAC_BINS_EP(cabac, state->crypto_prev_pos, 2, "coeff_abs_level_remaining");
             //m_pcBinIf->encodeBinsEP(m_prev_pos, 2);
           }
         }
     } else
       if(r_param==3) {
         if( base_level ==1) {
           uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 3);
           state->crypto_prev_pos  = ( Suffix + ( state->crypto_prev_pos^key ) ) & 7;
           CABAC_BINS_EP(cabac, state->crypto_prev_pos, 3, "coeff_abs_level_remaining");
           //m_pcBinIf->encodeBinsEP(m_prev_pos, 3);
         }
         else if( base_level ==2) {
           if(codeNumber<=15 || codeNumber>23) {
             uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 3);
             state->crypto_prev_pos  = ( Suffix + ( state->crypto_prev_pos^key ) ) & 7;
             CABAC_BINS_EP(cabac, state->crypto_prev_pos, 3, "coeff_abs_level_remaining");
             //m_pcBinIf->encodeBinsEP(m_prev_pos, 3);
           } else
             if(codeNumber<=19){
               uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 2);
               state->crypto_prev_pos  = ((Suffix&4))+(( (Suffix&3) + (state->crypto_prev_pos^key )) & 3);
               CABAC_BINS_EP(cabac, state->crypto_prev_pos, 3, "coeff_abs_level_remaining");
               //m_pcBinIf->encodeBinsEP(m_prev_pos, 3);
             } else
               if(codeNumber<=21){
               uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 1);
                 state->crypto_prev_pos  = 4+(( (Suffix&1) + ( state->crypto_prev_pos^key )) & 1);
                 CABAC_BINS_EP(cabac, state->crypto_prev_pos, 3, "coeff_abs_level_remaining");
                 //m_pcBinIf->encodeBinsEP(m_prev_pos, 3);
               } else
                 CABAC_BINS_EP(cabac, Suffix, 3, "coeff_abs_level_remaining");
           // m_pcBinIf->encodeBinsEP(Suffix, 3);
         } else {//base_level=3
           CABAC_BINS_EP(cabac, Suffix, 3, "coeff_abs_level_remaining");
           //m_pcBinIf->encodeBinsEP(Suffix, 3);
           if(codeNumber<=15 || codeNumber>23) {
             uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 3);
             state->crypto_prev_pos  = (Suffix + ( state->crypto_prev_pos^key ) ) & 7;
             CABAC_BINS_EP(cabac, state->crypto_prev_pos, 3, "coeff_abs_level_remaining");
             //m_pcBinIf->encodeBinsEP(m_prev_pos, 3);
           } else
             if(codeNumber<=19) {
               uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 2);
               state->crypto_prev_pos  = (( (Suffix&3) + ( state->crypto_prev_pos^key )) &3);
               CABAC_BINS_EP(cabac, state->crypto_prev_pos, 3, "coeff_abs_level_remaining");
               //m_pcBinIf->encodeBinsEP(m_prev_pos, 3);
             } else
               if(codeNumber<=23) {
                 uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 1);
                 state->crypto_prev_pos  = (Suffix&6)+(( (Suffix&1) + (state->crypto_prev_pos^key )) & 1);
                 CABAC_BINS_EP(cabac, state->crypto_prev_pos, 3, "coeff_abs_level_remaining");
                 //m_pcBinIf->encodeBinsEP(m_prev_pos, 3);
               }
         }
       } else
         if(r_param==4) {
           if( base_level ==1) {
             uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 4);
             state->crypto_prev_pos  = ( Suffix + ( state->crypto_prev_pos^key ) ) & 15;
             CABAC_BINS_EP(cabac, state->crypto_prev_pos, 4, "coeff_abs_level_remaining");
             //m_pcBinIf->encodeBinsEP(m_prev_pos, 4);
           } else
             if( base_level ==2) {
               if(codeNumber<=31 || codeNumber>47) {
                 uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 4);
                 state->crypto_prev_pos  = ( Suffix + ( state->crypto_prev_pos^key ) ) & 15;
                 CABAC_BINS_EP(cabac, state->crypto_prev_pos, r_param, "coeff_abs_level_remaining");
                 //m_pcBinIf->encodeBinsEP(m_prev_pos, r_param);
               } else
                 if(codeNumber<=39) {
                   uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 3);
                   state->crypto_prev_pos  = (( (Suffix&7) + ( state->crypto_prev_pos^key )) & 7);
                   CABAC_BINS_EP(cabac, state->crypto_prev_pos, 4, "coeff_abs_level_remaining");
                   //m_pcBinIf->encodeBinsEP(m_prev_pos, 4);
                 } else
                   if(codeNumber<=43) {
                     uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 2);
                     state->crypto_prev_pos  = 8+(( (Suffix&3) + ( state->crypto_prev_pos^key )) & 3);
                     CABAC_BINS_EP(cabac, state->crypto_prev_pos, 4, "coeff_abs_level_remaining");
                     //m_pcBinIf->encodeBinsEP(m_prev_pos, 4);
                   } else
                     if(codeNumber<=45){
                       uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 1);
                       state->crypto_prev_pos  = 12+(( (Suffix&1) + ( state->crypto_prev_pos^key )) & 1);
                       CABAC_BINS_EP(cabac, state->crypto_prev_pos, 4, "coeff_abs_level_remaining");
                       //m_pcBinIf->encodeBinsEP(m_prev_pos, 4);
                     } else
                       CABAC_BINS_EP(cabac, Suffix, 4, "coeff_abs_level_remaining");
                       //m_pcBinIf->encodeBinsEP(Suffix, 4);
             } else {//base_level=3
               if(codeNumber<=31 || codeNumber>47) {
                 uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 4);
                 state->crypto_prev_pos  = (Suffix + ( state->crypto_prev_pos^key ) ) & 15;
                 CABAC_BINS_EP(cabac, state->crypto_prev_pos, r_param, "coeff_abs_level_remaining");
                 //m_pcBinIf->encodeBinsEP(m_prev_pos, r_param);
               } else
                 if(codeNumber<=39) {
                   uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 3);
                   state->crypto_prev_pos  = (( (Suffix&7) + ( state->crypto_prev_pos^key )) & 7);
                   CABAC_BINS_EP(cabac, state->crypto_prev_pos, 4, "coeff_abs_level_remaining");
                   //m_pcBinIf->encodeBinsEP(m_prev_pos, 4);
                 } else
                   if(codeNumber<=43) {
                     uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 2);
                     state->crypto_prev_pos  = 8+(( (Suffix&3) + ( state->crypto_prev_pos^key )) & 3);
                     CABAC_BINS_EP(cabac, state->crypto_prev_pos, 4, "coeff_abs_level_remaining");
                     //m_pcBinIf->encodeBinsEP(m_prev_pos, 4);
                   } else
                     if(codeNumber<=47) {
                       uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, 1);
                       state->crypto_prev_pos  = (Suffix&14)+(( (Suffix&1) + (state->crypto_prev_pos^key )) & 1);
                       CABAC_BINS_EP(cabac, state->crypto_prev_pos, 4, "coeff_abs_level_remaining");
                       //m_pcBinIf->encodeBinsEP(m_prev_pos, 4);
                     }
             }
       }
  } else {
    length = r_param;
    codeNumber  = codeNumber - ( 3 << r_param);
    while (codeNumber >= (1<<length)) {
      codeNumber -=  (1<<(length));
      ++length;
    }
    CABAC_BINS_EP(cabac, (1 << (3 + length + 1 - r_param)) - 2, 3 + length + 1 - r_param, "coeff_abs_level_remaining");
    //m_pcBinIf->encodeBinsEP((1<<(COEF_REMAIN_BIN_REDUCTION+length+1-r_param))-2,COEF_REMAIN_BIN_REDUCTION+length+1-r_param);
    uint32_t Suffix = codeNumber;
    uint32_t key    = kvz_crypto_get_key(state->crypto_hdl, length);
    uint32_t mask   = ( (1<<length ) -1 );
    state->crypto_prev_pos  = ( Suffix + ( state->crypto_prev_pos^key ) ) & mask;
    CABAC_BINS_EP(cabac, state->crypto_prev_pos, length, "coeff_abs_level_remaining");
    //m_pcBinIf->encodeBinsEP(m_prev_pos,length);
  }
}
/**
 * \brief
 */
void kvz_cabac_write_unary_max_symbol(cabac_data_t * const data, cabac_ctx_t * const ctx, uint32_t symbol, const int32_t offset, const uint32_t max_symbol)
{
  int8_t code_last = max_symbol > symbol;

  assert(symbol <= max_symbol);

  if (!max_symbol) return;

  data->cur_ctx = &ctx[0];
  CABAC_BIN(data, symbol, "ums");

  if (!symbol) return;

  while (--symbol) {
    data->cur_ctx = &ctx[offset];
    CABAC_BIN(data, 1, "ums");
  }
  if (code_last) {
    data->cur_ctx = &ctx[offset];
    CABAC_BIN(data, 0, "ums");
  }
}

/**
 * This can be used for Truncated Rice binarization with cRiceParam=0.
 */
void kvz_cabac_write_unary_max_symbol_ep(cabac_data_t * const data, unsigned int symbol, const unsigned int max_symbol)
{
  /*if (symbol == 0) {
    CABAC_BIN_EP(data, 0, "ums_ep");
  } else {
    // Make a bit-string of (symbol) times 1 and a single 0, except when
    // symbol == max_symbol.
    unsigned bins = ((1 << symbol) - 1) << (symbol < max_symbol);
    CABAC_BINS_EP(data, bins, symbol + (symbol < max_symbol), "ums_ep");
  }*/

  int8_t code_last = max_symbol > symbol;

  assert(symbol <= max_symbol);

  CABAC_BIN_EP(data, symbol ? 1 : 0, "ums_ep");

  if (!symbol) return;

  while (--symbol) {
    CABAC_BIN_EP(data, 1, "ums_ep");
  }
  if (code_last) {
    CABAC_BIN_EP(data, 0, "ums_ep");
  }
}

/**
 * \brief
 */
void kvz_cabac_write_ep_ex_golomb(encoder_state_t * const state,
                                  cabac_data_t * const data,
                                  uint32_t symbol,
                                  uint32_t count)
{
  uint32_t bins = 0;
  int32_t num_bins = 0;

  while (symbol >= (uint32_t)(1 << count)) {
    bins = 2 * bins + 1;
    ++num_bins;
    symbol -= 1 << count;
    ++count;
  }
  bins = 2 * bins;
  ++num_bins;

  bins      = (bins << count) | symbol;
  num_bins += count;
  if (!data->only_count) {
    if (state->encoder_control->cfg.crypto_features & KVZ_CRYPTO_MVs) {
      uint32_t key, mask;
      key                      = kvz_crypto_get_key(state->crypto_hdl, num_bins>>1);
      mask                     = ( (1<<(num_bins >>1) ) -1 );
      state->crypto_prev_pos  = ( bins + ( state->crypto_prev_pos^key ) ) & mask;
      bins                     = ( (bins >> (num_bins >>1) ) << (num_bins >>1) ) | state->crypto_prev_pos;
    }
  }
  kvz_cabac_encode_bins_ep(data, bins, num_bins);
}
