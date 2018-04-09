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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "scalinglist.h"
#include "rdo.h"
#include "tables.h"


const uint8_t kvz_g_scaling_list_num[4]    = { 6, 6, 6, 2};
const uint16_t kvz_g_scaling_list_size[4]  = {   16,  64, 256,1024};
static const uint8_t g_scaling_list_size_x[4] = { 4, 8,16,32};

static const int32_t g_quant_default_4x4[16] =
{
  16,16,16,16,
  16,16,16,16,
  16,16,16,16,
  16,16,16,16
};

static const int32_t g_quant_intra_default_8x8[64] =
{
  16,16,16,16,17,18,21,24,
  16,16,16,16,17,19,22,25,
  16,16,17,18,20,22,25,29,
  16,16,18,21,24,27,31,36,
  17,17,20,24,30,35,41,47,
  18,19,22,27,35,44,54,65,
  21,22,25,31,41,54,70,88,
  24,25,29,36,47,65,88,115
};

static const int32_t g_quant_inter_default_8x8[64] =
{
  16,16,16,16,17,18,20,24,
  16,16,16,17,18,20,24,25,
  16,16,17,18,20,24,25,28,
  16,17,18,20,24,25,28,33,
  17,18,20,24,25,28,33,41,
  18,20,24,25,28,33,41,54,
  20,24,25,28,33,41,54,71,
  24,25,28,33,41,54,71,91
};

const int16_t kvz_g_quant_scales[6]        = { 26214,23302,20560,18396,16384,14564 };
const int16_t kvz_g_inv_quant_scales[6]    = { 40,45,51,57,64,72 };


/**
 * \brief Initialize scaling lists
 *
 */
void kvz_scalinglist_init(scaling_list_t * const scaling_list)
{
  uint32_t sizeId,listId,qp;

  for (sizeId = 0; sizeId < 4; sizeId++) {
    for (listId = 0; listId < kvz_g_scaling_list_num[sizeId]; listId++) {
      for (qp = 0; qp < 6; qp++) {
        if (!(sizeId == 3 && listId == 3)) {
          scaling_list->quant_coeff[sizeId][listId][qp]    = (int32_t*)calloc(kvz_g_scaling_list_size[sizeId], sizeof(int32_t));
          scaling_list->de_quant_coeff[sizeId][listId][qp] = (int32_t*)calloc(kvz_g_scaling_list_size[sizeId], sizeof(int32_t));
          scaling_list->error_scale[sizeId][listId][qp]    = (double*)calloc(kvz_g_scaling_list_size[sizeId], sizeof(double));
        }
      }
      scaling_list->scaling_list_coeff[sizeId][listId] = (int32_t*)calloc(MIN(MAX_MATRIX_COEF_NUM, kvz_g_scaling_list_size[sizeId]), sizeof(int32_t));
    }
  }
  // alias, assign pointer to an existing array
  for (qp = 0; qp < 6; qp++) {
    scaling_list->quant_coeff[3][3][qp]    = scaling_list->quant_coeff[3][1][qp];
    scaling_list->de_quant_coeff[3][3][qp] = scaling_list->de_quant_coeff[3][1][qp];
    scaling_list->error_scale[3][3][qp]    = scaling_list->error_scale[3][1][qp];
  }
  
  //Initialize dc (otherwise we switch on undef in kvz_scalinglist_set)
  for (sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; ++sizeId) {
    for (listId = 0; listId < SCALING_LIST_NUM; ++listId) {
      scaling_list->scaling_list_dc[sizeId][listId] = 0;
    }
  }
  
  scaling_list->enable = 0;
}

/**
 * \brief Destroy scaling list allocated memory
 *
 */
