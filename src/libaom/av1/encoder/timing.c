/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include "aom_ports/mem.h"
#include "av1/common/timing.h"

void set_aom_dec_model_info(aom_dec_model_info_t *decoder_model) {
  decoder_model->encoder_decoder_buffer_delay_length = 16;
  decoder_model->buffer_removal_delay_length = 10;
  decoder_model->frame_presentation_delay_length = 10;
  decoder_model->bitrate_scale = 4;      // in units of 1024 bits/second
  decoder_model->buffer_size_scale = 6;  // in units of 1024 bits
}

void set_dec_model_op_parameters(aom_dec_model_op_parameters_t *op_params,
                                 aom_dec_model_info_t *decoder_model,
                                 int64_t bitrate) {
  op_params->decoder_model_param_present_flag = 1;
  op_params->bitrate = (uint32_t)ROUND_POWER_OF_TWO_64(
      bitrate, decoder_model->bitrate_scale + 6);
  op_params->buffer_size = (uint32_t)ROUND_POWER_OF_TWO_64(
      bitrate, decoder_model->buffer_size_scale + 4);
  op_params->cbr_flag = 0;
  op_params->decoder_buffer_delay = 90000 >> 1;  //  0.5 s
  op_params->encoder_buffer_delay = 90000 >> 1;  //  0.5 s
  op_params->low_delay_mode_flag = 0;
  op_params->display_model_param_present_flag = 1;
  op_params->initial_display_delay = 8;  // 8 frames delay by default
}
