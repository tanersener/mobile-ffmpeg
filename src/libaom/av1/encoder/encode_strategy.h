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

#ifndef AOM_AV1_ENCODER_ENCODE_STRATEGY_H_
#define AOM_AV1_ENCODER_ENCODE_STRATEGY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "aom/aom_encoder.h"

#include "av1/encoder/encoder.h"
#include "av1/encoder/firstpass.h"

// This function will implement high-level encode strategy, choosing frame type,
// frame placement, etc.  It populates an EncodeFrameParams struct with the
// results of these decisions and then calls av1_encode()
int av1_encode_strategy(AV1_COMP *const cpi, size_t *const size,
                        uint8_t *const dest, unsigned int *frame_flags,
                        int64_t *const time_stamp, int64_t *const time_end,
                        const aom_rational_t *const timebase, int flush);

// Set individual buffer update flags based on frame reference type.
// force_refresh_all is used when we have a KEY_FRAME or S_FRAME.  It forces all
// refresh_*_frame flags to be set, because we refresh all buffers in this case.
void av1_configure_buffer_updates(AV1_COMP *const cpi,
                                  const FRAME_UPDATE_TYPE type,
                                  int force_refresh_all);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_ENCODER_ENCODE_STRATEGY_H_