void kvz_scalinglist_destroy(scaling_list_t * const scaling_list)
{
  uint32_t sizeId,listId,qp;

  for (sizeId = 0; sizeId < 4; sizeId++) {
    for (listId = 0; listId < kvz_g_scaling_list_num[sizeId]; listId++) {
      for (qp = 0; qp < 6; qp++) {
        if (!(sizeId == 3 && listId == 3)) {
          FREE_POINTER(scaling_list->quant_coeff[sizeId][listId][qp]);
          FREE_POINTER(scaling_list->de_quant_coeff[sizeId][listId][qp]);
          FREE_POINTER(scaling_list->error_scale[sizeId][listId][qp]);
        }
      }
      FREE_POINTER(scaling_list->scaling_list_coeff[sizeId][listId]);
    }
  }
}

int kvz_scalinglist_parse(scaling_list_t * const scaling_list, FILE *fp)
{
  #define LINE_BUFSIZE 1024
  static const char matrix_type[4][6][20] =
  {
    {
      "INTRA4X4_LUMA",
      "INTRA4X4_CHROMAU",
      "INTRA4X4_CHROMAV",
      "INTER4X4_LUMA",
      "INTER4X4_CHROMAU",
      "INTER4X4_CHROMAV"
    },
    {
      "INTRA8X8_LUMA",
      "INTRA8X8_CHROMAU",
      "INTRA8X8_CHROMAV",
      "INTER8X8_LUMA",
      "INTER8X8_CHROMAU",
      "INTER8X8_CHROMAV"
    },
    {
      "INTRA16X16_LUMA",
      "INTRA16X16_CHROMAU",
      "INTRA16X16_CHROMAV",
      "INTER16X16_LUMA",
      "INTER16X16_CHROMAU",
      "INTER16X16_CHROMAV"
    },
    {
      "INTRA32X32_LUMA",
      "INTER32X32_LUMA",
    },
  };
  static const char matrix_type_dc[2][6][22] =
  {
    {
      "INTRA16X16_LUMA_DC",
      "INTRA16X16_CHROMAU_DC",
      "INTRA16X16_CHROMAV_DC",
      "INTER16X16_LUMA_DC",
      "INTER16X16_CHROMAU_DC",
      "INTER16X16_CHROMAV_DC"
    },
    {
      "INTRA32X32_LUMA_DC",
      "INTER32X32_LUMA_DC",
    },
  };

  uint32_t size_id;
  for (size_id = 0; size_id < SCALING_LIST_SIZE_NUM; size_id++) {
    uint32_t list_id;
    uint32_t size = MIN(MAX_MATRIX_COEF_NUM, (int32_t)kvz_g_scaling_list_size[size_id]);
    //const uint32_t * const scan = (size_id == 0) ? kvz_g_sig_last_scan[SCAN_DIAG][1] : g_sig_last_scan_32x32;

    for (list_id = 0; list_id < kvz_g_scaling_list_num[size_id]; list_id++) {
      int found;
      uint32_t i;
      int32_t data;
      //This IS valid (our pointer is dynamically allocated in kvz_scalinglist_init)
      int32_t *coeff = (int32_t*) scaling_list->scaling_list_coeff[size_id][list_id];
      char line[LINE_BUFSIZE + 1] = { 0 }; // +1 for null-terminator

      // Go back for each matrix.
      fseek(fp, 0, SEEK_SET);

      do {
        if (!fgets(line, LINE_BUFSIZE, fp) ||
            ((found = !!strstr(line, matrix_type[size_id][list_id])) == 0 && feof(fp)))
          return 0;
      } while (!found);

      for (i = 0; i < size;) {
        char *p;
        if (!fgets(line, LINE_BUFSIZE, fp))
          return 0;
        p = line;

        // Read coefficients per line.
        // The comma (,) character is used as a separator.
        // The coefficients are stored in up-right diagonal order.
        do {
          int ret = sscanf(p, "%d", &data);
          if (ret != 1)
            break;
          else if (data < 1 || data > 255)
            return 0;

          coeff[i++] = data;
          if (i == size)
            break;

          // Seek to the next newline, null-terminator or comma.
          while (*p != '\n' && *p != '\0' && *p != ',')
            ++p;
          if (*p == ',')
            ++p;
        } while (*p != '\n' && *p != '\0');
      }

      // Set DC value.
      if (size_id >= SCALING_LIST_16x16) {
        fseek(fp, 0, SEEK_SET);

        do {
          if (!fgets(line, LINE_BUFSIZE, fp) ||
              ((found = !!strstr(line, matrix_type_dc[size_id - SCALING_LIST_16x16][list_id])) == 0 && feof(fp)))
            return 0;
        } while (!found);
        if (1 != fscanf(fp, "%d", &data) || data < 1 || data > 255)
          return 0;

        scaling_list->scaling_list_dc[size_id][list_id] = data;
      } else
        scaling_list->scaling_list_dc[size_id][list_id] = coeff[0];
    }
  }

  scaling_list->enable = 1;
  return 1;
  #undef LINE_BUFSIZE
}

