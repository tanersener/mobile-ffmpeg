/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/******************************************************************

 iLBC Speech Coder ANSI-C Source Code

 define.h

******************************************************************/
#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DEFINES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DEFINES_H_

#include <string.h>
#include "signal_processing_library.h"
#include "typedefs.h"
#include "ilbc.h"

/* cb settings */

#define CB_NSTAGES     3
#define CB_EXPAND     2
#define CB_MEML      147
#define CB_FILTERLEN    (2*4)
#define CB_HALFFILTERLEN   4
#define CB_RESRANGE     34
#define CB_MAXGAIN_FIXQ6   83 /* error = -0.24% */
#define CB_MAXGAIN_FIXQ14   21299

/* PLC */

/* Down sampling */

#define FILTERORDER_DS_PLUS1  7
#define DELAY_DS     3
#define FACTOR_DS     2

/* bit stream defs */

#define NO_OF_BYTES_20MS   38
#define NO_OF_BYTES_30MS   50
#define NO_OF_WORDS_20MS   19
#define NO_OF_WORDS_30MS   25
#define STATE_BITS     3
#define BYTE_LEN     8
#define ULP_CLASSES     3

/* help parameters */

#define TWO_PI_FIX     25736 /* Q12 */

/* Constants for codebook search and creation */

#define ST_MEM_L_TBL  85
#define MEM_LF_TBL  147


/* Struct for the bits */
typedef struct iLBC_bits_t_ {
  int16_t lsf[LSF_NSPLIT*LPC_N_MAX];
  int16_t cb_index[CB_NSTAGES*(NASUB_MAX+1)];  /* First CB_NSTAGES values contains extra CB index */
  int16_t gain_index[CB_NSTAGES*(NASUB_MAX+1)]; /* First CB_NSTAGES values contains extra CB gain */
  int16_t idxForMax;
  int16_t state_first;
  int16_t idxVec[STATE_SHORT_LEN_30MS];
  int16_t firstbits;
  int16_t startIdx;
} iLBC_bits;

#endif
