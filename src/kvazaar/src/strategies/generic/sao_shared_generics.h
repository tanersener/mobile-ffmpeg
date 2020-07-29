#ifndef SAO_BAND_DDISTORTION_H_
#define SAO_BAND_DDISTORTION_H_

// #include "encoder.h"
#include "encoderstate.h"
#include "kvazaar.h"
#include "sao.h"

// Mapping of edge_idx values to eo-classes.
static int sao_calc_eo_cat(kvz_pixel a, kvz_pixel b, kvz_pixel c)
{
  // Mapping relationships between a, b and c to eo_idx.
  static const int sao_eo_idx_to_eo_category[] = { 1, 2, 0, 3, 4 };

  int eo_idx = 2 + SIGN3((int)c - (int)a) + SIGN3((int)c - (int)b);

  return sao_eo_idx_to_eo_category[eo_idx];
}

static int sao_edge_ddistortion_generic(const kvz_pixel *orig_data,
                                        const kvz_pixel *rec_data,
                                              int32_t    block_width,
                                              int32_t    block_height,
                                              int32_t    eo_class,
                                        const int32_t    offsets[NUM_SAO_EDGE_CATEGORIES])
{
  int y, x;
  int32_t sum = 0;
  vector2d_t a_ofs = g_sao_edge_offsets[eo_class][0];
  vector2d_t b_ofs = g_sao_edge_offsets[eo_class][1];

  for (y = 1; y < block_height - 1; y++) {
    for (x = 1; x < block_width - 1; x++) {
      uint32_t c_pos =  y            * block_width + x;
      uint32_t a_pos = (y + a_ofs.y) * block_width + x + a_ofs.x;
      uint32_t b_pos = (y + b_ofs.y) * block_width + x + b_ofs.x;

      uint8_t   a    =  rec_data[a_pos];
      uint8_t   b    =  rec_data[b_pos];
      uint8_t   c    =  rec_data[c_pos];
      uint8_t   orig = orig_data[c_pos];

      int32_t eo_cat = sao_calc_eo_cat(a, b, c);
      int32_t offset = offsets[eo_cat];

      if (offset != 0) {
        int32_t diff   = orig - c;
        int32_t delta  = diff - offset;
        int32_t curr   = delta * delta - diff * diff;

        sum += curr;
      }
    }
  }
  return sum;
}

static int sao_band_ddistortion_generic(const encoder_state_t * const state,
                                        const kvz_pixel *orig_data,
                                        const kvz_pixel *rec_data,
                                        int block_width,
                                        int block_height,
                                        int band_pos,
                                        const int sao_bands[4])
{
  int y, x;
  int shift = state->encoder_control->bitdepth-5;
  int sum = 0;
  for (y = 0; y < block_height; ++y) {
    for (x = 0; x < block_width; ++x) {
      const int32_t curr_pos = y * block_width + x;

      kvz_pixel rec  =  rec_data[curr_pos];
      kvz_pixel orig = orig_data[curr_pos];

      int32_t band = (rec >> shift) - band_pos;
      int32_t offset = 0;
      if (band >= 0 && band <= 3) {
        offset = sao_bands[band];
      }
      // Offset is applied to reconstruction, so it is subtracted from diff.

      int32_t diff  = orig - rec;
      int32_t delta = diff - offset;

      int32_t dmask = (offset == 0) ? -1 : 0;
      diff  &= ~dmask;
      delta &= ~dmask;

      sum += delta * delta - diff * diff;
    }
  }

  return sum;
}

#endif
