/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * Kvazaar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

//Compile with gcc -o generate_tables generate_tables.c
//Run ./generate_tables > ../src/tables.c

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../src/global.h"
#include "../src/tables.h"

const uint32_t* _sig_last_scan[3][5];
int8_t _convert_to_bit[LCU_WIDTH + 1];

/**
 * Initialize g_sig_last_scan with scan positions for a transform block of
 * size width x height.
 */
static void init_sig_last_scan(uint32_t *buff_d, uint32_t *buff_h,
                               uint32_t *buff_v,
                               int32_t width, int32_t height)
{
  uint32_t num_scan_pos  = width * width;
  uint32_t next_scan_pos = 0;
  int32_t  xx, yy, x, y;
  uint32_t scan_line;
  uint32_t blk_y, blk_x;
  uint32_t blk;
  uint32_t cnt = 0;

  assert(width == height && width <= 32);

  if (width <= 4) {
    uint32_t *buff_tmp = buff_d;

    for (scan_line = 0; next_scan_pos < num_scan_pos; scan_line++) {
      int    primary_dim  = scan_line;
      int    second_dim  = 0;

      while (primary_dim >= width) {
        second_dim++;
        primary_dim--;
      }

      while (primary_dim >= 0 && second_dim < width) {
        buff_tmp[next_scan_pos] = primary_dim * width + second_dim ;
        next_scan_pos++;
        second_dim++;
        primary_dim--;
      }
    }
  }

  if (width > 4 && width <= 32) {
    uint32_t num_blk_side = width >> 2;
    uint32_t num_blks   = num_blk_side * num_blk_side;
    uint32_t log2_width = g_to_bits[width];

    for (blk = 0; blk < num_blks; blk++) {
      uint32_t init_blk_pos = g_sig_last_scan_cg[log2_width][SCAN_DIAG][blk];
      next_scan_pos   = 0;

      {
        uint32_t offset_y    = init_blk_pos / num_blk_side;
        uint32_t offset_x    = init_blk_pos - offset_y * num_blk_side;
        uint32_t offset_d    = 4 * (offset_x + offset_y * width);
        uint32_t offset_scan = 16 * blk;

        for (scan_line = 0; next_scan_pos < 16; scan_line++) {
          int    primary_dim  = scan_line;
          int    second_dim  = 0;

          //TODO: optimize
          while (primary_dim >= 4) {
            second_dim++;
            primary_dim--;
          }

          while (primary_dim >= 0 && second_dim < 4) {
            buff_d[next_scan_pos + offset_scan] = primary_dim * width +
                                                  second_dim + offset_d;
            next_scan_pos++;
            second_dim++;
            primary_dim--;
          }
        }
      }
    }
  }

  if (width > 2) {
    uint32_t num_blk_side = width >> 2;

    for (blk_y = 0; blk_y < num_blk_side; blk_y++) {
      for (blk_x = 0; blk_x < num_blk_side; blk_x++) {
        uint32_t offset    = blk_y * 4 * width + blk_x * 4;

        for (y = 0; y < 4; y++) {
          for (x = 0; x < 4; x++) {
            buff_h[cnt] = y * width + x + offset;
            cnt ++;
          }
        }
      }
    }

    cnt = 0;

    for (blk_x = 0; blk_x < num_blk_side; blk_x++) {
      for (blk_y = 0; blk_y < num_blk_side; blk_y++) {
        uint32_t offset = blk_y * 4 * width + blk_x * 4;

        for (x = 0; x < 4; x++) {
          for (y = 0; y < 4; y++) {
            buff_v[cnt] = y * width + x + offset;
            cnt ++;
          }
        }
      }
    }
  } else {
    for (yy = 0; yy < height; yy++) {
      for (xx = 0; xx < width; xx++) {
        buff_h[cnt] = yy * width + xx;
        cnt ++;
  }
      }

    cnt = 0;

    for (xx = 0; xx < width; xx++) {
      for (yy = 0; yy < height; yy++) {
        buff_v[cnt] = yy * width + xx;
        cnt ++;
      }
    }
  }
}


void init_tables(void)
{
  int i;
  int c = 0;

  memset( _convert_to_bit,-1, sizeof( _convert_to_bit ) );

  for (i = 4; i < LCU_WIDTH; i *= 2) {
    _convert_to_bit[i] = (int8_t)c;
    c++;
  }

  _convert_to_bit[i] = (int8_t)c;

  c = 2;
  for (i = 0; i < 5; i++) {
    uint32_t *sls0, *sls1, *sls2;
    sls0 = (uint32_t*)malloc(c*c*sizeof(uint32_t));
    sls1 = (uint32_t*)malloc(c*c*sizeof(uint32_t));
    sls2 = (uint32_t*)malloc(c*c*sizeof(uint32_t));

    init_sig_last_scan(sls0, sls1, sls2, c, c);
    
    _sig_last_scan[0][i] = sls0;
    _sig_last_scan[1][i] = sls1;
    _sig_last_scan[2][i] = sls2;
    
    c <<= 1;
  }
}

void free_tables(void)
{
  int i;
  for (i = 0; i < 5; i++) {
    FREE_POINTER(_sig_last_scan[0][i]);
    FREE_POINTER(_sig_last_scan[1][i]);
    FREE_POINTER(_sig_last_scan[2][i]);
  }
}


int main() {
  int i, c, j, h;
  printf("//The file tables.c is automatically generated by generate_tables, do not edit.\n\n");
  printf("#include \"tables.h\"\n\n");
  printf("#if LCU_WIDTH!=%d\n", LCU_WIDTH);
  printf("#error \"LCU_WIDTH!=%d\"\n", LCU_WIDTH);
  printf("#endif\n\n");
  
  init_tables();
  
  printf("const int8_t g_convert_to_bit[LCU_WIDTH + 1] = {");
  for (i=0; i < LCU_WIDTH + 1; ++i) {
    if (i!=LCU_WIDTH) {
      printf("%d, ", _convert_to_bit[i]);
    } else {
      printf("%d", _convert_to_bit[i]);
    }
  }
  printf("};\n\n");
  
  c = 2;
  for (i = 0; i < 5; i++) {
    for (h = 0; h < 3; h++) {
      printf("static const uint32_t g_sig_last_scan_%d_%d[%d] = {",h,i,c*c);
      
      for (j = 0; j < c*c; ++j) {
        if (j!=c*c-1) {
          printf("%u, ", _sig_last_scan[h][i][j]);
        } else {
          printf("%u", _sig_last_scan[h][i][j]);
        }
        if (j % 100 == 99) printf("\n  ");
      }
      printf("};\n");
    }
    printf("\n");
    c <<= 1;
  }
  
  printf("const uint32_t* const g_sig_last_scan[3][5] = {\n");
  for (h = 0; h < 3; h++) {
    printf("  {");
    for (i = 0; i < 5; i++) {
      if (i!=4) {
        printf("g_sig_last_scan_%d_%d, ", h, i);
      } else {
        printf("g_sig_last_scan_%d_%d", h, i);
      }
    }
    if (h<2) {
      printf("},\n");
    } else {
      printf("}\n");
    }
  }
  printf("};\n");
  return 0;
}