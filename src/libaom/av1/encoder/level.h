/*
 * Copyright (c) 2019, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_AV1_ENCODER_LEVEL_H_
#define AOM_AV1_ENCODER_LEVEL_H_

#include "av1/common/enums.h"

struct AV1_COMP;

// AV1 Level Specifications
typedef struct {
  AV1_LEVEL level;
  int max_picture_size;
  int max_h_size;
  int max_v_size;
  int max_header_rate;
  int max_tiles;
  int max_tile_cols;
  int64_t max_display_rate;
  int64_t max_decode_rate;
  double main_mbps;
  double high_mbps;
  double main_cr;
  double high_cr;
} AV1LevelSpec;

typedef struct {
  int64_t ts_start;
  int64_t ts_end;
  int pic_size;
  int frame_header_count;
  int show_frame;
  int show_existing_frame;
} FrameRecord;

// Record frame info. in a rolling window.
#define FRAME_WINDOW_SIZE 256
typedef struct {
  FrameRecord buf[FRAME_WINDOW_SIZE];
  int num;    // Number of FrameRecord stored in the buffer.
  int start;  // Buffer index of the first FrameRecord.
} FrameWindowBuffer;

// Used to keep track of AV1 Level Stats. Currently unimplemented.
typedef struct {
  uint64_t total_compressed_size;
  int max_tile_size;
  int min_cropped_tile_width;
  int min_cropped_tile_height;
  int tile_width_is_valid;
  double total_time_encoded;
  double min_cr;
} AV1LevelStats;

typedef struct {
  AV1LevelStats level_stats;
  AV1LevelSpec level_spec;
} AV1LevelInfo;

void av1_update_level_info(struct AV1_COMP *cpi, size_t size, int64_t ts_start,
                           int64_t ts_end);

// Return sequence level indices in seq_level_idx[MAX_NUM_OPERATING_POINTS].
aom_codec_err_t av1_get_seq_level_idx(const struct AV1_COMP *cpi,
                                      int *seq_level_idx);

#endif  // AOM_AV1_ENCODER_LEVEL_H_