const int32_t *kvz_scalinglist_get_default(const uint32_t size_id, const uint32_t list_id)
{
  const int32_t *list_ptr = g_quant_intra_default_8x8; // Default to "8x8" intra
  switch(size_id) {
    case SCALING_LIST_4x4:
      list_ptr = g_quant_default_4x4;
      break;
    case SCALING_LIST_8x8:
    case SCALING_LIST_16x16:
      if (list_id > 2) list_ptr = g_quant_inter_default_8x8;
      break;
    case SCALING_LIST_32x32:
      if (list_id > 0) list_ptr = g_quant_inter_default_8x8;
      break;
  }
  return list_ptr;
}


/**
 * \brief get scaling list for decoder
 *
 */
static void scalinglist_process_dec(const int32_t * const coeff, int32_t *dequantcoeff,
                                    int32_t inv_quant_scales, uint32_t height,
                                    uint32_t width, uint32_t ratio,
                                    int32_t size_num, uint32_t dc,
                                    uint8_t flat)
{
  uint32_t j,i;

  // Flat scaling list
  if (flat) {
    for (j = 0; j < height * width; j++) {
      *dequantcoeff++ = inv_quant_scales<<4;
    }
  } else {
    for (j = 0; j < height; j++) {
      for (i = 0; i < width; i++) {
        dequantcoeff[j*width + i] = inv_quant_scales * coeff[size_num * (j / ratio) + i / ratio];
      }
    }
    if (ratio > 1) {
      dequantcoeff[0] = inv_quant_scales * dc;
    }
  }
}

/**
 * \brief get scaling list for encoder
 *
 */
void kvz_scalinglist_process_enc(const int32_t * const coeff, int32_t* quantcoeff, const int32_t quant_scales,
                             const uint32_t height, const uint32_t width, const uint32_t ratio, 
                             const int32_t size_num, const uint32_t dc, const uint8_t flat)
{
  uint32_t j,i;
  int32_t nsqth = (height < width) ? 4: 1; //!< height ratio for NSQT
  int32_t nsqtw = (width < height) ? 4: 1; //!< width ratio for NSQT

  // Flat scaling list
  if (flat) {
    for (j = 0; j < height * width; j++) {
      *quantcoeff++ = quant_scales>>4;
    }
  } else {
    for (j = 0; j < height; j++) {
      for (i = 0; i < width; i++) {
        uint32_t coeffpos  = size_num * (j * nsqth / ratio) + i * nsqtw / ratio;
        quantcoeff[j*width + i] = quant_scales / ((coeffpos > 63) ? 1 : coeff[coeffpos]);
      }
    }
    if (ratio > 1) {
      quantcoeff[0] = quant_scales / dc;
    }
  }
}



/** set error scale coefficients
 * \param list List ID
 * \param uiSize Size
 * \param uiQP Quantization parameter
 */
static void scalinglist_set_err_scale(uint8_t bitdepth, scaling_list_t * const scaling_list, uint32_t list,uint32_t size, uint32_t qp)
{
  uint32_t log2_tr_size   = kvz_g_convert_to_bit[ g_scaling_list_size_x[size] ] + 2;
  int32_t transform_shift = MAX_TR_DYNAMIC_RANGE - bitdepth - log2_tr_size;  // Represents scaling through forward transform

  uint32_t i,max_num_coeff  = kvz_g_scaling_list_size[size];
  const int32_t *quantcoeff = scaling_list->quant_coeff[size][list][qp];
  //This cast is allowed, since error_scale is a malloc'd pointer in kvz_scalinglist_init
  double *err_scale         = (double *) scaling_list->error_scale[size][list][qp];

  // Compensate for scaling of bitcount in Lagrange cost function
  double scale = CTX_FRAC_ONE_BIT;
  // Compensate for scaling through forward transform
  scale = scale*pow(2.0,-2.0*transform_shift);
  for(i=0;i<max_num_coeff;i++) {
    err_scale[i] = scale / quantcoeff[i] / quantcoeff[i] / (1<<(2*(bitdepth-8)));
  }
}


/**
 * \brief set scaling lists
 *
 */
void kvz_scalinglist_set(scaling_list_t * const scaling_list, const int32_t * const coeff, uint32_t listId, uint32_t sizeId, uint32_t qp)
{
  const uint32_t width  = g_scaling_list_size_x[sizeId];
  const uint32_t height = g_scaling_list_size_x[sizeId];
  const uint32_t ratio  = g_scaling_list_size_x[sizeId] / MIN(8, g_scaling_list_size_x[sizeId]);
  const uint32_t dc = scaling_list->scaling_list_dc[sizeId][listId] != 0 ? scaling_list->scaling_list_dc[sizeId][listId] : 16;
  //These cast are allowed, since these are pointer's to malloc'd area in kvz_scalinglist_init
  int32_t *quantcoeff   = (int32_t*) scaling_list->quant_coeff[sizeId][listId][qp];
  int32_t *dequantcoeff = (int32_t*) scaling_list->de_quant_coeff[sizeId][listId][qp];

  // Encoder list
  kvz_scalinglist_process_enc(coeff, quantcoeff, kvz_g_quant_scales[qp]<<4, height, width, ratio,
                          MIN(8, g_scaling_list_size_x[sizeId]), dc, !scaling_list->enable);
  // Decoder list
  scalinglist_process_dec(coeff, dequantcoeff, kvz_g_inv_quant_scales[qp], height, width, ratio,
                          MIN(8, g_scaling_list_size_x[sizeId]), dc, !scaling_list->enable);


  // TODO: support NSQT
  // if(sizeId == /*SCALING_LIST_32x32*/3 || sizeId == /*SCALING_LIST_16x16*/2) { //for NSQT
  //   quantcoeff   = g_quant_coeff[listId][qp][sizeId-1][/*SCALING_LIST_VER*/1];
  //   kvz_scalinglist_process_enc(coeff,quantcoeff,g_quantScales[qp]<<4,height,width>>2,ratio,MIN(8,g_scalingListSizeX[sizeId]),/*scalingList->getScalingListDC(sizeId,listId)*/0);

  //   quantcoeff   = g_quant_coeff[listId][qp][sizeId-1][/*SCALING_LIST_HOR*/2];
  //   kvz_scalinglist_process_enc(coeff,quantcoeff,g_quantScales[qp]<<4,height>>2,width,ratio,MIN(8,g_scalingListSizeX[sizeId]),/*scalingList->getScalingListDC(sizeId,listId)*/0);
  // }
}

/**
 * \brief
 *
 */
void kvz_scalinglist_process(scaling_list_t * const scaling_list, uint8_t bitdepth)
{
  uint32_t size,list,qp;

  for (size = 0; size < SCALING_LIST_SIZE_NUM; size++) {
    for (list = 0; list < kvz_g_scaling_list_num[size]; list++) {
      const int32_t * const list_ptr = scaling_list->enable ?
                                       scaling_list->scaling_list_coeff[size][list] :
                                       kvz_scalinglist_get_default(size, list);

      for (qp = 0; qp < SCALING_LIST_REM_NUM; qp++) {
        kvz_scalinglist_set(scaling_list, list_ptr, list, size, qp);
        scalinglist_set_err_scale(bitdepth, scaling_list, list, size, qp);
      }
    }
  }
}