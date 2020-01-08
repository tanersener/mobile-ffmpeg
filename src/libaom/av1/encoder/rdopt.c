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

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "config/aom_dsp_rtcd.h"
#include "config/av1_rtcd.h"

#include "aom_dsp/aom_dsp_common.h"
#include "aom_dsp/blend.h"
#include "aom_mem/aom_mem.h"
#include "aom_ports/aom_timer.h"
#include "aom_ports/mem.h"
#include "aom_ports/system_state.h"

#include "av1/common/cfl.h"
#include "av1/common/common.h"
#include "av1/common/common_data.h"
#include "av1/common/entropy.h"
#include "av1/common/entropymode.h"
#include "av1/common/idct.h"
#include "av1/common/mvref_common.h"
#include "av1/common/obmc.h"
#include "av1/common/onyxc_int.h"
#include "av1/common/pred_common.h"
#include "av1/common/quant_common.h"
#include "av1/common/reconinter.h"
#include "av1/common/reconintra.h"
#include "av1/common/scan.h"
#include "av1/common/seg_common.h"
#include "av1/common/txb_common.h"
#include "av1/common/warped_motion.h"

#include "av1/encoder/aq_variance.h"
#include "av1/encoder/av1_quantize.h"
#include "av1/encoder/cost.h"
#include "av1/encoder/encodemb.h"
#include "av1/encoder/encodemv.h"
#include "av1/encoder/encoder.h"
#include "av1/encoder/encodetxb.h"
#include "av1/encoder/hybrid_fwd_txfm.h"
#include "av1/encoder/mcomp.h"
#include "av1/encoder/ml.h"
#include "av1/encoder/mode_prune_model_weights.h"
#include "av1/encoder/model_rd.h"
#include "av1/encoder/palette.h"
#include "av1/encoder/pustats.h"
#include "av1/encoder/random.h"
#include "av1/encoder/ratectrl.h"
#include "av1/encoder/rd.h"
#include "av1/encoder/rdopt.h"
#include "av1/encoder/rdopt_utils.h"
#include "av1/encoder/reconinter_enc.h"
#include "av1/encoder/tokenize.h"
#include "av1/encoder/tpl_model.h"
#include "av1/encoder/tx_prune_model_weights.h"

// Set this macro as 1 to collect data about tx size selection.
#define COLLECT_TX_SIZE_DATA 0

#if COLLECT_TX_SIZE_DATA
static const char av1_tx_size_data_output_file[] = "tx_size_data.txt";
#endif

#define DUAL_FILTER_SET_SIZE (SWITCHABLE_FILTERS * SWITCHABLE_FILTERS)
static const int_interpfilters filter_sets[DUAL_FILTER_SET_SIZE] = {
  { 0x00000000 }, { 0x00010000 }, { 0x00020000 },  // y = 0
  { 0x00000001 }, { 0x00010001 }, { 0x00020001 },  // y = 1
  { 0x00000002 }, { 0x00010002 }, { 0x00020002 },  // y = 2
};

typedef struct {
  PREDICTION_MODE mode;
  MV_REFERENCE_FRAME ref_frame[2];
} MODE_DEFINITION;

enum {
  FTXS_NONE = 0,
  FTXS_DCT_AND_1D_DCT_ONLY = 1 << 0,
  FTXS_DISABLE_TRELLIS_OPT = 1 << 1,
  FTXS_USE_TRANSFORM_DOMAIN = 1 << 2
} UENUM1BYTE(FAST_TX_SEARCH_MODE);

struct rdcost_block_args {
  const AV1_COMP *cpi;
  MACROBLOCK *x;
  ENTROPY_CONTEXT t_above[MAX_MIB_SIZE];
  ENTROPY_CONTEXT t_left[MAX_MIB_SIZE];
  RD_STATS rd_stats;
  int64_t this_rd;
  int64_t best_rd;
  int exit_early;
  int incomplete_exit;
  int use_fast_coef_costing;
  FAST_TX_SEARCH_MODE ftxs_mode;
  int skip_trellis;
};

// Structure to store the compound type related stats for best compound type
typedef struct {
  INTERINTER_COMPOUND_DATA best_compound_data;
  int64_t comp_best_model_rd;
  int best_compmode_interinter_cost;
} BEST_COMP_TYPE_STATS;

#define LAST_NEW_MV_INDEX 6
// This array defines the mapping from the enums in THR_MODES to the actual
// prediction modes and refrence frames
static const MODE_DEFINITION av1_mode_defs[MAX_MODES] = {
  { NEARESTMV, { LAST_FRAME, NONE_FRAME } },
  { NEARESTMV, { LAST2_FRAME, NONE_FRAME } },
  { NEARESTMV, { LAST3_FRAME, NONE_FRAME } },
  { NEARESTMV, { BWDREF_FRAME, NONE_FRAME } },
  { NEARESTMV, { ALTREF2_FRAME, NONE_FRAME } },
  { NEARESTMV, { ALTREF_FRAME, NONE_FRAME } },
  { NEARESTMV, { GOLDEN_FRAME, NONE_FRAME } },

  { NEWMV, { LAST_FRAME, NONE_FRAME } },
  { NEWMV, { LAST2_FRAME, NONE_FRAME } },
  { NEWMV, { LAST3_FRAME, NONE_FRAME } },
  { NEWMV, { BWDREF_FRAME, NONE_FRAME } },
  { NEWMV, { ALTREF2_FRAME, NONE_FRAME } },
  { NEWMV, { ALTREF_FRAME, NONE_FRAME } },
  { NEWMV, { GOLDEN_FRAME, NONE_FRAME } },

  { NEARMV, { LAST_FRAME, NONE_FRAME } },
  { NEARMV, { LAST2_FRAME, NONE_FRAME } },
  { NEARMV, { LAST3_FRAME, NONE_FRAME } },
  { NEARMV, { BWDREF_FRAME, NONE_FRAME } },
  { NEARMV, { ALTREF2_FRAME, NONE_FRAME } },
  { NEARMV, { ALTREF_FRAME, NONE_FRAME } },
  { NEARMV, { GOLDEN_FRAME, NONE_FRAME } },

  { GLOBALMV, { LAST_FRAME, NONE_FRAME } },
  { GLOBALMV, { LAST2_FRAME, NONE_FRAME } },
  { GLOBALMV, { LAST3_FRAME, NONE_FRAME } },
  { GLOBALMV, { BWDREF_FRAME, NONE_FRAME } },
  { GLOBALMV, { ALTREF2_FRAME, NONE_FRAME } },
  { GLOBALMV, { ALTREF_FRAME, NONE_FRAME } },
  { GLOBALMV, { GOLDEN_FRAME, NONE_FRAME } },

  // TODO(zoeliu): May need to reconsider the order on the modes to check

  { NEAREST_NEARESTMV, { LAST_FRAME, ALTREF_FRAME } },
  { NEAREST_NEARESTMV, { LAST2_FRAME, ALTREF_FRAME } },
  { NEAREST_NEARESTMV, { LAST3_FRAME, ALTREF_FRAME } },
  { NEAREST_NEARESTMV, { GOLDEN_FRAME, ALTREF_FRAME } },
  { NEAREST_NEARESTMV, { LAST_FRAME, BWDREF_FRAME } },
  { NEAREST_NEARESTMV, { LAST2_FRAME, BWDREF_FRAME } },
  { NEAREST_NEARESTMV, { LAST3_FRAME, BWDREF_FRAME } },
  { NEAREST_NEARESTMV, { GOLDEN_FRAME, BWDREF_FRAME } },
  { NEAREST_NEARESTMV, { LAST_FRAME, ALTREF2_FRAME } },
  { NEAREST_NEARESTMV, { LAST2_FRAME, ALTREF2_FRAME } },
  { NEAREST_NEARESTMV, { LAST3_FRAME, ALTREF2_FRAME } },
  { NEAREST_NEARESTMV, { GOLDEN_FRAME, ALTREF2_FRAME } },

  { NEAREST_NEARESTMV, { LAST_FRAME, LAST2_FRAME } },
  { NEAREST_NEARESTMV, { LAST_FRAME, LAST3_FRAME } },
  { NEAREST_NEARESTMV, { LAST_FRAME, GOLDEN_FRAME } },
  { NEAREST_NEARESTMV, { BWDREF_FRAME, ALTREF_FRAME } },

  { NEAR_NEARMV, { LAST_FRAME, ALTREF_FRAME } },
  { NEW_NEARESTMV, { LAST_FRAME, ALTREF_FRAME } },
  { NEAREST_NEWMV, { LAST_FRAME, ALTREF_FRAME } },
  { NEW_NEARMV, { LAST_FRAME, ALTREF_FRAME } },
  { NEAR_NEWMV, { LAST_FRAME, ALTREF_FRAME } },
  { NEW_NEWMV, { LAST_FRAME, ALTREF_FRAME } },
  { GLOBAL_GLOBALMV, { LAST_FRAME, ALTREF_FRAME } },

  { NEAR_NEARMV, { LAST2_FRAME, ALTREF_FRAME } },
  { NEW_NEARESTMV, { LAST2_FRAME, ALTREF_FRAME } },
  { NEAREST_NEWMV, { LAST2_FRAME, ALTREF_FRAME } },
  { NEW_NEARMV, { LAST2_FRAME, ALTREF_FRAME } },
  { NEAR_NEWMV, { LAST2_FRAME, ALTREF_FRAME } },
  { NEW_NEWMV, { LAST2_FRAME, ALTREF_FRAME } },
  { GLOBAL_GLOBALMV, { LAST2_FRAME, ALTREF_FRAME } },

  { NEAR_NEARMV, { LAST3_FRAME, ALTREF_FRAME } },
  { NEW_NEARESTMV, { LAST3_FRAME, ALTREF_FRAME } },
  { NEAREST_NEWMV, { LAST3_FRAME, ALTREF_FRAME } },
  { NEW_NEARMV, { LAST3_FRAME, ALTREF_FRAME } },
  { NEAR_NEWMV, { LAST3_FRAME, ALTREF_FRAME } },
  { NEW_NEWMV, { LAST3_FRAME, ALTREF_FRAME } },
  { GLOBAL_GLOBALMV, { LAST3_FRAME, ALTREF_FRAME } },

  { NEAR_NEARMV, { GOLDEN_FRAME, ALTREF_FRAME } },
  { NEW_NEARESTMV, { GOLDEN_FRAME, ALTREF_FRAME } },
  { NEAREST_NEWMV, { GOLDEN_FRAME, ALTREF_FRAME } },
  { NEW_NEARMV, { GOLDEN_FRAME, ALTREF_FRAME } },
  { NEAR_NEWMV, { GOLDEN_FRAME, ALTREF_FRAME } },
  { NEW_NEWMV, { GOLDEN_FRAME, ALTREF_FRAME } },
  { GLOBAL_GLOBALMV, { GOLDEN_FRAME, ALTREF_FRAME } },

  { NEAR_NEARMV, { LAST_FRAME, BWDREF_FRAME } },
  { NEW_NEARESTMV, { LAST_FRAME, BWDREF_FRAME } },
  { NEAREST_NEWMV, { LAST_FRAME, BWDREF_FRAME } },
  { NEW_NEARMV, { LAST_FRAME, BWDREF_FRAME } },
  { NEAR_NEWMV, { LAST_FRAME, BWDREF_FRAME } },
  { NEW_NEWMV, { LAST_FRAME, BWDREF_FRAME } },
  { GLOBAL_GLOBALMV, { LAST_FRAME, BWDREF_FRAME } },

  { NEAR_NEARMV, { LAST2_FRAME, BWDREF_FRAME } },
  { NEW_NEARESTMV, { LAST2_FRAME, BWDREF_FRAME } },
  { NEAREST_NEWMV, { LAST2_FRAME, BWDREF_FRAME } },
  { NEW_NEARMV, { LAST2_FRAME, BWDREF_FRAME } },
  { NEAR_NEWMV, { LAST2_FRAME, BWDREF_FRAME } },
  { NEW_NEWMV, { LAST2_FRAME, BWDREF_FRAME } },
  { GLOBAL_GLOBALMV, { LAST2_FRAME, BWDREF_FRAME } },

  { NEAR_NEARMV, { LAST3_FRAME, BWDREF_FRAME } },
  { NEW_NEARESTMV, { LAST3_FRAME, BWDREF_FRAME } },
  { NEAREST_NEWMV, { LAST3_FRAME, BWDREF_FRAME } },
  { NEW_NEARMV, { LAST3_FRAME, BWDREF_FRAME } },
  { NEAR_NEWMV, { LAST3_FRAME, BWDREF_FRAME } },
  { NEW_NEWMV, { LAST3_FRAME, BWDREF_FRAME } },
  { GLOBAL_GLOBALMV, { LAST3_FRAME, BWDREF_FRAME } },

  { NEAR_NEARMV, { GOLDEN_FRAME, BWDREF_FRAME } },
  { NEW_NEARESTMV, { GOLDEN_FRAME, BWDREF_FRAME } },
  { NEAREST_NEWMV, { GOLDEN_FRAME, BWDREF_FRAME } },
  { NEW_NEARMV, { GOLDEN_FRAME, BWDREF_FRAME } },
  { NEAR_NEWMV, { GOLDEN_FRAME, BWDREF_FRAME } },
  { NEW_NEWMV, { GOLDEN_FRAME, BWDREF_FRAME } },
  { GLOBAL_GLOBALMV, { GOLDEN_FRAME, BWDREF_FRAME } },

  { NEAR_NEARMV, { LAST_FRAME, ALTREF2_FRAME } },
  { NEW_NEARESTMV, { LAST_FRAME, ALTREF2_FRAME } },
  { NEAREST_NEWMV, { LAST_FRAME, ALTREF2_FRAME } },
  { NEW_NEARMV, { LAST_FRAME, ALTREF2_FRAME } },
  { NEAR_NEWMV, { LAST_FRAME, ALTREF2_FRAME } },
  { NEW_NEWMV, { LAST_FRAME, ALTREF2_FRAME } },
  { GLOBAL_GLOBALMV, { LAST_FRAME, ALTREF2_FRAME } },

  { NEAR_NEARMV, { LAST2_FRAME, ALTREF2_FRAME } },
  { NEW_NEARESTMV, { LAST2_FRAME, ALTREF2_FRAME } },
  { NEAREST_NEWMV, { LAST2_FRAME, ALTREF2_FRAME } },
  { NEW_NEARMV, { LAST2_FRAME, ALTREF2_FRAME } },
  { NEAR_NEWMV, { LAST2_FRAME, ALTREF2_FRAME } },
  { NEW_NEWMV, { LAST2_FRAME, ALTREF2_FRAME } },
  { GLOBAL_GLOBALMV, { LAST2_FRAME, ALTREF2_FRAME } },

  { NEAR_NEARMV, { LAST3_FRAME, ALTREF2_FRAME } },
  { NEW_NEARESTMV, { LAST3_FRAME, ALTREF2_FRAME } },
  { NEAREST_NEWMV, { LAST3_FRAME, ALTREF2_FRAME } },
  { NEW_NEARMV, { LAST3_FRAME, ALTREF2_FRAME } },
  { NEAR_NEWMV, { LAST3_FRAME, ALTREF2_FRAME } },
  { NEW_NEWMV, { LAST3_FRAME, ALTREF2_FRAME } },
  { GLOBAL_GLOBALMV, { LAST3_FRAME, ALTREF2_FRAME } },

  { NEAR_NEARMV, { GOLDEN_FRAME, ALTREF2_FRAME } },
  { NEW_NEARESTMV, { GOLDEN_FRAME, ALTREF2_FRAME } },
  { NEAREST_NEWMV, { GOLDEN_FRAME, ALTREF2_FRAME } },
  { NEW_NEARMV, { GOLDEN_FRAME, ALTREF2_FRAME } },
  { NEAR_NEWMV, { GOLDEN_FRAME, ALTREF2_FRAME } },
  { NEW_NEWMV, { GOLDEN_FRAME, ALTREF2_FRAME } },
  { GLOBAL_GLOBALMV, { GOLDEN_FRAME, ALTREF2_FRAME } },

  { NEAR_NEARMV, { LAST_FRAME, LAST2_FRAME } },
  { NEW_NEARESTMV, { LAST_FRAME, LAST2_FRAME } },
  { NEAREST_NEWMV, { LAST_FRAME, LAST2_FRAME } },
  { NEW_NEARMV, { LAST_FRAME, LAST2_FRAME } },
  { NEAR_NEWMV, { LAST_FRAME, LAST2_FRAME } },
  { NEW_NEWMV, { LAST_FRAME, LAST2_FRAME } },
  { GLOBAL_GLOBALMV, { LAST_FRAME, LAST2_FRAME } },

  { NEAR_NEARMV, { LAST_FRAME, LAST3_FRAME } },
  { NEW_NEARESTMV, { LAST_FRAME, LAST3_FRAME } },
  { NEAREST_NEWMV, { LAST_FRAME, LAST3_FRAME } },
  { NEW_NEARMV, { LAST_FRAME, LAST3_FRAME } },
  { NEAR_NEWMV, { LAST_FRAME, LAST3_FRAME } },
  { NEW_NEWMV, { LAST_FRAME, LAST3_FRAME } },
  { GLOBAL_GLOBALMV, { LAST_FRAME, LAST3_FRAME } },

  { NEAR_NEARMV, { LAST_FRAME, GOLDEN_FRAME } },
  { NEW_NEARESTMV, { LAST_FRAME, GOLDEN_FRAME } },
  { NEAREST_NEWMV, { LAST_FRAME, GOLDEN_FRAME } },
  { NEW_NEARMV, { LAST_FRAME, GOLDEN_FRAME } },
  { NEAR_NEWMV, { LAST_FRAME, GOLDEN_FRAME } },
  { NEW_NEWMV, { LAST_FRAME, GOLDEN_FRAME } },
  { GLOBAL_GLOBALMV, { LAST_FRAME, GOLDEN_FRAME } },

  { NEAR_NEARMV, { BWDREF_FRAME, ALTREF_FRAME } },
  { NEW_NEARESTMV, { BWDREF_FRAME, ALTREF_FRAME } },
  { NEAREST_NEWMV, { BWDREF_FRAME, ALTREF_FRAME } },
  { NEW_NEARMV, { BWDREF_FRAME, ALTREF_FRAME } },
  { NEAR_NEWMV, { BWDREF_FRAME, ALTREF_FRAME } },
  { NEW_NEWMV, { BWDREF_FRAME, ALTREF_FRAME } },
  { GLOBAL_GLOBALMV, { BWDREF_FRAME, ALTREF_FRAME } },

  // intra modes
  { DC_PRED, { INTRA_FRAME, NONE_FRAME } },
  { PAETH_PRED, { INTRA_FRAME, NONE_FRAME } },
  { SMOOTH_PRED, { INTRA_FRAME, NONE_FRAME } },
  { SMOOTH_V_PRED, { INTRA_FRAME, NONE_FRAME } },
  { SMOOTH_H_PRED, { INTRA_FRAME, NONE_FRAME } },
  { H_PRED, { INTRA_FRAME, NONE_FRAME } },
  { V_PRED, { INTRA_FRAME, NONE_FRAME } },
  { D135_PRED, { INTRA_FRAME, NONE_FRAME } },
  { D203_PRED, { INTRA_FRAME, NONE_FRAME } },
  { D157_PRED, { INTRA_FRAME, NONE_FRAME } },
  { D67_PRED, { INTRA_FRAME, NONE_FRAME } },
  { D113_PRED, { INTRA_FRAME, NONE_FRAME } },
  { D45_PRED, { INTRA_FRAME, NONE_FRAME } },
};

static const THR_MODES av1_default_mode_order[MAX_MODES] = {
  THR_NEARESTMV,
  THR_NEARESTL2,
  THR_NEARESTL3,
  THR_NEARESTB,
  THR_NEARESTA2,
  THR_NEARESTA,
  THR_NEARESTG,

  THR_NEWMV,
  THR_NEWL2,
  THR_NEWL3,
  THR_NEWB,
  THR_NEWA2,
  THR_NEWA,
  THR_NEWG,

  THR_NEARMV,
  THR_NEARL2,
  THR_NEARL3,
  THR_NEARB,
  THR_NEARA2,
  THR_NEARA,
  THR_NEARG,

  THR_GLOBALMV,
  THR_GLOBALL2,
  THR_GLOBALL3,
  THR_GLOBALB,
  THR_GLOBALA2,
  THR_GLOBALA,
  THR_GLOBALG,

  THR_COMP_NEAREST_NEARESTLA,
  THR_COMP_NEAREST_NEARESTL2A,
  THR_COMP_NEAREST_NEARESTL3A,
  THR_COMP_NEAREST_NEARESTGA,
  THR_COMP_NEAREST_NEARESTLB,
  THR_COMP_NEAREST_NEARESTL2B,
  THR_COMP_NEAREST_NEARESTL3B,
  THR_COMP_NEAREST_NEARESTGB,
  THR_COMP_NEAREST_NEARESTLA2,
  THR_COMP_NEAREST_NEARESTL2A2,
  THR_COMP_NEAREST_NEARESTL3A2,
  THR_COMP_NEAREST_NEARESTGA2,
  THR_COMP_NEAREST_NEARESTLL2,
  THR_COMP_NEAREST_NEARESTLL3,
  THR_COMP_NEAREST_NEARESTLG,
  THR_COMP_NEAREST_NEARESTBA,

  THR_COMP_NEAR_NEARLA,
  THR_COMP_NEW_NEARESTLA,
  THR_COMP_NEAREST_NEWLA,
  THR_COMP_NEW_NEARLA,
  THR_COMP_NEAR_NEWLA,
  THR_COMP_NEW_NEWLA,
  THR_COMP_GLOBAL_GLOBALLA,

  THR_COMP_NEAR_NEARL2A,
  THR_COMP_NEW_NEARESTL2A,
  THR_COMP_NEAREST_NEWL2A,
  THR_COMP_NEW_NEARL2A,
  THR_COMP_NEAR_NEWL2A,
  THR_COMP_NEW_NEWL2A,
  THR_COMP_GLOBAL_GLOBALL2A,

  THR_COMP_NEAR_NEARL3A,
  THR_COMP_NEW_NEARESTL3A,
  THR_COMP_NEAREST_NEWL3A,
  THR_COMP_NEW_NEARL3A,
  THR_COMP_NEAR_NEWL3A,
  THR_COMP_NEW_NEWL3A,
  THR_COMP_GLOBAL_GLOBALL3A,

  THR_COMP_NEAR_NEARGA,
  THR_COMP_NEW_NEARESTGA,
  THR_COMP_NEAREST_NEWGA,
  THR_COMP_NEW_NEARGA,
  THR_COMP_NEAR_NEWGA,
  THR_COMP_NEW_NEWGA,
  THR_COMP_GLOBAL_GLOBALGA,

  THR_COMP_NEAR_NEARLB,
  THR_COMP_NEW_NEARESTLB,
  THR_COMP_NEAREST_NEWLB,
  THR_COMP_NEW_NEARLB,
  THR_COMP_NEAR_NEWLB,
  THR_COMP_NEW_NEWLB,
  THR_COMP_GLOBAL_GLOBALLB,

  THR_COMP_NEAR_NEARL2B,
  THR_COMP_NEW_NEARESTL2B,
  THR_COMP_NEAREST_NEWL2B,
  THR_COMP_NEW_NEARL2B,
  THR_COMP_NEAR_NEWL2B,
  THR_COMP_NEW_NEWL2B,
  THR_COMP_GLOBAL_GLOBALL2B,

  THR_COMP_NEAR_NEARL3B,
  THR_COMP_NEW_NEARESTL3B,
  THR_COMP_NEAREST_NEWL3B,
  THR_COMP_NEW_NEARL3B,
  THR_COMP_NEAR_NEWL3B,
  THR_COMP_NEW_NEWL3B,
  THR_COMP_GLOBAL_GLOBALL3B,

  THR_COMP_NEAR_NEARGB,
  THR_COMP_NEW_NEARESTGB,
  THR_COMP_NEAREST_NEWGB,
  THR_COMP_NEW_NEARGB,
  THR_COMP_NEAR_NEWGB,
  THR_COMP_NEW_NEWGB,
  THR_COMP_GLOBAL_GLOBALGB,

  THR_COMP_NEAR_NEARLA2,
  THR_COMP_NEW_NEARESTLA2,
  THR_COMP_NEAREST_NEWLA2,
  THR_COMP_NEW_NEARLA2,
  THR_COMP_NEAR_NEWLA2,
  THR_COMP_NEW_NEWLA2,
  THR_COMP_GLOBAL_GLOBALLA2,

  THR_COMP_NEAR_NEARL2A2,
  THR_COMP_NEW_NEARESTL2A2,
  THR_COMP_NEAREST_NEWL2A2,
  THR_COMP_NEW_NEARL2A2,
  THR_COMP_NEAR_NEWL2A2,
  THR_COMP_NEW_NEWL2A2,
  THR_COMP_GLOBAL_GLOBALL2A2,

  THR_COMP_NEAR_NEARL3A2,
  THR_COMP_NEW_NEARESTL3A2,
  THR_COMP_NEAREST_NEWL3A2,
  THR_COMP_NEW_NEARL3A2,
  THR_COMP_NEAR_NEWL3A2,
  THR_COMP_NEW_NEWL3A2,
  THR_COMP_GLOBAL_GLOBALL3A2,

  THR_COMP_NEAR_NEARGA2,
  THR_COMP_NEW_NEARESTGA2,
  THR_COMP_NEAREST_NEWGA2,
  THR_COMP_NEW_NEARGA2,
  THR_COMP_NEAR_NEWGA2,
  THR_COMP_NEW_NEWGA2,
  THR_COMP_GLOBAL_GLOBALGA2,

  THR_COMP_NEAR_NEARLL2,
  THR_COMP_NEW_NEARESTLL2,
  THR_COMP_NEAREST_NEWLL2,
  THR_COMP_NEW_NEARLL2,
  THR_COMP_NEAR_NEWLL2,
  THR_COMP_NEW_NEWLL2,
  THR_COMP_GLOBAL_GLOBALLL2,

  THR_COMP_NEAR_NEARLL3,
  THR_COMP_NEW_NEARESTLL3,
  THR_COMP_NEAREST_NEWLL3,
  THR_COMP_NEW_NEARLL3,
  THR_COMP_NEAR_NEWLL3,
  THR_COMP_NEW_NEWLL3,
  THR_COMP_GLOBAL_GLOBALLL3,

  THR_COMP_NEAR_NEARLG,
  THR_COMP_NEW_NEARESTLG,
  THR_COMP_NEAREST_NEWLG,
  THR_COMP_NEW_NEARLG,
  THR_COMP_NEAR_NEWLG,
  THR_COMP_NEW_NEWLG,
  THR_COMP_GLOBAL_GLOBALLG,

  THR_COMP_NEAR_NEARBA,
  THR_COMP_NEW_NEARESTBA,
  THR_COMP_NEAREST_NEWBA,
  THR_COMP_NEW_NEARBA,
  THR_COMP_NEAR_NEWBA,
  THR_COMP_NEW_NEWBA,
  THR_COMP_GLOBAL_GLOBALBA,

  THR_DC,
  THR_PAETH,
  THR_SMOOTH,
  THR_SMOOTH_V,
  THR_SMOOTH_H,
  THR_H_PRED,
  THR_V_PRED,
  THR_D135_PRED,
  THR_D203_PRED,
  THR_D157_PRED,
  THR_D67_PRED,
  THR_D113_PRED,
  THR_D45_PRED,
};

static int find_last_single_ref_mode_idx(const THR_MODES *mode_order) {
  uint8_t mode_found[NUM_SINGLE_REF_MODES];
  av1_zero(mode_found);
  int num_single_ref_modes_left = NUM_SINGLE_REF_MODES;

  for (int idx = 0; idx < MAX_MODES; idx++) {
    const THR_MODES curr_mode = mode_order[idx];
    if (curr_mode < SINGLE_REF_MODE_END) {
      num_single_ref_modes_left--;
    }
    if (!num_single_ref_modes_left) {
      return idx;
    }
  }
  return -1;
}

static const PREDICTION_MODE intra_rd_search_mode_order[INTRA_MODES] = {
  DC_PRED,       H_PRED,        V_PRED,    SMOOTH_PRED, PAETH_PRED,
  SMOOTH_V_PRED, SMOOTH_H_PRED, D135_PRED, D203_PRED,   D157_PRED,
  D67_PRED,      D113_PRED,     D45_PRED,
};

static const UV_PREDICTION_MODE uv_rd_search_mode_order[UV_INTRA_MODES] = {
  UV_DC_PRED,     UV_CFL_PRED,   UV_H_PRED,        UV_V_PRED,
  UV_SMOOTH_PRED, UV_PAETH_PRED, UV_SMOOTH_V_PRED, UV_SMOOTH_H_PRED,
  UV_D135_PRED,   UV_D203_PRED,  UV_D157_PRED,     UV_D67_PRED,
  UV_D113_PRED,   UV_D45_PRED,
};

typedef struct SingleInterModeState {
  int64_t rd;
  MV_REFERENCE_FRAME ref_frame;
  int valid;
} SingleInterModeState;

typedef struct InterModeSearchState {
  int64_t best_rd;
  MB_MODE_INFO best_mbmode;
  int best_rate_y;
  int best_rate_uv;
  int best_mode_skippable;
  int best_skip2;
  THR_MODES best_mode_index;
  int skip_intra_modes;
  int num_available_refs;
  int64_t dist_refs[REF_FRAMES];
  int dist_order_refs[REF_FRAMES];
  int64_t mode_threshold[MAX_MODES];
  PREDICTION_MODE best_intra_mode;
  int64_t best_intra_rd;
  int angle_stats_ready;
  uint8_t directional_mode_skip_mask[INTRA_MODES];
  unsigned int best_pred_sse;
  int rate_uv_intra;
  int rate_uv_tokenonly;
  int64_t dist_uvs;
  int skip_uvs;
  UV_PREDICTION_MODE mode_uv;
  PALETTE_MODE_INFO pmi_uv;
  int8_t uv_angle_delta;
  int64_t best_pred_rd[REFERENCE_MODES];
  int64_t best_pred_diff[REFERENCE_MODES];
  // Save a set of single_newmv for each checked ref_mv.
  int_mv single_newmv[MAX_REF_MV_SEARCH][REF_FRAMES];
  int single_newmv_rate[MAX_REF_MV_SEARCH][REF_FRAMES];
  int single_newmv_valid[MAX_REF_MV_SEARCH][REF_FRAMES];
  int64_t modelled_rd[MB_MODE_COUNT][MAX_REF_MV_SEARCH][REF_FRAMES];
  // The rd of simple translation in single inter modes
  int64_t simple_rd[MB_MODE_COUNT][MAX_REF_MV_SEARCH][REF_FRAMES];

  // Single search results by [directions][modes][reference frames]
  SingleInterModeState single_state[2][SINGLE_INTER_MODE_NUM][FWD_REFS];
  int single_state_cnt[2][SINGLE_INTER_MODE_NUM];
  SingleInterModeState single_state_modelled[2][SINGLE_INTER_MODE_NUM]
                                            [FWD_REFS];
  int single_state_modelled_cnt[2][SINGLE_INTER_MODE_NUM];
  MV_REFERENCE_FRAME single_rd_order[2][SINGLE_INTER_MODE_NUM][FWD_REFS];
} InterModeSearchState;

static void alloc_compound_type_rd_buffers_no_check(
    CompoundTypeRdBuffers *const bufs) {
  bufs->pred0 =
      (uint8_t *)aom_memalign(16, 2 * MAX_SB_SQUARE * sizeof(*bufs->pred0));
  bufs->pred1 =
      (uint8_t *)aom_memalign(16, 2 * MAX_SB_SQUARE * sizeof(*bufs->pred1));
  bufs->residual1 =
      (int16_t *)aom_memalign(32, MAX_SB_SQUARE * sizeof(*bufs->residual1));
  bufs->diff10 =
      (int16_t *)aom_memalign(32, MAX_SB_SQUARE * sizeof(*bufs->diff10));
  bufs->tmp_best_mask_buf = (uint8_t *)aom_malloc(
      2 * MAX_SB_SQUARE * sizeof(*bufs->tmp_best_mask_buf));
}

void av1_inter_mode_data_init(TileDataEnc *tile_data) {
  for (int i = 0; i < BLOCK_SIZES_ALL; ++i) {
    InterModeRdModel *md = &tile_data->inter_mode_rd_models[i];
    md->ready = 0;
    md->num = 0;
    md->dist_sum = 0;
    md->ld_sum = 0;
    md->sse_sum = 0;
    md->sse_sse_sum = 0;
    md->sse_ld_sum = 0;
  }
}

static int get_est_rate_dist(const TileDataEnc *tile_data, BLOCK_SIZE bsize,
                             int64_t sse, int *est_residue_cost,
                             int64_t *est_dist) {
  aom_clear_system_state();
  const InterModeRdModel *md = &tile_data->inter_mode_rd_models[bsize];
  if (md->ready) {
    if (sse < md->dist_mean) {
      *est_residue_cost = 0;
      *est_dist = sse;
    } else {
      *est_dist = (int64_t)round(md->dist_mean);
      const double est_ld = md->a * sse + md->b;
      // Clamp estimated rate cost by INT_MAX / 2.
      // TODO(angiebird@google.com): find better solution than clamping.
      if (fabs(est_ld) < 1e-2) {
        *est_residue_cost = INT_MAX / 2;
      } else {
        double est_residue_cost_dbl = ((sse - md->dist_mean) / est_ld);
        if (est_residue_cost_dbl < 0) {
          *est_residue_cost = 0;
        } else {
          *est_residue_cost =
              (int)AOMMIN((int64_t)round(est_residue_cost_dbl), INT_MAX / 2);
        }
      }
      if (*est_residue_cost <= 0) {
        *est_residue_cost = 0;
        *est_dist = sse;
      }
    }
    return 1;
  }
  return 0;
}

void av1_inter_mode_data_fit(TileDataEnc *tile_data, int rdmult) {
  aom_clear_system_state();
  for (int bsize = 0; bsize < BLOCK_SIZES_ALL; ++bsize) {
    const int block_idx = inter_mode_data_block_idx(bsize);
    InterModeRdModel *md = &tile_data->inter_mode_rd_models[bsize];
    if (block_idx == -1) continue;
    if ((md->ready == 0 && md->num < 200) || (md->ready == 1 && md->num < 64)) {
      continue;
    } else {
      if (md->ready == 0) {
        md->dist_mean = md->dist_sum / md->num;
        md->ld_mean = md->ld_sum / md->num;
        md->sse_mean = md->sse_sum / md->num;
        md->sse_sse_mean = md->sse_sse_sum / md->num;
        md->sse_ld_mean = md->sse_ld_sum / md->num;
      } else {
        const double factor = 3;
        md->dist_mean =
            (md->dist_mean * factor + (md->dist_sum / md->num)) / (factor + 1);
        md->ld_mean =
            (md->ld_mean * factor + (md->ld_sum / md->num)) / (factor + 1);
        md->sse_mean =
            (md->sse_mean * factor + (md->sse_sum / md->num)) / (factor + 1);
        md->sse_sse_mean =
            (md->sse_sse_mean * factor + (md->sse_sse_sum / md->num)) /
            (factor + 1);
        md->sse_ld_mean =
            (md->sse_ld_mean * factor + (md->sse_ld_sum / md->num)) /
            (factor + 1);
      }

      const double my = md->ld_mean;
      const double mx = md->sse_mean;
      const double dx = sqrt(md->sse_sse_mean);
      const double dxy = md->sse_ld_mean;

      md->a = (dxy - mx * my) / (dx * dx - mx * mx);
      md->b = my - md->a * mx;
      md->ready = 1;

      md->num = 0;
      md->dist_sum = 0;
      md->ld_sum = 0;
      md->sse_sum = 0;
      md->sse_sse_sum = 0;
      md->sse_ld_sum = 0;
    }
    (void)rdmult;
  }
}

static AOM_INLINE void inter_mode_data_push(TileDataEnc *tile_data,
                                            BLOCK_SIZE bsize, int64_t sse,
                                            int64_t dist, int residue_cost) {
  if (residue_cost == 0 || sse == dist) return;
  const int block_idx = inter_mode_data_block_idx(bsize);
  if (block_idx == -1) return;
  InterModeRdModel *rd_model = &tile_data->inter_mode_rd_models[bsize];
  if (rd_model->num < INTER_MODE_RD_DATA_OVERALL_SIZE) {
    aom_clear_system_state();
    const double ld = (sse - dist) * 1. / residue_cost;
    ++rd_model->num;
    rd_model->dist_sum += dist;
    rd_model->ld_sum += ld;
    rd_model->sse_sum += sse;
    rd_model->sse_sse_sum += (double)sse * (double)sse;
    rd_model->sse_ld_sum += sse * ld;
  }
}

static AOM_INLINE void inter_modes_info_push(InterModesInfo *inter_modes_info,
                                             int mode_rate, int64_t sse,
                                             int64_t rd, RD_STATS *rd_cost,
                                             RD_STATS *rd_cost_y,
                                             RD_STATS *rd_cost_uv,
                                             const MB_MODE_INFO *mbmi) {
  const int num = inter_modes_info->num;
  assert(num < MAX_INTER_MODES);
  inter_modes_info->mbmi_arr[num] = *mbmi;
  inter_modes_info->mode_rate_arr[num] = mode_rate;
  inter_modes_info->sse_arr[num] = sse;
  inter_modes_info->est_rd_arr[num] = rd;
  inter_modes_info->rd_cost_arr[num] = *rd_cost;
  inter_modes_info->rd_cost_y_arr[num] = *rd_cost_y;
  inter_modes_info->rd_cost_uv_arr[num] = *rd_cost_uv;
  ++inter_modes_info->num;
}

static int compare_rd_idx_pair(const void *a, const void *b) {
  if (((RdIdxPair *)a)->rd == ((RdIdxPair *)b)->rd) {
    return 0;
  } else if (((const RdIdxPair *)a)->rd > ((const RdIdxPair *)b)->rd) {
    return 1;
  } else {
    return -1;
  }
}

static AOM_INLINE void inter_modes_info_sort(
    const InterModesInfo *inter_modes_info, RdIdxPair *rd_idx_pair_arr) {
  if (inter_modes_info->num == 0) {
    return;
  }
  for (int i = 0; i < inter_modes_info->num; ++i) {
    rd_idx_pair_arr[i].idx = i;
    rd_idx_pair_arr[i].rd = inter_modes_info->est_rd_arr[i];
  }
  qsort(rd_idx_pair_arr, inter_modes_info->num, sizeof(rd_idx_pair_arr[0]),
        compare_rd_idx_pair);
}

static INLINE int write_uniform_cost(int n, int v) {
  const int l = get_unsigned_bits(n);
  const int m = (1 << l) - n;
  if (l == 0) return 0;
  if (v < m)
    return av1_cost_literal(l - 1);
  else
    return av1_cost_literal(l);
}

// Similar to store_cfl_required(), but for use during the RDO process,
// where we haven't yet determined whether this block uses CfL.
static INLINE CFL_ALLOWED_TYPE store_cfl_required_rdo(const AV1_COMMON *cm,
                                                      const MACROBLOCK *x) {
  const MACROBLOCKD *xd = &x->e_mbd;

  if (cm->seq_params.monochrome || x->skip_chroma_rd) return CFL_DISALLOWED;

  if (!xd->cfl.is_chroma_reference) {
    // For non-chroma-reference blocks, we should always store the luma pixels,
    // in case the corresponding chroma-reference block uses CfL.
    // Note that this can only happen for block sizes which are <8 on
    // their shortest side, as otherwise they would be chroma reference
    // blocks.
    return CFL_ALLOWED;
  }

  // For chroma reference blocks, we should store data in the encoder iff we're
  // allowed to try out CfL.
  return is_cfl_allowed(xd);
}

static int inter_block_yrd(const AV1_COMP *cpi, MACROBLOCK *x,
                           RD_STATS *rd_stats, BLOCK_SIZE bsize,
                           int64_t ref_best_rd, FAST_TX_SEARCH_MODE ftxs_mode);

static unsigned pixel_dist_visible_only(
    const AV1_COMP *const cpi, const MACROBLOCK *x, const uint8_t *src,
    const int src_stride, const uint8_t *dst, const int dst_stride,
    const BLOCK_SIZE tx_bsize, int txb_rows, int txb_cols, int visible_rows,
    int visible_cols) {
  unsigned sse;

  if (txb_rows == visible_rows && txb_cols == visible_cols) {
    cpi->fn_ptr[tx_bsize].vf(src, src_stride, dst, dst_stride, &sse);
    return sse;
  }

#if CONFIG_AV1_HIGHBITDEPTH
  const MACROBLOCKD *xd = &x->e_mbd;
  if (is_cur_buf_hbd(xd)) {
    uint64_t sse64 = aom_highbd_sse_odd_size(src, src_stride, dst, dst_stride,
                                             visible_cols, visible_rows);
    return (unsigned int)ROUND_POWER_OF_TWO(sse64, (xd->bd - 8) * 2);
  }
#else
  (void)x;
#endif
  sse = aom_sse_odd_size(src, src_stride, dst, dst_stride, visible_cols,
                         visible_rows);
  return sse;
}

#if CONFIG_DIST_8X8
static uint64_t cdef_dist_8x8_16bit(uint16_t *dst, int dstride, uint16_t *src,
                                    int sstride, int coeff_shift) {
  uint64_t svar = 0;
  uint64_t dvar = 0;
  uint64_t sum_s = 0;
  uint64_t sum_d = 0;
  uint64_t sum_s2 = 0;
  uint64_t sum_d2 = 0;
  uint64_t sum_sd = 0;
  uint64_t dist = 0;

  int i, j;
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      sum_s += src[i * sstride + j];
      sum_d += dst[i * dstride + j];
      sum_s2 += src[i * sstride + j] * src[i * sstride + j];
      sum_d2 += dst[i * dstride + j] * dst[i * dstride + j];
      sum_sd += src[i * sstride + j] * dst[i * dstride + j];
    }
  }
  /* Compute the variance -- the calculation cannot go negative. */
  svar = sum_s2 - ((sum_s * sum_s + 32) >> 6);
  dvar = sum_d2 - ((sum_d * sum_d + 32) >> 6);

  // Tuning of jm's original dering distortion metric used in CDEF tool,
  // suggested by jm
  const uint64_t a = 4;
  const uint64_t b = 2;
  const uint64_t c1 = (400 * a << 2 * coeff_shift);
  const uint64_t c2 = (b * 20000 * a * a << 4 * coeff_shift);

  dist = (uint64_t)floor(.5 + (sum_d2 + sum_s2 - 2 * sum_sd) * .5 *
                                  (svar + dvar + c1) /
                                  (sqrt(svar * (double)dvar + c2)));

  // Calibrate dist to have similar rate for the same QP with MSE only
  // distortion (as in master branch)
  dist = (uint64_t)((float)dist * 0.75);

  return dist;
}

static int od_compute_var_4x4(uint16_t *x, int stride) {
  int sum;
  int s2;
  int i;
  sum = 0;
  s2 = 0;
  for (i = 0; i < 4; i++) {
    int j;
    for (j = 0; j < 4; j++) {
      int t;

      t = x[i * stride + j];
      sum += t;
      s2 += t * t;
    }
  }

  return (s2 - (sum * sum >> 4)) >> 4;
}

/* OD_DIST_LP_MID controls the frequency weighting filter used for computing
   the distortion. For a value X, the filter is [1 X 1]/(X + 2) and
   is applied both horizontally and vertically. For X=5, the filter is
   a good approximation for the OD_QM8_Q4_HVS quantization matrix. */
#define OD_DIST_LP_MID (5)
#define OD_DIST_LP_NORM (OD_DIST_LP_MID + 2)

static double od_compute_dist_8x8(int use_activity_masking, uint16_t *x,
                                  uint16_t *y, od_coeff *e_lp, int stride) {
  double sum;
  int min_var;
  double mean_var;
  double var_stat;
  double activity;
  double calibration;
  int i;
  int j;
  double vardist;

  vardist = 0;

#if 1
  min_var = INT_MAX;
  mean_var = 0;
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      int varx;
      int vary;
      varx = od_compute_var_4x4(x + 2 * i * stride + 2 * j, stride);
      vary = od_compute_var_4x4(y + 2 * i * stride + 2 * j, stride);
      min_var = OD_MINI(min_var, varx);
      mean_var += 1. / (1 + varx);
      /* The cast to (double) is to avoid an overflow before the sqrt.*/
      vardist += varx - 2 * sqrt(varx * (double)vary) + vary;
    }
  }
  /* We use a different variance statistic depending on whether activity
     masking is used, since the harmonic mean appeared slightly worse with
     masking off. The calibration constant just ensures that we preserve the
     rate compared to activity=1. */
  if (use_activity_masking) {
    calibration = 1.95;
    var_stat = 9. / mean_var;
  } else {
    calibration = 1.62;
    var_stat = min_var;
  }
  /* 1.62 is a calibration constant, 0.25 is a noise floor and 1/6 is the
     activity masking constant. */
  activity = calibration * pow(.25 + var_stat, -1. / 6);
#else
  activity = 1;
#endif  // 1
  sum = 0;
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++)
      sum += e_lp[i * stride + j] * (double)e_lp[i * stride + j];
  }
  /* Normalize the filter to unit DC response. */
  sum *= 1. / (OD_DIST_LP_NORM * OD_DIST_LP_NORM * OD_DIST_LP_NORM *
               OD_DIST_LP_NORM);
  return activity * activity * (sum + vardist);
}

// Note : Inputs x and y are in a pixel domain
static double od_compute_dist_common(int activity_masking, uint16_t *x,
                                     uint16_t *y, int bsize_w, int bsize_h,
                                     int qindex, od_coeff *tmp,
                                     od_coeff *e_lp) {
  int i, j;
  double sum = 0;
  const int mid = OD_DIST_LP_MID;

  for (j = 0; j < bsize_w; j++) {
    e_lp[j] = mid * tmp[j] + 2 * tmp[bsize_w + j];
    e_lp[(bsize_h - 1) * bsize_w + j] = mid * tmp[(bsize_h - 1) * bsize_w + j] +
                                        2 * tmp[(bsize_h - 2) * bsize_w + j];
  }
  for (i = 1; i < bsize_h - 1; i++) {
    for (j = 0; j < bsize_w; j++) {
      e_lp[i * bsize_w + j] = mid * tmp[i * bsize_w + j] +
                              tmp[(i - 1) * bsize_w + j] +
                              tmp[(i + 1) * bsize_w + j];
    }
  }
  for (i = 0; i < bsize_h; i += 8) {
    for (j = 0; j < bsize_w; j += 8) {
      sum += od_compute_dist_8x8(activity_masking, &x[i * bsize_w + j],
                                 &y[i * bsize_w + j], &e_lp[i * bsize_w + j],
                                 bsize_w);
    }
  }
  /* Scale according to linear regression against SSE, for 8x8 blocks. */
  if (activity_masking) {
    sum *= 2.2 + (1.7 - 2.2) * (qindex - 99) / (210 - 99) +
           (qindex < 99 ? 2.5 * (qindex - 99) / 99 * (qindex - 99) / 99 : 0);
  } else {
    sum *= qindex >= 128
               ? 1.4 + (0.9 - 1.4) * (qindex - 128) / (209 - 128)
               : qindex <= 43 ? 1.5 + (2.0 - 1.5) * (qindex - 43) / (16 - 43)
                              : 1.5 + (1.4 - 1.5) * (qindex - 43) / (128 - 43);
  }

  return sum;
}

static double od_compute_dist(uint16_t *x, uint16_t *y, int bsize_w,
                              int bsize_h, int qindex) {
  assert(bsize_w >= 8 && bsize_h >= 8);

  int activity_masking = 0;

  int i, j;
  DECLARE_ALIGNED(16, od_coeff, e[MAX_SB_SQUARE]);
  DECLARE_ALIGNED(16, od_coeff, tmp[MAX_SB_SQUARE]);
  DECLARE_ALIGNED(16, od_coeff, e_lp[MAX_SB_SQUARE]);
  for (i = 0; i < bsize_h; i++) {
    for (j = 0; j < bsize_w; j++) {
      e[i * bsize_w + j] = x[i * bsize_w + j] - y[i * bsize_w + j];
    }
  }
  int mid = OD_DIST_LP_MID;
  for (i = 0; i < bsize_h; i++) {
    tmp[i * bsize_w] = mid * e[i * bsize_w] + 2 * e[i * bsize_w + 1];
    tmp[i * bsize_w + bsize_w - 1] =
        mid * e[i * bsize_w + bsize_w - 1] + 2 * e[i * bsize_w + bsize_w - 2];
    for (j = 1; j < bsize_w - 1; j++) {
      tmp[i * bsize_w + j] = mid * e[i * bsize_w + j] + e[i * bsize_w + j - 1] +
                             e[i * bsize_w + j + 1];
    }
  }
  return od_compute_dist_common(activity_masking, x, y, bsize_w, bsize_h,
                                qindex, tmp, e_lp);
}

static double od_compute_dist_diff(uint16_t *x, int16_t *e, int bsize_w,
                                   int bsize_h, int qindex) {
  assert(bsize_w >= 8 && bsize_h >= 8);

  int activity_masking = 0;

  DECLARE_ALIGNED(16, uint16_t, y[MAX_SB_SQUARE]);
  DECLARE_ALIGNED(16, od_coeff, tmp[MAX_SB_SQUARE]);
  DECLARE_ALIGNED(16, od_coeff, e_lp[MAX_SB_SQUARE]);
  int i, j;
  for (i = 0; i < bsize_h; i++) {
    for (j = 0; j < bsize_w; j++) {
      y[i * bsize_w + j] = x[i * bsize_w + j] - e[i * bsize_w + j];
    }
  }
  int mid = OD_DIST_LP_MID;
  for (i = 0; i < bsize_h; i++) {
    tmp[i * bsize_w] = mid * e[i * bsize_w] + 2 * e[i * bsize_w + 1];
    tmp[i * bsize_w + bsize_w - 1] =
        mid * e[i * bsize_w + bsize_w - 1] + 2 * e[i * bsize_w + bsize_w - 2];
    for (j = 1; j < bsize_w - 1; j++) {
      tmp[i * bsize_w + j] = mid * e[i * bsize_w + j] + e[i * bsize_w + j - 1] +
                             e[i * bsize_w + j + 1];
    }
  }
  return od_compute_dist_common(activity_masking, x, y, bsize_w, bsize_h,
                                qindex, tmp, e_lp);
}

int64_t av1_dist_8x8(const AV1_COMP *const cpi, const MACROBLOCK *x,
                     const uint8_t *src, int src_stride, const uint8_t *dst,
                     int dst_stride, const BLOCK_SIZE tx_bsize, int bsw,
                     int bsh, int visible_w, int visible_h, int qindex) {
  int64_t d = 0;
  int i, j;
  const MACROBLOCKD *xd = &x->e_mbd;

  DECLARE_ALIGNED(16, uint16_t, orig[MAX_SB_SQUARE]);
  DECLARE_ALIGNED(16, uint16_t, rec[MAX_SB_SQUARE]);

  assert(bsw >= 8);
  assert(bsh >= 8);
  assert((bsw & 0x07) == 0);
  assert((bsh & 0x07) == 0);

  if (x->tune_metric == AOM_TUNE_CDEF_DIST ||
      x->tune_metric == AOM_TUNE_DAALA_DIST) {
    if (is_cur_buf_hbd(xd)) {
      for (j = 0; j < bsh; j++)
        for (i = 0; i < bsw; i++)
          orig[j * bsw + i] = CONVERT_TO_SHORTPTR(src)[j * src_stride + i];

      if ((bsw == visible_w) && (bsh == visible_h)) {
        for (j = 0; j < bsh; j++)
          for (i = 0; i < bsw; i++)
            rec[j * bsw + i] = CONVERT_TO_SHORTPTR(dst)[j * dst_stride + i];
      } else {
        for (j = 0; j < visible_h; j++)
          for (i = 0; i < visible_w; i++)
            rec[j * bsw + i] = CONVERT_TO_SHORTPTR(dst)[j * dst_stride + i];

        if (visible_w < bsw) {
          for (j = 0; j < bsh; j++)
            for (i = visible_w; i < bsw; i++)
              rec[j * bsw + i] = CONVERT_TO_SHORTPTR(src)[j * src_stride + i];
        }

        if (visible_h < bsh) {
          for (j = visible_h; j < bsh; j++)
            for (i = 0; i < bsw; i++)
              rec[j * bsw + i] = CONVERT_TO_SHORTPTR(src)[j * src_stride + i];
        }
      }
    } else {
      for (j = 0; j < bsh; j++)
        for (i = 0; i < bsw; i++) orig[j * bsw + i] = src[j * src_stride + i];

      if ((bsw == visible_w) && (bsh == visible_h)) {
        for (j = 0; j < bsh; j++)
          for (i = 0; i < bsw; i++) rec[j * bsw + i] = dst[j * dst_stride + i];
      } else {
        for (j = 0; j < visible_h; j++)
          for (i = 0; i < visible_w; i++)
            rec[j * bsw + i] = dst[j * dst_stride + i];

        if (visible_w < bsw) {
          for (j = 0; j < bsh; j++)
            for (i = visible_w; i < bsw; i++)
              rec[j * bsw + i] = src[j * src_stride + i];
        }

        if (visible_h < bsh) {
          for (j = visible_h; j < bsh; j++)
            for (i = 0; i < bsw; i++)
              rec[j * bsw + i] = src[j * src_stride + i];
        }
      }
    }
  }

  if (x->tune_metric == AOM_TUNE_DAALA_DIST) {
    d = (int64_t)od_compute_dist(orig, rec, bsw, bsh, qindex);
  } else if (x->tune_metric == AOM_TUNE_CDEF_DIST) {
    int coeff_shift = AOMMAX(xd->bd - 8, 0);

    for (i = 0; i < bsh; i += 8) {
      for (j = 0; j < bsw; j += 8) {
        d += cdef_dist_8x8_16bit(&rec[i * bsw + j], bsw, &orig[i * bsw + j],
                                 bsw, coeff_shift);
      }
    }
    if (is_cur_buf_hbd(xd)) d = ((uint64_t)d) >> 2 * coeff_shift;
  } else {
    // Otherwise, MSE by default
    d = pixel_dist_visible_only(cpi, x, src, src_stride, dst, dst_stride,
                                tx_bsize, bsh, bsw, visible_h, visible_w);
  }

  return d;
}

static int64_t dist_8x8_diff(const MACROBLOCK *x, const uint8_t *src,
                             int src_stride, const int16_t *diff,
                             int diff_stride, int bsw, int bsh, int visible_w,
                             int visible_h, int qindex) {
  int64_t d = 0;
  int i, j;
  const MACROBLOCKD *xd = &x->e_mbd;

  DECLARE_ALIGNED(16, uint16_t, orig[MAX_SB_SQUARE]);
  DECLARE_ALIGNED(16, int16_t, diff16[MAX_SB_SQUARE]);

  assert(bsw >= 8);
  assert(bsh >= 8);
  assert((bsw & 0x07) == 0);
  assert((bsh & 0x07) == 0);

  if (x->tune_metric == AOM_TUNE_CDEF_DIST ||
      x->tune_metric == AOM_TUNE_DAALA_DIST) {
    if (is_cur_buf_hbd(xd)) {
      for (j = 0; j < bsh; j++)
        for (i = 0; i < bsw; i++)
          orig[j * bsw + i] = CONVERT_TO_SHORTPTR(src)[j * src_stride + i];
    } else {
      for (j = 0; j < bsh; j++)
        for (i = 0; i < bsw; i++) orig[j * bsw + i] = src[j * src_stride + i];
    }

    if ((bsw == visible_w) && (bsh == visible_h)) {
      for (j = 0; j < bsh; j++)
        for (i = 0; i < bsw; i++)
          diff16[j * bsw + i] = diff[j * diff_stride + i];
    } else {
      for (j = 0; j < visible_h; j++)
        for (i = 0; i < visible_w; i++)
          diff16[j * bsw + i] = diff[j * diff_stride + i];

      if (visible_w < bsw) {
        for (j = 0; j < bsh; j++)
          for (i = visible_w; i < bsw; i++) diff16[j * bsw + i] = 0;
      }

      if (visible_h < bsh) {
        for (j = visible_h; j < bsh; j++)
          for (i = 0; i < bsw; i++) diff16[j * bsw + i] = 0;
      }
    }
  }

  if (x->tune_metric == AOM_TUNE_DAALA_DIST) {
    d = (int64_t)od_compute_dist_diff(orig, diff16, bsw, bsh, qindex);
  } else if (x->tune_metric == AOM_TUNE_CDEF_DIST) {
    int coeff_shift = AOMMAX(xd->bd - 8, 0);
    DECLARE_ALIGNED(16, uint16_t, dst16[MAX_SB_SQUARE]);

    for (i = 0; i < bsh; i++) {
      for (j = 0; j < bsw; j++) {
        dst16[i * bsw + j] = orig[i * bsw + j] - diff16[i * bsw + j];
      }
    }

    for (i = 0; i < bsh; i += 8) {
      for (j = 0; j < bsw; j += 8) {
        d += cdef_dist_8x8_16bit(&dst16[i * bsw + j], bsw, &orig[i * bsw + j],
                                 bsw, coeff_shift);
      }
    }
    // Don't scale 'd' for HBD since it will be done by caller side for diff
    // input
  } else {
    // Otherwise, MSE by default
    d = aom_sum_squares_2d_i16(diff, diff_stride, visible_w, visible_h);
  }

  return d;
}
#endif  // CONFIG_DIST_8X8

static AOM_INLINE void get_energy_distribution_finer(const int16_t *diff,
                                                     int stride, int bw, int bh,
                                                     float *hordist,
                                                     float *verdist) {
  // First compute downscaled block energy values (esq); downscale factors
  // are defined by w_shift and h_shift.
  unsigned int esq[256];
  const int w_shift = bw <= 8 ? 0 : 1;
  const int h_shift = bh <= 8 ? 0 : 1;
  const int esq_w = bw >> w_shift;
  const int esq_h = bh >> h_shift;
  const int esq_sz = esq_w * esq_h;
  int i, j;
  memset(esq, 0, esq_sz * sizeof(esq[0]));
  if (w_shift) {
    for (i = 0; i < bh; i++) {
      unsigned int *cur_esq_row = esq + (i >> h_shift) * esq_w;
      const int16_t *cur_diff_row = diff + i * stride;
      for (j = 0; j < bw; j += 2) {
        cur_esq_row[j >> 1] += (cur_diff_row[j] * cur_diff_row[j] +
                                cur_diff_row[j + 1] * cur_diff_row[j + 1]);
      }
    }
  } else {
    for (i = 0; i < bh; i++) {
      unsigned int *cur_esq_row = esq + (i >> h_shift) * esq_w;
      const int16_t *cur_diff_row = diff + i * stride;
      for (j = 0; j < bw; j++) {
        cur_esq_row[j] += cur_diff_row[j] * cur_diff_row[j];
      }
    }
  }

  uint64_t total = 0;
  for (i = 0; i < esq_sz; i++) total += esq[i];

  // Output hordist and verdist arrays are normalized 1D projections of esq
  if (total == 0) {
    float hor_val = 1.0f / esq_w;
    for (j = 0; j < esq_w - 1; j++) hordist[j] = hor_val;
    float ver_val = 1.0f / esq_h;
    for (i = 0; i < esq_h - 1; i++) verdist[i] = ver_val;
    return;
  }

  const float e_recip = 1.0f / (float)total;
  memset(hordist, 0, (esq_w - 1) * sizeof(hordist[0]));
  memset(verdist, 0, (esq_h - 1) * sizeof(verdist[0]));
  const unsigned int *cur_esq_row;
  for (i = 0; i < esq_h - 1; i++) {
    cur_esq_row = esq + i * esq_w;
    for (j = 0; j < esq_w - 1; j++) {
      hordist[j] += (float)cur_esq_row[j];
      verdist[i] += (float)cur_esq_row[j];
    }
    verdist[i] += (float)cur_esq_row[j];
  }
  cur_esq_row = esq + i * esq_w;
  for (j = 0; j < esq_w - 1; j++) hordist[j] += (float)cur_esq_row[j];

  for (j = 0; j < esq_w - 1; j++) hordist[j] *= e_recip;
  for (i = 0; i < esq_h - 1; i++) verdist[i] *= e_recip;
}

// Similar to get_horver_correlation, but also takes into account first
// row/column, when computing horizontal/vertical correlation.
void av1_get_horver_correlation_full_c(const int16_t *diff, int stride,
                                       int width, int height, float *hcorr,
                                       float *vcorr) {
  // The following notation is used:
  // x - current pixel
  // y - left neighbor pixel
  // z - top neighbor pixel
  int64_t x_sum = 0, x2_sum = 0, xy_sum = 0, xz_sum = 0;
  int64_t x_firstrow = 0, x_finalrow = 0, x_firstcol = 0, x_finalcol = 0;
  int64_t x2_firstrow = 0, x2_finalrow = 0, x2_firstcol = 0, x2_finalcol = 0;

  // First, process horizontal correlation on just the first row
  x_sum += diff[0];
  x2_sum += diff[0] * diff[0];
  x_firstrow += diff[0];
  x2_firstrow += diff[0] * diff[0];
  for (int j = 1; j < width; ++j) {
    const int16_t x = diff[j];
    const int16_t y = diff[j - 1];
    x_sum += x;
    x_firstrow += x;
    x2_sum += x * x;
    x2_firstrow += x * x;
    xy_sum += x * y;
  }

  // Process vertical correlation in the first column
  x_firstcol += diff[0];
  x2_firstcol += diff[0] * diff[0];
  for (int i = 1; i < height; ++i) {
    const int16_t x = diff[i * stride];
    const int16_t z = diff[(i - 1) * stride];
    x_sum += x;
    x_firstcol += x;
    x2_sum += x * x;
    x2_firstcol += x * x;
    xz_sum += x * z;
  }

  // Now process horiz and vert correlation through the rest unit
  for (int i = 1; i < height; ++i) {
    for (int j = 1; j < width; ++j) {
      const int16_t x = diff[i * stride + j];
      const int16_t y = diff[i * stride + j - 1];
      const int16_t z = diff[(i - 1) * stride + j];
      x_sum += x;
      x2_sum += x * x;
      xy_sum += x * y;
      xz_sum += x * z;
    }
  }

  for (int j = 0; j < width; ++j) {
    x_finalrow += diff[(height - 1) * stride + j];
    x2_finalrow +=
        diff[(height - 1) * stride + j] * diff[(height - 1) * stride + j];
  }
  for (int i = 0; i < height; ++i) {
    x_finalcol += diff[i * stride + width - 1];
    x2_finalcol += diff[i * stride + width - 1] * diff[i * stride + width - 1];
  }

  int64_t xhor_sum = x_sum - x_finalcol;
  int64_t xver_sum = x_sum - x_finalrow;
  int64_t y_sum = x_sum - x_firstcol;
  int64_t z_sum = x_sum - x_firstrow;
  int64_t x2hor_sum = x2_sum - x2_finalcol;
  int64_t x2ver_sum = x2_sum - x2_finalrow;
  int64_t y2_sum = x2_sum - x2_firstcol;
  int64_t z2_sum = x2_sum - x2_firstrow;

  const float num_hor = (float)(height * (width - 1));
  const float num_ver = (float)((height - 1) * width);

  const float xhor_var_n = x2hor_sum - (xhor_sum * xhor_sum) / num_hor;
  const float xver_var_n = x2ver_sum - (xver_sum * xver_sum) / num_ver;

  const float y_var_n = y2_sum - (y_sum * y_sum) / num_hor;
  const float z_var_n = z2_sum - (z_sum * z_sum) / num_ver;

  const float xy_var_n = xy_sum - (xhor_sum * y_sum) / num_hor;
  const float xz_var_n = xz_sum - (xver_sum * z_sum) / num_ver;

  if (xhor_var_n > 0 && y_var_n > 0) {
    *hcorr = xy_var_n / sqrtf(xhor_var_n * y_var_n);
    *hcorr = *hcorr < 0 ? 0 : *hcorr;
  } else {
    *hcorr = 1.0;
  }
  if (xver_var_n > 0 && z_var_n > 0) {
    *vcorr = xz_var_n / sqrtf(xver_var_n * z_var_n);
    *vcorr = *vcorr < 0 ? 0 : *vcorr;
  } else {
    *vcorr = 1.0;
  }
}

// These thresholds were calibrated to provide a certain number of TX types
// pruned by the model on average, i.e. selecting a threshold with index i
// will lead to pruning i+1 TX types on average
static const float *prune_2D_adaptive_thresholds[] = {
  // TX_4X4
  (float[]){ 0.00549f, 0.01306f, 0.02039f, 0.02747f, 0.03406f, 0.04065f,
             0.04724f, 0.05383f, 0.06067f, 0.06799f, 0.07605f, 0.08533f,
             0.09778f, 0.11780f },
  // TX_8X8
  (float[]){ 0.00037f, 0.00183f, 0.00525f, 0.01038f, 0.01697f, 0.02502f,
             0.03381f, 0.04333f, 0.05286f, 0.06287f, 0.07434f, 0.08850f,
             0.10803f, 0.14124f },
  // TX_16X16
  (float[]){ 0.01404f, 0.02000f, 0.04211f, 0.05164f, 0.05798f, 0.06335f,
             0.06897f, 0.07629f, 0.08875f, 0.11169f },
  // TX_32X32
  NULL,
  // TX_64X64
  NULL,
  // TX_4X8
  (float[]){ 0.00183f, 0.00745f, 0.01428f, 0.02185f, 0.02966f, 0.03723f,
             0.04456f, 0.05188f, 0.05920f, 0.06702f, 0.07605f, 0.08704f,
             0.10168f, 0.12585f },
  // TX_8X4
  (float[]){ 0.00085f, 0.00476f, 0.01135f, 0.01892f, 0.02698f, 0.03528f,
             0.04358f, 0.05164f, 0.05994f, 0.06848f, 0.07849f, 0.09021f,
             0.10583f, 0.13123f },
  // TX_8X16
  (float[]){ 0.00037f, 0.00232f, 0.00671f, 0.01257f, 0.01965f, 0.02722f,
             0.03552f, 0.04382f, 0.05237f, 0.06189f, 0.07336f, 0.08728f,
             0.10730f, 0.14221f },
  // TX_16X8
  (float[]){ 0.00061f, 0.00330f, 0.00818f, 0.01453f, 0.02185f, 0.02966f,
             0.03772f, 0.04578f, 0.05383f, 0.06262f, 0.07288f, 0.08582f,
             0.10339f, 0.13464f },
  // TX_16X32
  NULL,
  // TX_32X16
  NULL,
  // TX_32X64
  NULL,
  // TX_64X32
  NULL,
  // TX_4X16
  (float[]){ 0.00232f, 0.00671f, 0.01257f, 0.01941f, 0.02673f, 0.03430f,
             0.04211f, 0.04968f, 0.05750f, 0.06580f, 0.07507f, 0.08655f,
             0.10242f, 0.12878f },
  // TX_16X4
  (float[]){ 0.00110f, 0.00525f, 0.01208f, 0.01990f, 0.02795f, 0.03601f,
             0.04358f, 0.05115f, 0.05896f, 0.06702f, 0.07629f, 0.08752f,
             0.10217f, 0.12610f },
  // TX_8X32
  NULL,
  // TX_32X8
  NULL,
  // TX_16X64
  NULL,
  // TX_64X16
  NULL,
};

// Probablities are sorted in descending order.
static INLINE void sort_probability(float prob[], int txk[], int len) {
  int i, j, k;

  for (i = 1; i <= len - 1; ++i) {
    for (j = 0; j < i; ++j) {
      if (prob[j] < prob[i]) {
        float temp;
        int tempi;

        temp = prob[i];
        tempi = txk[i];

        for (k = i; k > j; k--) {
          prob[k] = prob[k - 1];
          txk[k] = txk[k - 1];
        }

        prob[j] = temp;
        txk[j] = tempi;
        break;
      }
    }
  }
}

static void prune_tx_2D(MACROBLOCK *x, BLOCK_SIZE bsize, TX_SIZE tx_size,
                        int blk_row, int blk_col, TxSetType tx_set_type,
                        TX_TYPE_PRUNE_MODE prune_mode, int *txk_map,
                        uint16_t *allowed_tx_mask) {
  int tx_type_table_2D[16] = {
    DCT_DCT,      DCT_ADST,      DCT_FLIPADST,      V_DCT,
    ADST_DCT,     ADST_ADST,     ADST_FLIPADST,     V_ADST,
    FLIPADST_DCT, FLIPADST_ADST, FLIPADST_FLIPADST, V_FLIPADST,
    H_DCT,        H_ADST,        H_FLIPADST,        IDTX
  };
  if (tx_set_type != EXT_TX_SET_ALL16 &&
      tx_set_type != EXT_TX_SET_DTT9_IDTX_1DDCT)
    return;
#if CONFIG_NN_V2
  NN_CONFIG_V2 *nn_config_hor = av1_tx_type_nnconfig_map_hor[tx_size];
  NN_CONFIG_V2 *nn_config_ver = av1_tx_type_nnconfig_map_ver[tx_size];
#else
  const NN_CONFIG *nn_config_hor = av1_tx_type_nnconfig_map_hor[tx_size];
  const NN_CONFIG *nn_config_ver = av1_tx_type_nnconfig_map_ver[tx_size];
#endif
  if (!nn_config_hor || !nn_config_ver) return;  // Model not established yet.

  aom_clear_system_state();
  float hfeatures[16], vfeatures[16];
  float hscores[4], vscores[4];
  float scores_2D_raw[16];
  float scores_2D[16];
  const int bw = tx_size_wide[tx_size];
  const int bh = tx_size_high[tx_size];
  const int hfeatures_num = bw <= 8 ? bw : bw / 2;
  const int vfeatures_num = bh <= 8 ? bh : bh / 2;
  assert(hfeatures_num <= 16);
  assert(vfeatures_num <= 16);

  const struct macroblock_plane *const p = &x->plane[0];
  const int diff_stride = block_size_wide[bsize];
  const int16_t *diff = p->src_diff + 4 * blk_row * diff_stride + 4 * blk_col;
  get_energy_distribution_finer(diff, diff_stride, bw, bh, hfeatures,
                                vfeatures);
  av1_get_horver_correlation_full(diff, diff_stride, bw, bh,
                                  &hfeatures[hfeatures_num - 1],
                                  &vfeatures[vfeatures_num - 1]);
  aom_clear_system_state();
#if CONFIG_NN_V2
  av1_nn_predict_v2(hfeatures, nn_config_hor, 0, hscores);
  av1_nn_predict_v2(vfeatures, nn_config_ver, 0, vscores);
#else
  av1_nn_predict(hfeatures, nn_config_hor, 1, hscores);
  av1_nn_predict(vfeatures, nn_config_ver, 1, vscores);
#endif
  aom_clear_system_state();

  for (int i = 0; i < 4; i++) {
    float *cur_scores_2D = scores_2D_raw + i * 4;
    cur_scores_2D[0] = vscores[i] * hscores[0];
    cur_scores_2D[1] = vscores[i] * hscores[1];
    cur_scores_2D[2] = vscores[i] * hscores[2];
    cur_scores_2D[3] = vscores[i] * hscores[3];
  }

  av1_nn_softmax(scores_2D_raw, scores_2D, 16);

  const int prune_aggr_table[4][2] = { { 4, 1 }, { 6, 3 }, { 9, 6 }, { 9, 6 } };
  int pruning_aggressiveness = 0;
  if (tx_set_type == EXT_TX_SET_ALL16) {
    pruning_aggressiveness =
        prune_aggr_table[prune_mode - PRUNE_2D_ACCURATE][0];
  } else if (tx_set_type == EXT_TX_SET_DTT9_IDTX_1DDCT) {
    pruning_aggressiveness =
        prune_aggr_table[prune_mode - PRUNE_2D_ACCURATE][1];
  }

  // Always keep the TX type with the highest score, prune all others with
  // score below score_thresh.
  int max_score_i = 0;
  float max_score = 0.0f;
  for (int i = 0; i < 16; i++) {
    if (scores_2D[i] > max_score &&
        (*allowed_tx_mask & (1 << tx_type_table_2D[i]))) {
      max_score = scores_2D[i];
      max_score_i = i;
    }
  }

  const float score_thresh =
      prune_2D_adaptive_thresholds[tx_size][pruning_aggressiveness];

  uint16_t allow_bitmask = 0;
  float sum_score = 0.0;
  // Calculate sum of allowed tx type score and Populate allow bit mask based
  // on score_thresh and allowed_tx_mask
  for (int tx_idx = 0; tx_idx < TX_TYPES; tx_idx++) {
    int allow_tx_type = *allowed_tx_mask & (1 << tx_type_table_2D[tx_idx]);
    if ((scores_2D[tx_idx] >= score_thresh && allow_tx_type) ||
        tx_idx == max_score_i) {
      // Set allow mask based on score_thresh and tx type with max score
      allow_bitmask |= (1 << tx_type_table_2D[tx_idx]);

      // Accumulate score of allowed tx type
      sum_score += scores_2D[tx_idx];
    }
  }
  // Sort tx type probability of all types
  sort_probability(scores_2D, tx_type_table_2D, TX_TYPES);

  // Enable more pruning based on tx type probability and number of allowed tx
  // types
  if (prune_mode == PRUNE_2D_AGGRESSIVE) {
    float temp_score = 0.0;
    float score_ratio = 0.0;
    int tx_idx, tx_count = 0;
    const float inv_sum_score = 100 / sum_score;
    // Get allowed tx types based on sorted probability score and tx count
    for (tx_idx = 0; tx_idx < TX_TYPES; tx_idx++) {
      // Skip the tx type which has more than 30% of cumulative
      // probability and allowed tx type count is more than 2
      if (score_ratio > 30.0 && tx_count >= 2) break;

      // Calculate cumulative probability of allowed tx types
      if (allow_bitmask & (1 << tx_type_table_2D[tx_idx])) {
        // Calculate cumulative probability
        temp_score += scores_2D[tx_idx];

        // Calculate percentage of cumulative probability of allowed tx type
        score_ratio = temp_score * inv_sum_score;
        tx_count++;
      }
    }
    // Set remaining tx types as pruned
    for (; tx_idx < TX_TYPES; tx_idx++)
      allow_bitmask &= ~(1 << tx_type_table_2D[tx_idx]);
  }
  memcpy(txk_map, tx_type_table_2D, sizeof(tx_type_table_2D));
  *allowed_tx_mask = allow_bitmask;
}

static int64_t get_sse(const AV1_COMP *cpi, const MACROBLOCK *x) {
  const AV1_COMMON *cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  const MACROBLOCKD *xd = &x->e_mbd;
  const MB_MODE_INFO *mbmi = xd->mi[0];
  int64_t total_sse = 0;
  for (int plane = 0; plane < num_planes; ++plane) {
    const struct macroblock_plane *const p = &x->plane[plane];
    const struct macroblockd_plane *const pd = &xd->plane[plane];
    const BLOCK_SIZE bs = get_plane_block_size(mbmi->sb_type, pd->subsampling_x,
                                               pd->subsampling_y);
    unsigned int sse;

    if (x->skip_chroma_rd && plane) continue;

    cpi->fn_ptr[bs].vf(p->src.buf, p->src.stride, pd->dst.buf, pd->dst.stride,
                       &sse);
    total_sse += sse;
  }
  total_sse <<= 4;
  return total_sse;
}

int64_t av1_block_error_c(const tran_low_t *coeff, const tran_low_t *dqcoeff,
                          intptr_t block_size, int64_t *ssz) {
  int i;
  int64_t error = 0, sqcoeff = 0;

  for (i = 0; i < block_size; i++) {
    const int diff = coeff[i] - dqcoeff[i];
    error += diff * diff;
    sqcoeff += coeff[i] * coeff[i];
  }

  *ssz = sqcoeff;
  return error;
}

#if CONFIG_AV1_HIGHBITDEPTH
int64_t av1_highbd_block_error_c(const tran_low_t *coeff,
                                 const tran_low_t *dqcoeff, intptr_t block_size,
                                 int64_t *ssz, int bd) {
  int i;
  int64_t error = 0, sqcoeff = 0;
  int shift = 2 * (bd - 8);
  int rounding = shift > 0 ? 1 << (shift - 1) : 0;

  for (i = 0; i < block_size; i++) {
    const int64_t diff = coeff[i] - dqcoeff[i];
    error += diff * diff;
    sqcoeff += (int64_t)coeff[i] * (int64_t)coeff[i];
  }
  assert(error >= 0 && sqcoeff >= 0);
  error = (error + rounding) >> shift;
  sqcoeff = (sqcoeff + rounding) >> shift;

  *ssz = sqcoeff;
  return error;
}
#endif

// Compute the pixel domain distortion from src and dst on all visible 4x4s in
// the
// transform block.
static unsigned pixel_dist(const AV1_COMP *const cpi, const MACROBLOCK *x,
                           int plane, const uint8_t *src, const int src_stride,
                           const uint8_t *dst, const int dst_stride,
                           int blk_row, int blk_col,
                           const BLOCK_SIZE plane_bsize,
                           const BLOCK_SIZE tx_bsize) {
  int txb_rows, txb_cols, visible_rows, visible_cols;
  const MACROBLOCKD *xd = &x->e_mbd;

  get_txb_dimensions(xd, plane, plane_bsize, blk_row, blk_col, tx_bsize,
                     &txb_cols, &txb_rows, &visible_cols, &visible_rows);
  assert(visible_rows > 0);
  assert(visible_cols > 0);

#if CONFIG_DIST_8X8
  if (x->using_dist_8x8 && plane == 0)
    return (unsigned)av1_dist_8x8(cpi, x, src, src_stride, dst, dst_stride,
                                  tx_bsize, txb_cols, txb_rows, visible_cols,
                                  visible_rows, x->qindex);
#endif  // CONFIG_DIST_8X8

  unsigned sse = pixel_dist_visible_only(cpi, x, src, src_stride, dst,
                                         dst_stride, tx_bsize, txb_rows,
                                         txb_cols, visible_rows, visible_cols);

  return sse;
}

// Compute the pixel domain distortion from diff on all visible 4x4s in the
// transform block.
static INLINE int64_t pixel_diff_dist(const MACROBLOCK *x, int plane,
                                      int blk_row, int blk_col,
                                      const BLOCK_SIZE plane_bsize,
                                      const BLOCK_SIZE tx_bsize,
                                      unsigned int *block_mse_q8) {
  int visible_rows, visible_cols;
  const MACROBLOCKD *xd = &x->e_mbd;
  get_txb_dimensions(xd, plane, plane_bsize, blk_row, blk_col, tx_bsize, NULL,
                     NULL, &visible_cols, &visible_rows);
  const int diff_stride = block_size_wide[plane_bsize];
  const int16_t *diff = x->plane[plane].src_diff;
#if CONFIG_DIST_8X8
  int txb_height = block_size_high[tx_bsize];
  int txb_width = block_size_wide[tx_bsize];
  if (x->using_dist_8x8 && plane == 0) {
    const int src_stride = x->plane[plane].src.stride;
    const int src_idx = (blk_row * src_stride + blk_col) << MI_SIZE_LOG2;
    const int diff_idx = (blk_row * diff_stride + blk_col) << MI_SIZE_LOG2;
    const uint8_t *src = &x->plane[plane].src.buf[src_idx];
    return dist_8x8_diff(x, src, src_stride, diff + diff_idx, diff_stride,
                         txb_width, txb_height, visible_cols, visible_rows,
                         x->qindex);
  }
#endif
  diff += ((blk_row * diff_stride + blk_col) << MI_SIZE_LOG2);
  uint64_t sse =
      aom_sum_squares_2d_i16(diff, diff_stride, visible_cols, visible_rows);
  if (block_mse_q8 != NULL) {
    if (visible_cols > 0 && visible_rows > 0)
      *block_mse_q8 =
          (unsigned int)((256 * sse) / (visible_cols * visible_rows));
    else
      *block_mse_q8 = UINT_MAX;
  }
  return sse;
}

int av1_count_colors(const uint8_t *src, int stride, int rows, int cols,
                     int *val_count) {
  const int max_pix_val = 1 << 8;
  memset(val_count, 0, max_pix_val * sizeof(val_count[0]));
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      const int this_val = src[r * stride + c];
      assert(this_val < max_pix_val);
      ++val_count[this_val];
    }
  }
  int n = 0;
  for (int i = 0; i < max_pix_val; ++i) {
    if (val_count[i]) ++n;
  }
  return n;
}

int av1_count_colors_highbd(const uint8_t *src8, int stride, int rows, int cols,
                            int bit_depth, int *val_count) {
  assert(bit_depth <= 12);
  const int max_pix_val = 1 << bit_depth;
  const uint16_t *src = CONVERT_TO_SHORTPTR(src8);
  memset(val_count, 0, max_pix_val * sizeof(val_count[0]));
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      const int this_val = src[r * stride + c];
      assert(this_val < max_pix_val);
      if (this_val >= max_pix_val) return 0;
      ++val_count[this_val];
    }
  }
  int n = 0;
  for (int i = 0; i < max_pix_val; ++i) {
    if (val_count[i]) ++n;
  }
  return n;
}

static AOM_INLINE void inverse_transform_block_facade(MACROBLOCKD *xd,
                                                      int plane, int block,
                                                      int blk_row, int blk_col,
                                                      int eob,
                                                      int reduced_tx_set) {
  if (!eob) return;

  struct macroblockd_plane *const pd = &xd->plane[plane];
  tran_low_t *dqcoeff = pd->dqcoeff + BLOCK_OFFSET(block);
  const PLANE_TYPE plane_type = get_plane_type(plane);
  const TX_SIZE tx_size = av1_get_tx_size(plane, xd);
  const TX_TYPE tx_type = av1_get_tx_type(xd, plane_type, blk_row, blk_col,
                                          tx_size, reduced_tx_set);
  const int dst_stride = pd->dst.stride;
  uint8_t *dst = &pd->dst.buf[(blk_row * dst_stride + blk_col) << MI_SIZE_LOG2];
  av1_inverse_transform_block(xd, dqcoeff, plane, tx_type, tx_size, dst,
                              dst_stride, eob, reduced_tx_set);
}

static int find_tx_size_rd_info(TXB_RD_RECORD *cur_record, const uint32_t hash);

static uint32_t get_intra_txb_hash(MACROBLOCK *x, int plane, int blk_row,
                                   int blk_col, BLOCK_SIZE plane_bsize,
                                   TX_SIZE tx_size) {
  int16_t tmp_data[64 * 64];
  const int diff_stride = block_size_wide[plane_bsize];
  const int16_t *diff = x->plane[plane].src_diff;
  const int16_t *cur_diff_row = diff + 4 * blk_row * diff_stride + 4 * blk_col;
  const int txb_w = tx_size_wide[tx_size];
  const int txb_h = tx_size_high[tx_size];
  uint8_t *hash_data = (uint8_t *)cur_diff_row;
  if (txb_w != diff_stride) {
    int16_t *cur_hash_row = tmp_data;
    for (int i = 0; i < txb_h; i++) {
      memcpy(cur_hash_row, cur_diff_row, sizeof(*diff) * txb_w);
      cur_hash_row += txb_w;
      cur_diff_row += diff_stride;
    }
    hash_data = (uint8_t *)tmp_data;
  }
  CRC32C *crc = &x->mb_rd_record.crc_calculator;
  const uint32_t hash = av1_get_crc32c_value(crc, hash_data, 2 * txb_w * txb_h);
  return (hash << 5) + tx_size;
}

static INLINE void dist_block_tx_domain(MACROBLOCK *x, int plane, int block,
                                        TX_SIZE tx_size, int64_t *out_dist,
                                        int64_t *out_sse) {
  MACROBLOCKD *const xd = &x->e_mbd;
  const struct macroblock_plane *const p = &x->plane[plane];
  const struct macroblockd_plane *const pd = &xd->plane[plane];
  // Transform domain distortion computation is more efficient as it does
  // not involve an inverse transform, but it is less accurate.
  const int buffer_length = av1_get_max_eob(tx_size);
  int64_t this_sse;
  // TX-domain results need to shift down to Q2/D10 to match pixel
  // domain distortion values which are in Q2^2
  int shift = (MAX_TX_SCALE - av1_get_tx_scale(tx_size)) * 2;
  const int block_offset = BLOCK_OFFSET(block);
  tran_low_t *const coeff = p->coeff + block_offset;
  tran_low_t *const dqcoeff = pd->dqcoeff + block_offset;
#if CONFIG_AV1_HIGHBITDEPTH
  if (is_cur_buf_hbd(xd))
    *out_dist = av1_highbd_block_error(coeff, dqcoeff, buffer_length, &this_sse,
                                       xd->bd);
  else
    *out_dist = av1_block_error(coeff, dqcoeff, buffer_length, &this_sse);
#else
  *out_dist = av1_block_error(coeff, dqcoeff, buffer_length, &this_sse);
#endif
  *out_dist = RIGHT_SIGNED_SHIFT(*out_dist, shift);
  *out_sse = RIGHT_SIGNED_SHIFT(this_sse, shift);
}

static INLINE int64_t dist_block_px_domain(const AV1_COMP *cpi, MACROBLOCK *x,
                                           int plane, BLOCK_SIZE plane_bsize,
                                           int block, int blk_row, int blk_col,
                                           TX_SIZE tx_size) {
  MACROBLOCKD *const xd = &x->e_mbd;
  const struct macroblock_plane *const p = &x->plane[plane];
  const struct macroblockd_plane *const pd = &xd->plane[plane];
  const uint16_t eob = p->eobs[block];
  const BLOCK_SIZE tx_bsize = txsize_to_bsize[tx_size];
  const int bsw = block_size_wide[tx_bsize];
  const int bsh = block_size_high[tx_bsize];
  const int src_stride = x->plane[plane].src.stride;
  const int dst_stride = xd->plane[plane].dst.stride;
  // Scale the transform block index to pixel unit.
  const int src_idx = (blk_row * src_stride + blk_col) << MI_SIZE_LOG2;
  const int dst_idx = (blk_row * dst_stride + blk_col) << MI_SIZE_LOG2;
  const uint8_t *src = &x->plane[plane].src.buf[src_idx];
  const uint8_t *dst = &xd->plane[plane].dst.buf[dst_idx];
  const tran_low_t *dqcoeff = pd->dqcoeff + BLOCK_OFFSET(block);

  assert(cpi != NULL);
  assert(tx_size_wide_log2[0] == tx_size_high_log2[0]);

  uint8_t *recon;
  DECLARE_ALIGNED(16, uint16_t, recon16[MAX_TX_SQUARE]);

#if CONFIG_AV1_HIGHBITDEPTH
  if (is_cur_buf_hbd(xd)) {
    recon = CONVERT_TO_BYTEPTR(recon16);
    av1_highbd_convolve_2d_copy_sr(CONVERT_TO_SHORTPTR(dst), dst_stride,
                                   CONVERT_TO_SHORTPTR(recon), MAX_TX_SIZE, bsw,
                                   bsh, NULL, NULL, 0, 0, NULL, xd->bd);
  } else {
    recon = (uint8_t *)recon16;
    av1_convolve_2d_copy_sr(dst, dst_stride, recon, MAX_TX_SIZE, bsw, bsh, NULL,
                            NULL, 0, 0, NULL);
  }
#else
  recon = (uint8_t *)recon16;
  av1_convolve_2d_copy_sr(dst, dst_stride, recon, MAX_TX_SIZE, bsw, bsh, NULL,
                          NULL, 0, 0, NULL);
#endif

  const PLANE_TYPE plane_type = get_plane_type(plane);
  TX_TYPE tx_type = av1_get_tx_type(xd, plane_type, blk_row, blk_col, tx_size,
                                    cpi->common.reduced_tx_set_used);
  av1_inverse_transform_block(xd, dqcoeff, plane, tx_type, tx_size, recon,
                              MAX_TX_SIZE, eob,
                              cpi->common.reduced_tx_set_used);

  return 16 * pixel_dist(cpi, x, plane, src, src_stride, recon, MAX_TX_SIZE,
                         blk_row, blk_col, plane_bsize, tx_bsize);
}

// NOTE: CONFIG_COLLECT_RD_STATS has 3 possible values
// 0: Do not collect any RD stats
// 1: Collect RD stats for transform units
// 2: Collect RD stats for partition units
#if CONFIG_COLLECT_RD_STATS

static AOM_INLINE void get_energy_distribution_fine(
    const AV1_COMP *cpi, BLOCK_SIZE bsize, const uint8_t *src, int src_stride,
    const uint8_t *dst, int dst_stride, int need_4th, double *hordist,
    double *verdist) {
  const int bw = block_size_wide[bsize];
  const int bh = block_size_high[bsize];
  unsigned int esq[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  if (bsize < BLOCK_16X16 || (bsize >= BLOCK_4X16 && bsize <= BLOCK_32X8)) {
    // Special cases: calculate 'esq' values manually, as we don't have 'vf'
    // functions for the 16 (very small) sub-blocks of this block.
    const int w_shift = (bw == 4) ? 0 : (bw == 8) ? 1 : (bw == 16) ? 2 : 3;
    const int h_shift = (bh == 4) ? 0 : (bh == 8) ? 1 : (bh == 16) ? 2 : 3;
    assert(bw <= 32);
    assert(bh <= 32);
    assert(((bw - 1) >> w_shift) + (((bh - 1) >> h_shift) << 2) == 15);
    if (cpi->common.seq_params.use_highbitdepth) {
      const uint16_t *src16 = CONVERT_TO_SHORTPTR(src);
      const uint16_t *dst16 = CONVERT_TO_SHORTPTR(dst);
      for (int i = 0; i < bh; ++i)
        for (int j = 0; j < bw; ++j) {
          const int index = (j >> w_shift) + ((i >> h_shift) << 2);
          esq[index] +=
              (src16[j + i * src_stride] - dst16[j + i * dst_stride]) *
              (src16[j + i * src_stride] - dst16[j + i * dst_stride]);
        }
    } else {
      for (int i = 0; i < bh; ++i)
        for (int j = 0; j < bw; ++j) {
          const int index = (j >> w_shift) + ((i >> h_shift) << 2);
          esq[index] += (src[j + i * src_stride] - dst[j + i * dst_stride]) *
                        (src[j + i * src_stride] - dst[j + i * dst_stride]);
        }
    }
  } else {  // Calculate 'esq' values using 'vf' functions on the 16 sub-blocks.
    const int f_index =
        (bsize < BLOCK_SIZES) ? bsize - BLOCK_16X16 : bsize - BLOCK_8X16;
    assert(f_index >= 0 && f_index < BLOCK_SIZES_ALL);
    const BLOCK_SIZE subsize = (BLOCK_SIZE)f_index;
    assert(block_size_wide[bsize] == 4 * block_size_wide[subsize]);
    assert(block_size_high[bsize] == 4 * block_size_high[subsize]);
    cpi->fn_ptr[subsize].vf(src, src_stride, dst, dst_stride, &esq[0]);
    cpi->fn_ptr[subsize].vf(src + bw / 4, src_stride, dst + bw / 4, dst_stride,
                            &esq[1]);
    cpi->fn_ptr[subsize].vf(src + bw / 2, src_stride, dst + bw / 2, dst_stride,
                            &esq[2]);
    cpi->fn_ptr[subsize].vf(src + 3 * bw / 4, src_stride, dst + 3 * bw / 4,
                            dst_stride, &esq[3]);
    src += bh / 4 * src_stride;
    dst += bh / 4 * dst_stride;

    cpi->fn_ptr[subsize].vf(src, src_stride, dst, dst_stride, &esq[4]);
    cpi->fn_ptr[subsize].vf(src + bw / 4, src_stride, dst + bw / 4, dst_stride,
                            &esq[5]);
    cpi->fn_ptr[subsize].vf(src + bw / 2, src_stride, dst + bw / 2, dst_stride,
                            &esq[6]);
    cpi->fn_ptr[subsize].vf(src + 3 * bw / 4, src_stride, dst + 3 * bw / 4,
                            dst_stride, &esq[7]);
    src += bh / 4 * src_stride;
    dst += bh / 4 * dst_stride;

    cpi->fn_ptr[subsize].vf(src, src_stride, dst, dst_stride, &esq[8]);
    cpi->fn_ptr[subsize].vf(src + bw / 4, src_stride, dst + bw / 4, dst_stride,
                            &esq[9]);
    cpi->fn_ptr[subsize].vf(src + bw / 2, src_stride, dst + bw / 2, dst_stride,
                            &esq[10]);
    cpi->fn_ptr[subsize].vf(src + 3 * bw / 4, src_stride, dst + 3 * bw / 4,
                            dst_stride, &esq[11]);
    src += bh / 4 * src_stride;
    dst += bh / 4 * dst_stride;

    cpi->fn_ptr[subsize].vf(src, src_stride, dst, dst_stride, &esq[12]);
    cpi->fn_ptr[subsize].vf(src + bw / 4, src_stride, dst + bw / 4, dst_stride,
                            &esq[13]);
    cpi->fn_ptr[subsize].vf(src + bw / 2, src_stride, dst + bw / 2, dst_stride,
                            &esq[14]);
    cpi->fn_ptr[subsize].vf(src + 3 * bw / 4, src_stride, dst + 3 * bw / 4,
                            dst_stride, &esq[15]);
  }

  double total = (double)esq[0] + esq[1] + esq[2] + esq[3] + esq[4] + esq[5] +
                 esq[6] + esq[7] + esq[8] + esq[9] + esq[10] + esq[11] +
                 esq[12] + esq[13] + esq[14] + esq[15];
  if (total > 0) {
    const double e_recip = 1.0 / total;
    hordist[0] = ((double)esq[0] + esq[4] + esq[8] + esq[12]) * e_recip;
    hordist[1] = ((double)esq[1] + esq[5] + esq[9] + esq[13]) * e_recip;
    hordist[2] = ((double)esq[2] + esq[6] + esq[10] + esq[14]) * e_recip;
    if (need_4th) {
      hordist[3] = ((double)esq[3] + esq[7] + esq[11] + esq[15]) * e_recip;
    }
    verdist[0] = ((double)esq[0] + esq[1] + esq[2] + esq[3]) * e_recip;
    verdist[1] = ((double)esq[4] + esq[5] + esq[6] + esq[7]) * e_recip;
    verdist[2] = ((double)esq[8] + esq[9] + esq[10] + esq[11]) * e_recip;
    if (need_4th) {
      verdist[3] = ((double)esq[12] + esq[13] + esq[14] + esq[15]) * e_recip;
    }
  } else {
    hordist[0] = verdist[0] = 0.25;
    hordist[1] = verdist[1] = 0.25;
    hordist[2] = verdist[2] = 0.25;
    if (need_4th) {
      hordist[3] = verdist[3] = 0.25;
    }
  }
}

static double get_sse_norm(const int16_t *diff, int stride, int w, int h) {
  double sum = 0.0;
  for (int j = 0; j < h; ++j) {
    for (int i = 0; i < w; ++i) {
      const int err = diff[j * stride + i];
      sum += err * err;
    }
  }
  assert(w > 0 && h > 0);
  return sum / (w * h);
}

static double get_sad_norm(const int16_t *diff, int stride, int w, int h) {
  double sum = 0.0;
  for (int j = 0; j < h; ++j) {
    for (int i = 0; i < w; ++i) {
      sum += abs(diff[j * stride + i]);
    }
  }
  assert(w > 0 && h > 0);
  return sum / (w * h);
}

static AOM_INLINE void get_2x2_normalized_sses_and_sads(
    const AV1_COMP *const cpi, BLOCK_SIZE tx_bsize, const uint8_t *const src,
    int src_stride, const uint8_t *const dst, int dst_stride,
    const int16_t *const src_diff, int diff_stride, double *const sse_norm_arr,
    double *const sad_norm_arr) {
  const BLOCK_SIZE tx_bsize_half =
      get_partition_subsize(tx_bsize, PARTITION_SPLIT);
  if (tx_bsize_half == BLOCK_INVALID) {  // manually calculate stats
    const int half_width = block_size_wide[tx_bsize] / 2;
    const int half_height = block_size_high[tx_bsize] / 2;
    for (int row = 0; row < 2; ++row) {
      for (int col = 0; col < 2; ++col) {
        const int16_t *const this_src_diff =
            src_diff + row * half_height * diff_stride + col * half_width;
        if (sse_norm_arr) {
          sse_norm_arr[row * 2 + col] =
              get_sse_norm(this_src_diff, diff_stride, half_width, half_height);
        }
        if (sad_norm_arr) {
          sad_norm_arr[row * 2 + col] =
              get_sad_norm(this_src_diff, diff_stride, half_width, half_height);
        }
      }
    }
  } else {  // use function pointers to calculate stats
    const int half_width = block_size_wide[tx_bsize_half];
    const int half_height = block_size_high[tx_bsize_half];
    const int num_samples_half = half_width * half_height;
    for (int row = 0; row < 2; ++row) {
      for (int col = 0; col < 2; ++col) {
        const uint8_t *const this_src =
            src + row * half_height * src_stride + col * half_width;
        const uint8_t *const this_dst =
            dst + row * half_height * dst_stride + col * half_width;

        if (sse_norm_arr) {
          unsigned int this_sse;
          cpi->fn_ptr[tx_bsize_half].vf(this_src, src_stride, this_dst,
                                        dst_stride, &this_sse);
          sse_norm_arr[row * 2 + col] = (double)this_sse / num_samples_half;
        }

        if (sad_norm_arr) {
          const unsigned int this_sad = cpi->fn_ptr[tx_bsize_half].sdf(
              this_src, src_stride, this_dst, dst_stride);
          sad_norm_arr[row * 2 + col] = (double)this_sad / num_samples_half;
        }
      }
    }
  }
}

#if CONFIG_COLLECT_RD_STATS == 1
static double get_mean(const int16_t *diff, int stride, int w, int h) {
  double sum = 0.0;
  for (int j = 0; j < h; ++j) {
    for (int i = 0; i < w; ++i) {
      sum += diff[j * stride + i];
    }
  }
  assert(w > 0 && h > 0);
  return sum / (w * h);
}
static AOM_INLINE void PrintTransformUnitStats(
    const AV1_COMP *const cpi, MACROBLOCK *x, const RD_STATS *const rd_stats,
    int blk_row, int blk_col, BLOCK_SIZE plane_bsize, TX_SIZE tx_size,
    TX_TYPE tx_type, int64_t rd) {
  if (rd_stats->rate == INT_MAX || rd_stats->dist == INT64_MAX) return;

  // Generate small sample to restrict output size.
  static unsigned int seed = 21743;
  if (lcg_rand16(&seed) % 256 > 0) return;

  const char output_file[] = "tu_stats.txt";
  FILE *fout = fopen(output_file, "a");
  if (!fout) return;

  const BLOCK_SIZE tx_bsize = txsize_to_bsize[tx_size];
  const MACROBLOCKD *const xd = &x->e_mbd;
  const int plane = 0;
  struct macroblock_plane *const p = &x->plane[plane];
  const struct macroblockd_plane *const pd = &xd->plane[plane];
  const int txw = tx_size_wide[tx_size];
  const int txh = tx_size_high[tx_size];
  const int dequant_shift = (is_cur_buf_hbd(xd)) ? xd->bd - 5 : 3;
  const int q_step = p->dequant_QTX[1] >> dequant_shift;
  const int num_samples = txw * txh;

  const double rate_norm = (double)rd_stats->rate / num_samples;
  const double dist_norm = (double)rd_stats->dist / num_samples;

  fprintf(fout, "%g %g", rate_norm, dist_norm);

  const int src_stride = p->src.stride;
  const uint8_t *const src =
      &p->src.buf[(blk_row * src_stride + blk_col) << MI_SIZE_LOG2];
  const int dst_stride = pd->dst.stride;
  const uint8_t *const dst =
      &pd->dst.buf[(blk_row * dst_stride + blk_col) << MI_SIZE_LOG2];
  unsigned int sse;
  cpi->fn_ptr[tx_bsize].vf(src, src_stride, dst, dst_stride, &sse);
  const double sse_norm = (double)sse / num_samples;

  const unsigned int sad =
      cpi->fn_ptr[tx_bsize].sdf(src, src_stride, dst, dst_stride);
  const double sad_norm = (double)sad / num_samples;

  fprintf(fout, " %g %g", sse_norm, sad_norm);

  const int diff_stride = block_size_wide[plane_bsize];
  const int16_t *const src_diff =
      &p->src_diff[(blk_row * diff_stride + blk_col) << MI_SIZE_LOG2];

  double sse_norm_arr[4], sad_norm_arr[4];
  get_2x2_normalized_sses_and_sads(cpi, tx_bsize, src, src_stride, dst,
                                   dst_stride, src_diff, diff_stride,
                                   sse_norm_arr, sad_norm_arr);
  for (int i = 0; i < 4; ++i) {
    fprintf(fout, " %g", sse_norm_arr[i]);
  }
  for (int i = 0; i < 4; ++i) {
    fprintf(fout, " %g", sad_norm_arr[i]);
  }

  const TX_TYPE_1D tx_type_1d_row = htx_tab[tx_type];
  const TX_TYPE_1D tx_type_1d_col = vtx_tab[tx_type];

  fprintf(fout, " %d %d %d %d %d", q_step, tx_size_wide[tx_size],
          tx_size_high[tx_size], tx_type_1d_row, tx_type_1d_col);

  int model_rate;
  int64_t model_dist;
  model_rd_sse_fn[MODELRD_CURVFIT](cpi, x, tx_bsize, plane, sse, num_samples,
                                   &model_rate, &model_dist);
  const double model_rate_norm = (double)model_rate / num_samples;
  const double model_dist_norm = (double)model_dist / num_samples;
  fprintf(fout, " %g %g", model_rate_norm, model_dist_norm);

  const double mean = get_mean(src_diff, diff_stride, txw, txh);
  float hor_corr, vert_corr;
  av1_get_horver_correlation_full(src_diff, diff_stride, txw, txh, &hor_corr,
                                  &vert_corr);
  fprintf(fout, " %g %g %g", mean, hor_corr, vert_corr);

  double hdist[4] = { 0 }, vdist[4] = { 0 };
  get_energy_distribution_fine(cpi, tx_bsize, src, src_stride, dst, dst_stride,
                               1, hdist, vdist);
  fprintf(fout, " %g %g %g %g %g %g %g %g", hdist[0], hdist[1], hdist[2],
          hdist[3], vdist[0], vdist[1], vdist[2], vdist[3]);

  fprintf(fout, " %d %" PRId64, x->rdmult, rd);

  fprintf(fout, "\n");
  fclose(fout);
}
#endif  // CONFIG_COLLECT_RD_STATS == 1

#if CONFIG_COLLECT_RD_STATS >= 2
static double get_highbd_diff_mean(const uint8_t *src8, int src_stride,
                                   const uint8_t *dst8, int dst_stride, int w,
                                   int h) {
  const uint16_t *src = CONVERT_TO_SHORTPTR(src8);
  const uint16_t *dst = CONVERT_TO_SHORTPTR(dst8);
  double sum = 0.0;
  for (int j = 0; j < h; ++j) {
    for (int i = 0; i < w; ++i) {
      const int diff = src[j * src_stride + i] - dst[j * dst_stride + i];
      sum += diff;
    }
  }
  assert(w > 0 && h > 0);
  return sum / (w * h);
}

static double get_diff_mean(const uint8_t *src, int src_stride,
                            const uint8_t *dst, int dst_stride, int w, int h) {
  double sum = 0.0;
  for (int j = 0; j < h; ++j) {
    for (int i = 0; i < w; ++i) {
      const int diff = src[j * src_stride + i] - dst[j * dst_stride + i];
      sum += diff;
    }
  }
  assert(w > 0 && h > 0);
  return sum / (w * h);
}

static AOM_INLINE void PrintPredictionUnitStats(const AV1_COMP *const cpi,
                                                const TileDataEnc *tile_data,
                                                MACROBLOCK *x,
                                                const RD_STATS *const rd_stats,
                                                BLOCK_SIZE plane_bsize) {
  if (rd_stats->rate == INT_MAX || rd_stats->dist == INT64_MAX) return;

  if (cpi->sf.inter_sf.inter_mode_rd_model_estimation == 1 &&
      (tile_data == NULL ||
       !tile_data->inter_mode_rd_models[plane_bsize].ready))
    return;
  (void)tile_data;
  // Generate small sample to restrict output size.
  static unsigned int seed = 95014;

  if ((lcg_rand16(&seed) % (1 << (14 - num_pels_log2_lookup[plane_bsize]))) !=
      1)
    return;

  const char output_file[] = "pu_stats.txt";
  FILE *fout = fopen(output_file, "a");
  if (!fout) return;

  MACROBLOCKD *const xd = &x->e_mbd;
  const int plane = 0;
  struct macroblock_plane *const p = &x->plane[plane];
  struct macroblockd_plane *pd = &xd->plane[plane];
  const int diff_stride = block_size_wide[plane_bsize];
  int bw, bh;
  get_txb_dimensions(xd, plane, plane_bsize, 0, 0, plane_bsize, NULL, NULL, &bw,
                     &bh);
  const int num_samples = bw * bh;
  const int dequant_shift = (is_cur_buf_hbd(xd)) ? xd->bd - 5 : 3;
  const int q_step = p->dequant_QTX[1] >> dequant_shift;
  const int shift = (xd->bd - 8);

  const double rate_norm = (double)rd_stats->rate / num_samples;
  const double dist_norm = (double)rd_stats->dist / num_samples;
  const double rdcost_norm =
      (double)RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist) / num_samples;

  fprintf(fout, "%g %g %g", rate_norm, dist_norm, rdcost_norm);

  const int src_stride = p->src.stride;
  const uint8_t *const src = p->src.buf;
  const int dst_stride = pd->dst.stride;
  const uint8_t *const dst = pd->dst.buf;
  const int16_t *const src_diff = p->src_diff;

  int64_t sse = calculate_sse(xd, p, pd, bw, bh);
  const double sse_norm = (double)sse / num_samples;

  const unsigned int sad =
      cpi->fn_ptr[plane_bsize].sdf(src, src_stride, dst, dst_stride);
  const double sad_norm =
      (double)sad / (1 << num_pels_log2_lookup[plane_bsize]);

  fprintf(fout, " %g %g", sse_norm, sad_norm);

  double sse_norm_arr[4], sad_norm_arr[4];
  get_2x2_normalized_sses_and_sads(cpi, plane_bsize, src, src_stride, dst,
                                   dst_stride, src_diff, diff_stride,
                                   sse_norm_arr, sad_norm_arr);
  if (shift) {
    for (int k = 0; k < 4; ++k) sse_norm_arr[k] /= (1 << (2 * shift));
    for (int k = 0; k < 4; ++k) sad_norm_arr[k] /= (1 << shift);
  }
  for (int i = 0; i < 4; ++i) {
    fprintf(fout, " %g", sse_norm_arr[i]);
  }
  for (int i = 0; i < 4; ++i) {
    fprintf(fout, " %g", sad_norm_arr[i]);
  }

  fprintf(fout, " %d %d %d %d", q_step, x->rdmult, bw, bh);

  int model_rate;
  int64_t model_dist;
  model_rd_sse_fn[MODELRD_CURVFIT](cpi, x, plane_bsize, plane, sse, num_samples,
                                   &model_rate, &model_dist);
  const double model_rdcost_norm =
      (double)RDCOST(x->rdmult, model_rate, model_dist) / num_samples;
  const double model_rate_norm = (double)model_rate / num_samples;
  const double model_dist_norm = (double)model_dist / num_samples;
  fprintf(fout, " %g %g %g", model_rate_norm, model_dist_norm,
          model_rdcost_norm);

  double mean;
  if (is_cur_buf_hbd(xd)) {
    mean = get_highbd_diff_mean(p->src.buf, p->src.stride, pd->dst.buf,
                                pd->dst.stride, bw, bh);
  } else {
    mean = get_diff_mean(p->src.buf, p->src.stride, pd->dst.buf, pd->dst.stride,
                         bw, bh);
  }
  mean /= (1 << shift);
  float hor_corr, vert_corr;
  av1_get_horver_correlation_full(src_diff, diff_stride, bw, bh, &hor_corr,
                                  &vert_corr);
  fprintf(fout, " %g %g %g", mean, hor_corr, vert_corr);

  double hdist[4] = { 0 }, vdist[4] = { 0 };
  get_energy_distribution_fine(cpi, plane_bsize, src, src_stride, dst,
                               dst_stride, 1, hdist, vdist);
  fprintf(fout, " %g %g %g %g %g %g %g %g", hdist[0], hdist[1], hdist[2],
          hdist[3], vdist[0], vdist[1], vdist[2], vdist[3]);

  if (cpi->sf.inter_sf.inter_mode_rd_model_estimation == 1) {
    assert(tile_data->inter_mode_rd_models[plane_bsize].ready);
    const int64_t overall_sse = get_sse(cpi, x);
    int est_residue_cost = 0;
    int64_t est_dist = 0;
    get_est_rate_dist(tile_data, plane_bsize, overall_sse, &est_residue_cost,
                      &est_dist);
    const double est_residue_cost_norm = (double)est_residue_cost / num_samples;
    const double est_dist_norm = (double)est_dist / num_samples;
    const double est_rdcost_norm =
        (double)RDCOST(x->rdmult, est_residue_cost, est_dist) / num_samples;
    fprintf(fout, " %g %g %g", est_residue_cost_norm, est_dist_norm,
            est_rdcost_norm);
  }

  fprintf(fout, "\n");
  fclose(fout);
}
#endif  // CONFIG_COLLECT_RD_STATS >= 2
#endif  // CONFIG_COLLECT_RD_STATS

static int64_t search_txk_type(const AV1_COMP *cpi, MACROBLOCK *x, int plane,
                               int block, int blk_row, int blk_col,
                               BLOCK_SIZE plane_bsize, TX_SIZE tx_size,
                               const TXB_CTX *const txb_ctx,
                               FAST_TX_SEARCH_MODE ftxs_mode,
                               int use_fast_coef_costing, int skip_trellis,
                               int64_t ref_best_rd, RD_STATS *best_rd_stats) {
  const AV1_COMMON *cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;
  struct macroblockd_plane *const pd = &xd->plane[plane];
  MB_MODE_INFO *mbmi = xd->mi[0];
  const int is_inter = is_inter_block(mbmi);
  int64_t best_rd = INT64_MAX;
  uint16_t best_eob = 0;
  TX_TYPE best_tx_type = DCT_DCT;
  TX_TYPE last_tx_type = TX_TYPES;
  const int fast_tx_search = ftxs_mode & FTXS_DCT_AND_1D_DCT_ONLY;
  // The buffer used to swap dqcoeff in macroblockd_plane so we can keep dqcoeff
  // of the best tx_type
  DECLARE_ALIGNED(32, tran_low_t, this_dqcoeff[MAX_SB_SQUARE]);
  tran_low_t *orig_dqcoeff = pd->dqcoeff;
  tran_low_t *best_dqcoeff = this_dqcoeff;
  const int tx_type_map_idx =
      plane ? 0 : blk_row * xd->tx_type_map_stride + blk_col;
  int perform_block_coeff_opt = 0;
  av1_invalid_rd_stats(best_rd_stats);

  TXB_RD_INFO *intra_txb_rd_info = NULL;
  uint16_t cur_joint_ctx = 0;
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  const int within_border =
      mi_row >= xd->tile.mi_row_start &&
      (mi_row + mi_size_high[plane_bsize] < xd->tile.mi_row_end) &&
      mi_col >= xd->tile.mi_col_start &&
      (mi_col + mi_size_wide[plane_bsize] < xd->tile.mi_col_end);
  skip_trellis |=
      cpi->optimize_seg_arr[mbmi->segment_id] == NO_TRELLIS_OPT ||
      cpi->optimize_seg_arr[mbmi->segment_id] == FINAL_PASS_TRELLIS_OPT;
  if (within_border && cpi->sf.tx_sf.use_intra_txb_hash &&
      frame_is_intra_only(cm) && !is_inter && plane == 0 &&
      tx_size_wide[tx_size] == tx_size_high[tx_size]) {
    const uint32_t intra_hash =
        get_intra_txb_hash(x, plane, blk_row, blk_col, plane_bsize, tx_size);
    const int intra_hash_idx =
        find_tx_size_rd_info(&x->txb_rd_record_intra, intra_hash);
    intra_txb_rd_info = &x->txb_rd_record_intra.tx_rd_info[intra_hash_idx];

    cur_joint_ctx = (txb_ctx->dc_sign_ctx << 8) + txb_ctx->txb_skip_ctx;
    if (intra_txb_rd_info->entropy_context == cur_joint_ctx &&
        x->txb_rd_record_intra.tx_rd_info[intra_hash_idx].valid) {
      xd->tx_type_map[tx_type_map_idx] = intra_txb_rd_info->tx_type;
      const TX_TYPE ref_tx_type =
          av1_get_tx_type(xd, get_plane_type(plane), blk_row, blk_col, tx_size,
                          cpi->common.reduced_tx_set_used);
      if (ref_tx_type == intra_txb_rd_info->tx_type) {
        best_rd_stats->rate = intra_txb_rd_info->rate;
        best_rd_stats->dist = intra_txb_rd_info->dist;
        best_rd_stats->sse = intra_txb_rd_info->sse;
        best_rd_stats->skip = intra_txb_rd_info->eob == 0;
        x->plane[plane].eobs[block] = intra_txb_rd_info->eob;
        x->plane[plane].txb_entropy_ctx[block] =
            intra_txb_rd_info->txb_entropy_ctx;
        best_rd = RDCOST(x->rdmult, best_rd_stats->rate, best_rd_stats->dist);
        best_eob = intra_txb_rd_info->eob;
        best_tx_type = intra_txb_rd_info->tx_type;
        perform_block_coeff_opt = intra_txb_rd_info->perform_block_coeff_opt;
        skip_trellis |= !perform_block_coeff_opt;
        update_txk_array(xd, blk_row, blk_col, tx_size, best_tx_type);
        goto RECON_INTRA;
      }
    }
  }

  int rate_cost = 0;
  // if txk_allowed = TX_TYPES, >1 tx types are allowed, else, if txk_allowed <
  // TX_TYPES, only that specific tx type is allowed.
  TX_TYPE txk_allowed = TX_TYPES;
  int txk_map[TX_TYPES] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
  };

  if ((!is_inter && x->use_default_intra_tx_type) ||
      (is_inter && x->use_default_inter_tx_type)) {
    txk_allowed =
        get_default_tx_type(0, xd, tx_size, cpi->is_screen_content_type);
  } else if (x->rd_model == LOW_TXFM_RD) {
    if (plane == 0) txk_allowed = DCT_DCT;
  }

  uint8_t best_txb_ctx = 0;
  const TxSetType tx_set_type =
      av1_get_ext_tx_set_type(tx_size, is_inter, cm->reduced_tx_set_used);

  TX_TYPE uv_tx_type = DCT_DCT;
  if (plane) {
    // tx_type of PLANE_TYPE_UV should be the same as PLANE_TYPE_Y
    uv_tx_type = txk_allowed =
        av1_get_tx_type(xd, get_plane_type(plane), blk_row, blk_col, tx_size,
                        cm->reduced_tx_set_used);
  }
  PREDICTION_MODE intra_dir =
      mbmi->filter_intra_mode_info.use_filter_intra
          ? fimode_to_intradir[mbmi->filter_intra_mode_info.filter_intra_mode]
          : mbmi->mode;
  const uint16_t ext_tx_used_flag =
      cpi->sf.tx_sf.tx_type_search.use_reduced_intra_txset &&
              tx_set_type == EXT_TX_SET_DTT4_IDTX_1DDCT
          ? av1_reduced_intra_tx_used_flag[intra_dir]
          : av1_ext_tx_used_flag[tx_set_type];
  if (xd->lossless[mbmi->segment_id] || txsize_sqr_up_map[tx_size] > TX_32X32 ||
      ext_tx_used_flag == 0x0001 ||
      (is_inter && cpi->oxcf.use_inter_dct_only) ||
      (!is_inter && cpi->oxcf.use_intra_dct_only)) {
    txk_allowed = DCT_DCT;
  }
  uint16_t allowed_tx_mask = 0;  // 1: allow; 0: skip.
  if (txk_allowed < TX_TYPES) {
    allowed_tx_mask = 1 << txk_allowed;
    allowed_tx_mask &= ext_tx_used_flag;
  } else if (fast_tx_search) {
    allowed_tx_mask = 0x0c01;  // V_DCT, H_DCT, DCT_DCT
    allowed_tx_mask &= ext_tx_used_flag;
  } else {
    assert(plane == 0);
    allowed_tx_mask = ext_tx_used_flag;
    int num_allowed = 0;
    const FRAME_UPDATE_TYPE update_type = get_frame_update_type(&cpi->gf_group);
    const int *tx_type_probs = cpi->tx_type_probs[update_type][tx_size];
    int i;

    if (cpi->sf.tx_sf.tx_type_search.prune_tx_type_using_stats) {
      const int thresh = cpi->tx_type_probs_thresh[update_type];
      uint16_t prune = 0;
      int max_prob = -1;
      int max_idx = 0;
      for (i = 0; i < TX_TYPES; i++) {
        if (tx_type_probs[i] > max_prob && (allowed_tx_mask & (1 << i))) {
          max_prob = tx_type_probs[i];
          max_idx = i;
        }
      }

      for (i = 0; i < TX_TYPES; i++) {
        if (tx_type_probs[i] < thresh && i != max_idx) prune |= (1 << i);
      }
      allowed_tx_mask &= (~prune);
    }

    for (i = 0; i < TX_TYPES; i++) {
      if (allowed_tx_mask & (1 << i)) num_allowed++;
    }
    assert(num_allowed > 0);

    int allowed_tx_count = (x->prune_mode == PRUNE_2D_AGGRESSIVE) ? 1 : 5;
    // !fast_tx_search && txk_end != txk_start && plane == 0
    if (x->prune_mode >= PRUNE_2D_ACCURATE && is_inter &&
        num_allowed > allowed_tx_count) {
      prune_tx_2D(x, plane_bsize, tx_size, blk_row, blk_col, tx_set_type,
                  x->prune_mode, txk_map, &allowed_tx_mask);
    }
  }

  if (cpi->oxcf.enable_flip_idtx == 0) {
    for (TX_TYPE tx_type = FLIPADST_DCT; tx_type <= H_FLIPADST; ++tx_type) {
      allowed_tx_mask &= ~(1 << tx_type);
    }
  }

  // Need to have at least one transform type allowed.
  if (allowed_tx_mask == 0) {
    txk_allowed = (plane ? uv_tx_type : DCT_DCT);
    allowed_tx_mask = (1 << txk_allowed);
  }

  const BLOCK_SIZE tx_bsize = txsize_to_bsize[tx_size];
  int64_t block_sse = 0;
  unsigned int block_mse_q8 = UINT_MAX;
  block_sse = pixel_diff_dist(x, plane, blk_row, blk_col, plane_bsize, tx_bsize,
                              &block_mse_q8);
  assert(block_mse_q8 != UINT_MAX);
  if (is_cur_buf_hbd(xd)) {
    block_sse = ROUND_POWER_OF_TWO(block_sse, (xd->bd - 8) * 2);
    block_mse_q8 = ROUND_POWER_OF_TWO(block_mse_q8, (xd->bd - 8) * 2);
  }
  block_sse *= 16;
  // Tranform domain distortion is accurate for higher residuals.
  // TODO(any): Experiment with variance and mean based thresholds
  int use_transform_domain_distortion =
      (x->use_transform_domain_distortion > 0) &&
      (block_mse_q8 >= x->tx_domain_dist_threshold) &&
      // Any 64-pt transforms only preserves half the coefficients.
      // Therefore transform domain distortion is not valid for these
      // transform sizes.
      txsize_sqr_up_map[tx_size] != TX_64X64;
#if CONFIG_DIST_8X8
  if (x->using_dist_8x8) use_transform_domain_distortion = 0;
#endif
  int calc_pixel_domain_distortion_final =
      x->use_transform_domain_distortion == 1 &&
      use_transform_domain_distortion && x->rd_model != LOW_TXFM_RD;
  if (calc_pixel_domain_distortion_final &&
      (txk_allowed < TX_TYPES || allowed_tx_mask == 0x0001))
    calc_pixel_domain_distortion_final = use_transform_domain_distortion = 0;

  const uint16_t *eobs_ptr = x->plane[plane].eobs;

  // Used mse based threshold logic to take decision of R-D of optimization of
  // coeffs. For smaller residuals, coeff optimization would be helpful. For
  // larger residuals, R-D optimization may not be effective.
  // TODO(any): Experiment with variance and mean based thresholds
  perform_block_coeff_opt = (block_mse_q8 <= x->coeff_opt_dist_threshold);
  skip_trellis |= !perform_block_coeff_opt;

  assert(IMPLIES(txk_allowed < TX_TYPES, allowed_tx_mask == 1 << txk_allowed));

  TxfmParam txfm_param;
  QUANT_PARAM quant_param;
  av1_setup_xform(cm, x, tx_size, DCT_DCT, &txfm_param);
  av1_setup_quant(cm, tx_size, !skip_trellis,
                  skip_trellis ? (USE_B_QUANT_NO_TRELLIS ? AV1_XFORM_QUANT_B
                                                         : AV1_XFORM_QUANT_FP)
                               : AV1_XFORM_QUANT_FP,
                  &quant_param);
  int use_qm = !(xd->lossless[mbmi->segment_id] || cm->using_qmatrix == 0);

  for (int idx = 0; idx < TX_TYPES; ++idx) {
    const TX_TYPE tx_type = (TX_TYPE)txk_map[idx];
    if (!(allowed_tx_mask & (1 << tx_type))) continue;
    txfm_param.tx_type = tx_type;
    if (use_qm) {
      av1_setup_qmatrix(cm, x, plane, tx_size, tx_type, &quant_param);
    }
    if (plane == 0) xd->tx_type_map[tx_type_map_idx] = tx_type;
    RD_STATS this_rd_stats;
    av1_invalid_rd_stats(&this_rd_stats);

    av1_xform_quant(x, plane, block, blk_row, blk_col, plane_bsize, &txfm_param,
                    &quant_param);

    if (quant_param.use_optimize_b) {
      if (cpi->sf.rd_sf.optimize_b_precheck && best_rd < INT64_MAX &&
          eobs_ptr[block] >= 4) {
        // Calculate distortion quickly in transform domain.
        dist_block_tx_domain(x, plane, block, tx_size, &this_rd_stats.dist,
                             &this_rd_stats.sse);

        const int64_t best_rd_ = AOMMIN(best_rd, ref_best_rd);
        const int64_t dist_cost_estimate =
            RDCOST(x->rdmult, 0, AOMMIN(this_rd_stats.dist, this_rd_stats.sse));
        if (dist_cost_estimate - (dist_cost_estimate >> 3) > best_rd_) continue;
      }
      av1_optimize_b(cpi, x, plane, block, tx_size, tx_type, txb_ctx,
                     cpi->sf.rd_sf.trellis_eob_fast, &rate_cost);
    } else {
      rate_cost =
          av1_cost_coeffs(x, plane, block, tx_size, tx_type, txb_ctx,
                          use_fast_coef_costing, cm->reduced_tx_set_used);
    }

    // If rd cost based on coeff rate is more than best_rd, skip the calculation
    // of distortion
    int64_t tmp_rd = RDCOST(x->rdmult, rate_cost, 0);
    if (tmp_rd > best_rd) continue;
    if (eobs_ptr[block] == 0) {
      // When eob is 0, pixel domain distortion is more efficient and accurate.
      this_rd_stats.dist = this_rd_stats.sse = block_sse;
    } else if (use_transform_domain_distortion) {
      dist_block_tx_domain(x, plane, block, tx_size, &this_rd_stats.dist,
                           &this_rd_stats.sse);
    } else {
      int64_t sse_diff = INT64_MAX;
      // high_energy threshold assumes that every pixel within a txfm block
      // has a residue energy of at least 25% of the maximum, i.e. 128 * 128
      // for 8 bit, then the threshold is scaled based on input bit depth.
      const int64_t high_energy_thresh =
          ((int64_t)128 * 128 * tx_size_2d[tx_size]) << ((xd->bd - 8) * 2);
      const int is_high_energy = (block_sse >= high_energy_thresh);
      if (tx_size == TX_64X64 || is_high_energy) {
        // Because 3 out 4 quadrants of transform coefficients are forced to
        // zero, the inverse transform has a tendency to overflow. sse_diff
        // is effectively the energy of those 3 quadrants, here we use it
        // to decide if we should do pixel domain distortion. If the energy
        // is mostly in first quadrant, then it is unlikely that we have
        // overflow issue in inverse transform.
        dist_block_tx_domain(x, plane, block, tx_size, &this_rd_stats.dist,
                             &this_rd_stats.sse);
        sse_diff = block_sse - this_rd_stats.sse;
      }
      if (tx_size != TX_64X64 || !is_high_energy ||
          (sse_diff * 2) < this_rd_stats.sse) {
        const int64_t tx_domain_dist = this_rd_stats.dist;
        this_rd_stats.dist = dist_block_px_domain(
            cpi, x, plane, plane_bsize, block, blk_row, blk_col, tx_size);
        // For high energy blocks, occasionally, the pixel domain distortion
        // can be artificially low due to clamping at reconstruction stage
        // even when inverse transform output is hugely different from the
        // actual residue.
        if (is_high_energy && this_rd_stats.dist < tx_domain_dist)
          this_rd_stats.dist = tx_domain_dist;
      } else {
        this_rd_stats.dist += sse_diff;
      }
      this_rd_stats.sse = block_sse;
    }

    this_rd_stats.rate = rate_cost;

    const int64_t rd =
        RDCOST(x->rdmult, this_rd_stats.rate, this_rd_stats.dist);

    if (rd < best_rd) {
      best_rd = rd;
      *best_rd_stats = this_rd_stats;
      best_tx_type = tx_type;
      best_txb_ctx = x->plane[plane].txb_entropy_ctx[block];
      best_eob = x->plane[plane].eobs[block];
      last_tx_type = best_tx_type;

      // Swap qcoeff and dqcoeff buffers
      tran_low_t *const tmp_dqcoeff = best_dqcoeff;
      best_dqcoeff = pd->dqcoeff;
      pd->dqcoeff = tmp_dqcoeff;
    }

#if CONFIG_COLLECT_RD_STATS == 1
    if (plane == 0) {
      PrintTransformUnitStats(cpi, x, &this_rd_stats, blk_row, blk_col,
                              plane_bsize, tx_size, tx_type, rd);
    }
#endif  // CONFIG_COLLECT_RD_STATS == 1

#if COLLECT_TX_SIZE_DATA
    // Generate small sample to restrict output size.
    static unsigned int seed = 21743;
    if (lcg_rand16(&seed) % 200 == 0) {
      FILE *fp = NULL;

      if (within_border) {
        fp = fopen(av1_tx_size_data_output_file, "a");
      }

      if (fp) {
        // Transform info and RD
        const int txb_w = tx_size_wide[tx_size];
        const int txb_h = tx_size_high[tx_size];

        // Residue signal.
        const int diff_stride = block_size_wide[plane_bsize];
        struct macroblock_plane *const p = &x->plane[plane];
        const int16_t *src_diff =
            &p->src_diff[(blk_row * diff_stride + blk_col) * 4];

        for (int r = 0; r < txb_h; ++r) {
          for (int c = 0; c < txb_w; ++c) {
            fprintf(fp, "%d,", src_diff[c]);
          }
          src_diff += diff_stride;
        }

        fprintf(fp, "%d,%d,%d,%" PRId64, txb_w, txb_h, tx_type, rd);
        fprintf(fp, "\n");
        fclose(fp);
      }
    }
#endif  // COLLECT_TX_SIZE_DATA

    if (cpi->sf.tx_sf.adaptive_txb_search_level) {
      if ((best_rd - (best_rd >> cpi->sf.tx_sf.adaptive_txb_search_level)) >
          ref_best_rd) {
        break;
      }
    }

    // Skip transform type search when we found the block has been quantized to
    // all zero and at the same time, it has better rdcost than doing transform.
    if (cpi->sf.tx_sf.tx_type_search.skip_tx_search && !best_eob) break;
  }

  assert(best_rd != INT64_MAX);

  best_rd_stats->skip = best_eob == 0;
  if (plane == 0) update_txk_array(xd, blk_row, blk_col, tx_size, best_tx_type);
  x->plane[plane].txb_entropy_ctx[block] = best_txb_ctx;
  x->plane[plane].eobs[block] = best_eob;

  pd->dqcoeff = best_dqcoeff;

  if (calc_pixel_domain_distortion_final && best_eob) {
    best_rd_stats->dist = dist_block_px_domain(
        cpi, x, plane, plane_bsize, block, blk_row, blk_col, tx_size);
    best_rd_stats->sse = block_sse;
  }

  if (intra_txb_rd_info != NULL) {
    intra_txb_rd_info->valid = 1;
    intra_txb_rd_info->entropy_context = cur_joint_ctx;
    intra_txb_rd_info->rate = best_rd_stats->rate;
    intra_txb_rd_info->dist = best_rd_stats->dist;
    intra_txb_rd_info->sse = best_rd_stats->sse;
    intra_txb_rd_info->eob = best_eob;
    intra_txb_rd_info->txb_entropy_ctx = best_txb_ctx;
    intra_txb_rd_info->perform_block_coeff_opt = perform_block_coeff_opt;
    if (plane == 0) intra_txb_rd_info->tx_type = best_tx_type;
  }

RECON_INTRA:
  if (!is_inter && best_eob &&
      (blk_row + tx_size_high_unit[tx_size] < mi_size_high[plane_bsize] ||
       blk_col + tx_size_wide_unit[tx_size] < mi_size_wide[plane_bsize])) {
    // intra mode needs decoded result such that the next transform block
    // can use it for prediction.
    // if the last search tx_type is the best tx_type, we don't need to
    // do this again
    if (best_tx_type != last_tx_type) {
      TxfmParam txfm_param_intra;
      QUANT_PARAM quant_param_intra;
      av1_setup_xform(cm, x, tx_size, best_tx_type, &txfm_param_intra);
      av1_setup_quant(cm, tx_size, !skip_trellis,
                      skip_trellis
                          ? (USE_B_QUANT_NO_TRELLIS ? AV1_XFORM_QUANT_B
                                                    : AV1_XFORM_QUANT_FP)
                          : AV1_XFORM_QUANT_FP,
                      &quant_param_intra);
      av1_setup_qmatrix(cm, x, plane, tx_size, best_tx_type,
                        &quant_param_intra);
      av1_xform_quant(x, plane, block, blk_row, blk_col, plane_bsize,
                      &txfm_param_intra, &quant_param_intra);
      if (quant_param_intra.use_optimize_b) {
        av1_optimize_b(cpi, x, plane, block, tx_size, best_tx_type, txb_ctx,
                       cpi->sf.rd_sf.trellis_eob_fast, &rate_cost);
      }
    }

    inverse_transform_block_facade(xd, plane, block, blk_row, blk_col,
                                   x->plane[plane].eobs[block],
                                   cm->reduced_tx_set_used);

    // This may happen because of hash collision. The eob stored in the hash
    // table is non-zero, but the real eob is zero. We need to make sure tx_type
    // is DCT_DCT in this case.
    if (plane == 0 && x->plane[plane].eobs[block] == 0 &&
        best_tx_type != DCT_DCT) {
      update_txk_array(xd, blk_row, blk_col, tx_size, DCT_DCT);
    }
  }
  pd->dqcoeff = orig_dqcoeff;

  return best_rd;
}

static AOM_INLINE void block_rd_txfm(int plane, int block, int blk_row,
                                     int blk_col, BLOCK_SIZE plane_bsize,
                                     TX_SIZE tx_size, void *arg) {
  struct rdcost_block_args *args = arg;
  MACROBLOCK *const x = args->x;
  MACROBLOCKD *const xd = &x->e_mbd;
  const int is_inter = is_inter_block(xd->mi[0]);
  const AV1_COMP *cpi = args->cpi;
  ENTROPY_CONTEXT *a = args->t_above + blk_col;
  ENTROPY_CONTEXT *l = args->t_left + blk_row;
  const AV1_COMMON *cm = &cpi->common;
  RD_STATS this_rd_stats;

  av1_init_rd_stats(&this_rd_stats);

  if (args->exit_early) {
    args->incomplete_exit = 1;
    return;
  }

  if (!is_inter) {
    av1_predict_intra_block_facade(cm, xd, plane, blk_col, blk_row, tx_size);
    av1_subtract_txb(x, plane, plane_bsize, blk_col, blk_row, tx_size);
  }
  TXB_CTX txb_ctx;
  get_txb_ctx(plane_bsize, tx_size, plane, a, l, &txb_ctx);
  search_txk_type(cpi, x, plane, block, blk_row, blk_col, plane_bsize, tx_size,
                  &txb_ctx, args->ftxs_mode, args->use_fast_coef_costing,
                  args->skip_trellis, args->best_rd - args->this_rd,
                  &this_rd_stats);

  if (plane == AOM_PLANE_Y && xd->cfl.store_y) {
    assert(!is_inter || plane_bsize < BLOCK_8X8);
    cfl_store_tx(xd, blk_row, blk_col, tx_size, plane_bsize);
  }

#if CONFIG_RD_DEBUG
  av1_update_txb_coeff_cost(&this_rd_stats, plane, tx_size, blk_row, blk_col,
                            this_rd_stats.rate);
#endif  // CONFIG_RD_DEBUG
  av1_set_txb_context(x, plane, block, tx_size, a, l);

  const int blk_idx =
      blk_row * (block_size_wide[plane_bsize] >> MI_SIZE_LOG2) + blk_col;

  if (plane == 0)
    set_blk_skip(x, plane, blk_idx, x->plane[plane].eobs[block] == 0);
  else
    set_blk_skip(x, plane, blk_idx, 0);

  int64_t rd;
  if (is_inter) {
    const int64_t rd1 =
        RDCOST(x->rdmult, this_rd_stats.rate, this_rd_stats.dist);
    const int64_t rd2 = RDCOST(x->rdmult, 0, this_rd_stats.sse);

    // TODO(jingning): temporarily enabled only for luma component
    rd = AOMMIN(rd1, rd2);
    this_rd_stats.skip &= !x->plane[plane].eobs[block];
  } else {
    // Signal non-skip for Intra blocks
    rd = RDCOST(x->rdmult, this_rd_stats.rate, this_rd_stats.dist);
    this_rd_stats.skip = 0;
  }

  av1_merge_rd_stats(&args->rd_stats, &this_rd_stats);

  args->this_rd += rd;

  if (args->this_rd > args->best_rd) args->exit_early = 1;
}

static AOM_INLINE void txfm_rd_in_plane(MACROBLOCK *x, const AV1_COMP *cpi,
                                        RD_STATS *rd_stats, int64_t ref_best_rd,
                                        int64_t this_rd, int plane,
                                        BLOCK_SIZE plane_bsize, TX_SIZE tx_size,
                                        int use_fast_coef_casting,
                                        FAST_TX_SEARCH_MODE ftxs_mode,
                                        int skip_trellis) {
  if (!cpi->oxcf.enable_tx64 && txsize_sqr_up_map[tx_size] == TX_64X64) {
    av1_invalid_rd_stats(rd_stats);
    return;
  }

  MACROBLOCKD *const xd = &x->e_mbd;
  const struct macroblockd_plane *const pd = &xd->plane[plane];
  struct rdcost_block_args args;
  av1_zero(args);
  args.x = x;
  args.cpi = cpi;
  args.best_rd = ref_best_rd;
  args.use_fast_coef_costing = use_fast_coef_casting;
  args.ftxs_mode = ftxs_mode;
  args.this_rd = this_rd;
  args.skip_trellis = skip_trellis;
  av1_init_rd_stats(&args.rd_stats);

  if (plane == 0) xd->mi[0]->tx_size = tx_size;

  av1_get_entropy_contexts(plane_bsize, pd, args.t_above, args.t_left);

  if (args.this_rd > args.best_rd) {
    args.exit_early = 1;
  }

  av1_foreach_transformed_block_in_plane(xd, plane_bsize, plane, block_rd_txfm,
                                         &args);

  MB_MODE_INFO *const mbmi = xd->mi[0];
  const int is_inter = is_inter_block(mbmi);
  const int invalid_rd = is_inter ? args.incomplete_exit : args.exit_early;

  if (invalid_rd) {
    av1_invalid_rd_stats(rd_stats);
  } else {
    *rd_stats = args.rd_stats;
  }
}

static int tx_size_cost(const MACROBLOCK *const x, BLOCK_SIZE bsize,
                        TX_SIZE tx_size) {
  assert(bsize == x->e_mbd.mi[0]->sb_type);
  if (x->tx_mode_search_type != TX_MODE_SELECT || !block_signals_txsize(bsize))
    return 0;

  const int32_t tx_size_cat = bsize_to_tx_size_cat(bsize);
  const int depth = tx_size_to_depth(tx_size, bsize);
  const MACROBLOCKD *const xd = &x->e_mbd;
  const int tx_size_ctx = get_tx_size_context(xd);
  return x->tx_size_cost[tx_size_cat][tx_size_ctx][depth];
}

static int64_t txfm_yrd(const AV1_COMP *const cpi, MACROBLOCK *x,
                        RD_STATS *rd_stats, int64_t ref_best_rd, BLOCK_SIZE bs,
                        TX_SIZE tx_size, FAST_TX_SEARCH_MODE ftxs_mode,
                        int skip_trellis) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  int64_t rd = INT64_MAX;
  const int skip_ctx = av1_get_skip_context(xd);
  int s0, s1;
  const int is_inter = is_inter_block(mbmi);
  const int tx_select = x->tx_mode_search_type == TX_MODE_SELECT &&
                        block_signals_txsize(mbmi->sb_type);
  int ctx = txfm_partition_context(
      xd->above_txfm_context, xd->left_txfm_context, mbmi->sb_type, tx_size);
  const int r_tx_size =
      is_inter ? x->txfm_partition_cost[ctx][0] : tx_size_cost(x, bs, tx_size);

  assert(IMPLIES(is_rect_tx(tx_size), is_rect_tx_allowed_bsize(bs)));

  s0 = x->skip_cost[skip_ctx][0];
  s1 = x->skip_cost[skip_ctx][1];

  int64_t skip_rd = INT64_MAX;
  int64_t this_rd = RDCOST(x->rdmult, s0 + r_tx_size * tx_select, 0);

  if (is_inter) skip_rd = RDCOST(x->rdmult, s1, 0);

  mbmi->tx_size = tx_size;
  txfm_rd_in_plane(
      x, cpi, rd_stats, ref_best_rd, AOMMIN(this_rd, skip_rd), AOM_PLANE_Y, bs,
      tx_size, cpi->sf.rd_sf.use_fast_coef_costing, ftxs_mode, skip_trellis);
  if (rd_stats->rate == INT_MAX) return INT64_MAX;

  // rdstats->rate should include all the rate except skip/non-skip cost as the
  // same is accounted in the caller functions after rd evaluation of all
  // planes. However the decisions should be done after considering the
  // skip/non-skip header cost
  if (rd_stats->skip && is_inter) {
    rd = RDCOST(x->rdmult, s1, rd_stats->sse);
  } else {
    // Intra blocks are always signalled as non-skip
    rd = RDCOST(x->rdmult, rd_stats->rate + s0 + r_tx_size * tx_select,
                rd_stats->dist);
    rd_stats->rate += r_tx_size * tx_select;
  }
  if (is_inter && !xd->lossless[xd->mi[0]->segment_id]) {
    int64_t temp_skip_rd = RDCOST(x->rdmult, s1, rd_stats->sse);
    if (temp_skip_rd <= rd) {
      rd = temp_skip_rd;
      rd_stats->rate = 0;
      rd_stats->dist = rd_stats->sse;
      rd_stats->skip = 1;
    }
  }

  return rd;
}

static int64_t estimate_yrd_for_sb(const AV1_COMP *const cpi, BLOCK_SIZE bs,
                                   MACROBLOCK *x, int64_t ref_best_rd,
                                   RD_STATS *rd_stats) {
  MACROBLOCKD *const xd = &x->e_mbd;
  if (ref_best_rd < 0) return INT64_MAX;
  av1_subtract_plane(x, bs, 0);
  x->rd_model = LOW_TXFM_RD;
  int skip_trellis = cpi->optimize_seg_arr[xd->mi[0]->segment_id] ==
                     NO_ESTIMATE_YRD_TRELLIS_OPT;
  const int64_t rd =
      txfm_yrd(cpi, x, rd_stats, ref_best_rd, bs, max_txsize_rect_lookup[bs],
               FTXS_NONE, skip_trellis);
  x->rd_model = FULL_TXFM_RD;
  if (rd != INT64_MAX) {
    const int skip_ctx = av1_get_skip_context(xd);
    if (rd_stats->skip) {
      const int s1 = x->skip_cost[skip_ctx][1];
      rd_stats->rate = s1;
    } else {
      const int s0 = x->skip_cost[skip_ctx][0];
      rd_stats->rate += s0;
    }
  }
  return rd;
}

static AOM_INLINE void choose_largest_tx_size(const AV1_COMP *const cpi,
                                              MACROBLOCK *x, RD_STATS *rd_stats,
                                              int64_t ref_best_rd,
                                              BLOCK_SIZE bs) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  mbmi->tx_size = tx_size_from_tx_mode(bs, x->tx_mode_search_type);

  // If tx64 is not enabled, we need to go down to the next available size
  if (!cpi->oxcf.enable_tx64) {
    static const TX_SIZE tx_size_max_32[TX_SIZES_ALL] = {
      TX_4X4,    // 4x4 transform
      TX_8X8,    // 8x8 transform
      TX_16X16,  // 16x16 transform
      TX_32X32,  // 32x32 transform
      TX_32X32,  // 64x64 transform
      TX_4X8,    // 4x8 transform
      TX_8X4,    // 8x4 transform
      TX_8X16,   // 8x16 transform
      TX_16X8,   // 16x8 transform
      TX_16X32,  // 16x32 transform
      TX_32X16,  // 32x16 transform
      TX_32X32,  // 32x64 transform
      TX_32X32,  // 64x32 transform
      TX_4X16,   // 4x16 transform
      TX_16X4,   // 16x4 transform
      TX_8X32,   // 8x32 transform
      TX_32X8,   // 32x8 transform
      TX_16X32,  // 16x64 transform
      TX_32X16,  // 64x16 transform
    };

    mbmi->tx_size = tx_size_max_32[mbmi->tx_size];
  }

  const int skip_ctx = av1_get_skip_context(xd);
  int s0, s1;

  s0 = x->skip_cost[skip_ctx][0];
  s1 = x->skip_cost[skip_ctx][1];

  int64_t skip_rd = INT64_MAX;
  int64_t this_rd = RDCOST(x->rdmult, s0, 0);

  // Skip RDcost is used only for Inter blocks
  if (is_inter_block(xd->mi[0])) skip_rd = RDCOST(x->rdmult, s1, 0);

  txfm_rd_in_plane(x, cpi, rd_stats, ref_best_rd, AOMMIN(this_rd, skip_rd),
                   AOM_PLANE_Y, bs, mbmi->tx_size,
                   cpi->sf.rd_sf.use_fast_coef_costing, FTXS_NONE, 0);
}

static AOM_INLINE void choose_smallest_tx_size(const AV1_COMP *const cpi,
                                               MACROBLOCK *x,
                                               RD_STATS *rd_stats,
                                               int64_t ref_best_rd,
                                               BLOCK_SIZE bs) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];

  mbmi->tx_size = TX_4X4;
  // TODO(any) : Pass this_rd based on skip/non-skip cost
  txfm_rd_in_plane(x, cpi, rd_stats, ref_best_rd, 0, 0, bs, mbmi->tx_size,
                   cpi->sf.rd_sf.use_fast_coef_costing, FTXS_NONE, 0);
}

static int get_search_init_depth(int mi_width, int mi_height, int is_inter,
                                 const SPEED_FEATURES *sf,
                                 int tx_size_search_method) {
  if (tx_size_search_method == USE_LARGESTALL) return MAX_VARTX_DEPTH;

  if (sf->tx_sf.tx_size_search_lgr_block) {
    if (mi_width > mi_size_wide[BLOCK_64X64] ||
        mi_height > mi_size_high[BLOCK_64X64])
      return MAX_VARTX_DEPTH;
  }

  if (is_inter) {
    return (mi_height != mi_width)
               ? sf->tx_sf.inter_tx_size_search_init_depth_rect
               : sf->tx_sf.inter_tx_size_search_init_depth_sqr;
  } else {
    return (mi_height != mi_width)
               ? sf->tx_sf.intra_tx_size_search_init_depth_rect
               : sf->tx_sf.intra_tx_size_search_init_depth_sqr;
  }
}

static AOM_INLINE void choose_tx_size_type_from_rd(const AV1_COMP *const cpi,
                                                   MACROBLOCK *x,
                                                   RD_STATS *rd_stats,
                                                   int64_t ref_best_rd,
                                                   BLOCK_SIZE bs) {
  av1_invalid_rd_stats(rd_stats);

  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const TX_SIZE max_rect_tx_size = max_txsize_rect_lookup[bs];
  const int tx_select = x->tx_mode_search_type == TX_MODE_SELECT;
  int start_tx;
  int depth, init_depth;

  if (tx_select) {
    start_tx = max_rect_tx_size;
    init_depth = get_search_init_depth(mi_size_wide[bs], mi_size_high[bs],
                                       is_inter_block(mbmi), &cpi->sf,
                                       x->tx_size_search_method);
  } else {
    const TX_SIZE chosen_tx_size =
        tx_size_from_tx_mode(bs, x->tx_mode_search_type);
    start_tx = chosen_tx_size;
    init_depth = MAX_TX_DEPTH;
  }

  uint8_t best_txk_type_map[MAX_MIB_SIZE * MAX_MIB_SIZE];
  uint8_t best_blk_skip[MAX_MIB_SIZE * MAX_MIB_SIZE];
  TX_SIZE best_tx_size = max_rect_tx_size;
  int64_t best_rd = INT64_MAX;
  const int n4 = bsize_to_num_blk(bs);
  x->rd_model = FULL_TXFM_RD;
  depth = init_depth;
  int64_t rd[MAX_TX_DEPTH + 1] = { INT64_MAX, INT64_MAX, INT64_MAX };
  for (int n = start_tx; depth <= MAX_TX_DEPTH;
       depth++, n = sub_tx_size_map[n]) {
#if CONFIG_DIST_8X8
    if (x->using_dist_8x8) {
      if (tx_size_wide[n] < 8 || tx_size_high[n] < 8) continue;
    }
#endif
    if (!cpi->oxcf.enable_tx64 && txsize_sqr_up_map[n] == TX_64X64) continue;

    RD_STATS this_rd_stats;
    rd[depth] =
        txfm_yrd(cpi, x, &this_rd_stats, ref_best_rd, bs, n, FTXS_NONE, 0);

    if (rd[depth] < best_rd) {
      av1_copy_array(best_blk_skip, x->blk_skip, n4);
      av1_copy_array(best_txk_type_map, xd->tx_type_map, n4);
      best_tx_size = n;
      best_rd = rd[depth];
      *rd_stats = this_rd_stats;
    }
    if (n == TX_4X4) break;
    // If we are searching three depths, prune the smallest size depending
    // on rd results for the first two depths for low contrast blocks.
    if (depth > init_depth && depth != MAX_TX_DEPTH &&
        x->source_variance < 256) {
      if (rd[depth - 1] != INT64_MAX && rd[depth] > rd[depth - 1]) break;
    }
  }

  if (rd_stats->rate != INT_MAX) {
    mbmi->tx_size = best_tx_size;
    av1_copy_array(xd->tx_type_map, best_txk_type_map, n4);
    av1_copy_array(x->blk_skip, best_blk_skip, n4);
  }
}

// origin_threshold * 128 / 100
static const uint32_t skip_pred_threshold[3][BLOCK_SIZES_ALL] = {
  {
      64, 64, 64, 70, 60, 60, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 64, 64, 70, 70, 68, 68,
  },
  {
      88, 88, 88, 86, 87, 87, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 88, 88, 86, 86, 68, 68,
  },
  {
      90, 93, 93, 90, 93, 93, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 90, 90, 90, 90, 74, 74,
  },
};

// lookup table for predict_skip_flag
// int max_tx_size = max_txsize_rect_lookup[bsize];
// if (tx_size_high[max_tx_size] > 16 || tx_size_wide[max_tx_size] > 16)
//   max_tx_size = AOMMIN(max_txsize_lookup[bsize], TX_16X16);
static const TX_SIZE max_predict_sf_tx_size[BLOCK_SIZES_ALL] = {
  TX_4X4,   TX_4X8,   TX_8X4,   TX_8X8,   TX_8X16,  TX_16X8,
  TX_16X16, TX_16X16, TX_16X16, TX_16X16, TX_16X16, TX_16X16,
  TX_16X16, TX_16X16, TX_16X16, TX_16X16, TX_4X16,  TX_16X4,
  TX_8X8,   TX_8X8,   TX_16X16, TX_16X16,
};

// Uses simple features on top of DCT coefficients to quickly predict
// whether optimal RD decision is to skip encoding the residual.
// The sse value is stored in dist.
static int predict_skip_flag(MACROBLOCK *x, BLOCK_SIZE bsize, int64_t *dist,
                             int reduced_tx_set) {
  const int bw = block_size_wide[bsize];
  const int bh = block_size_high[bsize];
  const MACROBLOCKD *xd = &x->e_mbd;
  const int16_t dc_q = av1_dc_quant_QTX(x->qindex, 0, xd->bd);

  *dist = pixel_diff_dist(x, 0, 0, 0, bsize, bsize, NULL);

  const int64_t mse = *dist / bw / bh;
  // Normalized quantizer takes the transform upscaling factor (8 for tx size
  // smaller than 32) into account.
  const int16_t normalized_dc_q = dc_q >> 3;
  const int64_t mse_thresh = (int64_t)normalized_dc_q * normalized_dc_q / 8;
  // For faster early skip decision, use dist to compare against threshold so
  // that quality risk is less for the skip=1 decision. Otherwise, use mse
  // since the fwd_txfm coeff checks will take care of quality
  // TODO(any): Use dist to return 0 when predict_skip_level is 1
  int64_t pred_err = (x->predict_skip_level >= 2) ? *dist : mse;
  // Predict not to skip when error is larger than threshold.
  if (pred_err > mse_thresh) return 0;
  // Return as skip otherwise for aggressive early skip
  else if (x->predict_skip_level >= 2)
    return 1;

  const int max_tx_size = max_predict_sf_tx_size[bsize];
  const int tx_h = tx_size_high[max_tx_size];
  const int tx_w = tx_size_wide[max_tx_size];
  DECLARE_ALIGNED(32, tran_low_t, coefs[32 * 32]);
  TxfmParam param;
  param.tx_type = DCT_DCT;
  param.tx_size = max_tx_size;
  param.bd = xd->bd;
  param.is_hbd = is_cur_buf_hbd(xd);
  param.lossless = 0;
  param.tx_set_type = av1_get_ext_tx_set_type(
      param.tx_size, is_inter_block(xd->mi[0]), reduced_tx_set);
  const int bd_idx = (xd->bd == 8) ? 0 : ((xd->bd == 10) ? 1 : 2);
  const uint32_t max_qcoef_thresh = skip_pred_threshold[bd_idx][bsize];
  const int16_t *src_diff = x->plane[0].src_diff;
  const int n_coeff = tx_w * tx_h;
  const int16_t ac_q = av1_ac_quant_QTX(x->qindex, 0, xd->bd);
  const uint32_t dc_thresh = max_qcoef_thresh * dc_q;
  const uint32_t ac_thresh = max_qcoef_thresh * ac_q;
  for (int row = 0; row < bh; row += tx_h) {
    for (int col = 0; col < bw; col += tx_w) {
      av1_fwd_txfm(src_diff + col, coefs, bw, &param);
      // Operating on TX domain, not pixels; we want the QTX quantizers
      const uint32_t dc_coef = (((uint32_t)abs(coefs[0])) << 7);
      if (dc_coef >= dc_thresh) return 0;
      for (int i = 1; i < n_coeff; ++i) {
        const uint32_t ac_coef = (((uint32_t)abs(coefs[i])) << 7);
        if (ac_coef >= ac_thresh) return 0;
      }
    }
    src_diff += tx_h * bw;
  }
  return 1;
}

// Used to set proper context for early termination with skip = 1.
static AOM_INLINE void set_skip_flag(MACROBLOCK *x, RD_STATS *rd_stats,
                                     int bsize, int64_t dist) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const int n4 = bsize_to_num_blk(bsize);
  const TX_SIZE tx_size = max_txsize_rect_lookup[bsize];
  memset(xd->tx_type_map, DCT_DCT, sizeof(xd->tx_type_map[0]) * n4);
  memset(mbmi->inter_tx_size, tx_size, sizeof(mbmi->inter_tx_size));
  mbmi->tx_size = tx_size;
  for (int i = 0; i < n4; ++i) set_blk_skip(x, 0, i, 1);
  rd_stats->skip = 1;
  if (is_cur_buf_hbd(xd)) dist = ROUND_POWER_OF_TWO(dist, (xd->bd - 8) * 2);
  rd_stats->dist = rd_stats->sse = (dist << 4);
  // Though decision is to make the block as skip based on luma stats,
  // it is possible that block becomes non skip after chroma rd. In addition
  // intermediate non skip costs calculated by caller function will be
  // incorrect, if rate is set as  zero (i.e., if zero_blk_rate is not
  // accounted). Hence intermediate rate is populated to code the luma tx blks
  // as skip, the caller function based on final rd decision (i.e., skip vs
  // non-skip) sets the final rate accordingly. Here the rate populated
  // corresponds to coding all the tx blocks with zero_blk_rate (based on max tx
  // size possible) in the current block. Eg: For 128*128 block, rate would be
  // 4 * zero_blk_rate where zero_blk_rate corresponds to coding of one 64x64 tx
  // block as 'all zeros'
  ENTROPY_CONTEXT ctxa[MAX_MIB_SIZE];
  ENTROPY_CONTEXT ctxl[MAX_MIB_SIZE];
  av1_get_entropy_contexts(bsize, &xd->plane[0], ctxa, ctxl);
  ENTROPY_CONTEXT *ta = ctxa;
  ENTROPY_CONTEXT *tl = ctxl;
  const TX_SIZE txs_ctx = get_txsize_entropy_ctx(tx_size);
  TXB_CTX txb_ctx;
  get_txb_ctx(bsize, tx_size, 0, ta, tl, &txb_ctx);
  const int zero_blk_rate = x->coeff_costs[txs_ctx][PLANE_TYPE_Y]
                                .txb_skip_cost[txb_ctx.txb_skip_ctx][1];
  rd_stats->rate = zero_blk_rate *
                   (block_size_wide[bsize] >> tx_size_wide_log2[tx_size]) *
                   (block_size_high[bsize] >> tx_size_high_log2[tx_size]);
}

static INLINE uint32_t get_block_residue_hash(MACROBLOCK *x, BLOCK_SIZE bsize) {
  const int rows = block_size_high[bsize];
  const int cols = block_size_wide[bsize];
  const int16_t *diff = x->plane[0].src_diff;
  const uint32_t hash = av1_get_crc32c_value(&x->mb_rd_record.crc_calculator,
                                             (uint8_t *)diff, 2 * rows * cols);
  return (hash << 5) + bsize;
}

static AOM_INLINE void save_tx_rd_info(int n4, uint32_t hash,
                                       const MACROBLOCK *const x,
                                       const RD_STATS *const rd_stats,
                                       MB_RD_RECORD *tx_rd_record) {
  int index;
  if (tx_rd_record->num < RD_RECORD_BUFFER_LEN) {
    index =
        (tx_rd_record->index_start + tx_rd_record->num) % RD_RECORD_BUFFER_LEN;
    ++tx_rd_record->num;
  } else {
    index = tx_rd_record->index_start;
    tx_rd_record->index_start =
        (tx_rd_record->index_start + 1) % RD_RECORD_BUFFER_LEN;
  }
  MB_RD_INFO *const tx_rd_info = &tx_rd_record->tx_rd_info[index];
  const MACROBLOCKD *const xd = &x->e_mbd;
  const MB_MODE_INFO *const mbmi = xd->mi[0];
  tx_rd_info->hash_value = hash;
  tx_rd_info->tx_size = mbmi->tx_size;
  memcpy(tx_rd_info->blk_skip, x->blk_skip,
         sizeof(tx_rd_info->blk_skip[0]) * n4);
  av1_copy(tx_rd_info->inter_tx_size, mbmi->inter_tx_size);
  av1_copy_array(tx_rd_info->tx_type_map, xd->tx_type_map, n4);
  tx_rd_info->rd_stats = *rd_stats;
}

static AOM_INLINE void fetch_tx_rd_info(int n4,
                                        const MB_RD_INFO *const tx_rd_info,
                                        RD_STATS *const rd_stats,
                                        MACROBLOCK *const x) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  mbmi->tx_size = tx_rd_info->tx_size;
  memcpy(x->blk_skip, tx_rd_info->blk_skip,
         sizeof(tx_rd_info->blk_skip[0]) * n4);
  av1_copy(mbmi->inter_tx_size, tx_rd_info->inter_tx_size);
  av1_copy_array(xd->tx_type_map, tx_rd_info->tx_type_map, n4);
  *rd_stats = tx_rd_info->rd_stats;
}

static INLINE int32_t find_mb_rd_info(const MB_RD_RECORD *const mb_rd_record,
                                      const int64_t ref_best_rd,
                                      const uint32_t hash) {
  int32_t match_index = -1;
  if (ref_best_rd != INT64_MAX) {
    for (int i = 0; i < mb_rd_record->num; ++i) {
      const int index = (mb_rd_record->index_start + i) % RD_RECORD_BUFFER_LEN;
      // If there is a match in the tx_rd_record, fetch the RD decision and
      // terminate early.
      if (mb_rd_record->tx_rd_info[index].hash_value == hash) {
        match_index = index;
        break;
      }
    }
  }
  return match_index;
}

static AOM_INLINE void super_block_yrd(const AV1_COMP *const cpi, MACROBLOCK *x,
                                       RD_STATS *rd_stats, BLOCK_SIZE bs,
                                       int64_t ref_best_rd) {
  MACROBLOCKD *xd = &x->e_mbd;
  av1_init_rd_stats(rd_stats);
  int is_inter = is_inter_block(xd->mi[0]);
  assert(bs == xd->mi[0]->sb_type);

  const int mi_row = -xd->mb_to_top_edge >> (3 + MI_SIZE_LOG2);
  const int mi_col = -xd->mb_to_left_edge >> (3 + MI_SIZE_LOG2);

  uint32_t hash = 0;
  int32_t match_index = -1;
  MB_RD_RECORD *mb_rd_record = NULL;
  const int within_border = mi_row >= xd->tile.mi_row_start &&
                            (mi_row + mi_size_high[bs] < xd->tile.mi_row_end) &&
                            mi_col >= xd->tile.mi_col_start &&
                            (mi_col + mi_size_wide[bs] < xd->tile.mi_col_end);
  const int is_mb_rd_hash_enabled =
      (within_border && cpi->sf.rd_sf.use_mb_rd_hash && is_inter);
  const int n4 = bsize_to_num_blk(bs);
  if (is_mb_rd_hash_enabled) {
    hash = get_block_residue_hash(x, bs);
    mb_rd_record = &x->mb_rd_record;
    match_index = find_mb_rd_info(mb_rd_record, ref_best_rd, hash);
    if (match_index != -1) {
      MB_RD_INFO *tx_rd_info = &mb_rd_record->tx_rd_info[match_index];
      fetch_tx_rd_info(n4, tx_rd_info, rd_stats, x);
      return;
    }
  }

  // If we predict that skip is the optimal RD decision - set the respective
  // context and terminate early.
  int64_t dist;

  if (x->predict_skip_level && is_inter &&
      (!xd->lossless[xd->mi[0]->segment_id]) &&
      predict_skip_flag(x, bs, &dist, cpi->common.reduced_tx_set_used)) {
    // Populate rdstats as per skip decision
    set_skip_flag(x, rd_stats, bs, dist);
    // Save the RD search results into tx_rd_record.
    if (is_mb_rd_hash_enabled)
      save_tx_rd_info(n4, hash, x, rd_stats, mb_rd_record);
    return;
  }

  if (xd->lossless[xd->mi[0]->segment_id]) {
    choose_smallest_tx_size(cpi, x, rd_stats, ref_best_rd, bs);
  } else if (x->tx_size_search_method == USE_LARGESTALL) {
    choose_largest_tx_size(cpi, x, rd_stats, ref_best_rd, bs);
  } else {
    choose_tx_size_type_from_rd(cpi, x, rd_stats, ref_best_rd, bs);
  }

  // Save the RD search results into tx_rd_record.
  if (is_mb_rd_hash_enabled) {
    assert(mb_rd_record != NULL);
    save_tx_rd_info(n4, hash, x, rd_stats, mb_rd_record);
  }
}

// Return the rate cost for luma prediction mode info. of intra blocks.
static int intra_mode_info_cost_y(const AV1_COMP *cpi, const MACROBLOCK *x,
                                  const MB_MODE_INFO *mbmi, BLOCK_SIZE bsize,
                                  int mode_cost) {
  int total_rate = mode_cost;
  const int use_palette = mbmi->palette_mode_info.palette_size[0] > 0;
  const int use_filter_intra = mbmi->filter_intra_mode_info.use_filter_intra;
  const int use_intrabc = mbmi->use_intrabc;
  // Can only activate one mode.
  assert(((mbmi->mode != DC_PRED) + use_palette + use_intrabc +
          use_filter_intra) <= 1);
  const int try_palette =
      av1_allow_palette(cpi->common.allow_screen_content_tools, mbmi->sb_type);
  if (try_palette && mbmi->mode == DC_PRED) {
    const MACROBLOCKD *xd = &x->e_mbd;
    const int bsize_ctx = av1_get_palette_bsize_ctx(bsize);
    const int mode_ctx = av1_get_palette_mode_ctx(xd);
    total_rate += x->palette_y_mode_cost[bsize_ctx][mode_ctx][use_palette];
    if (use_palette) {
      const uint8_t *const color_map = xd->plane[0].color_index_map;
      int block_width, block_height, rows, cols;
      av1_get_block_dimensions(bsize, 0, xd, &block_width, &block_height, &rows,
                               &cols);
      const int plt_size = mbmi->palette_mode_info.palette_size[0];
      int palette_mode_cost =
          x->palette_y_size_cost[bsize_ctx][plt_size - PALETTE_MIN_SIZE] +
          write_uniform_cost(plt_size, color_map[0]);
      uint16_t color_cache[2 * PALETTE_MAX_SIZE];
      const int n_cache = av1_get_palette_cache(xd, 0, color_cache);
      palette_mode_cost +=
          av1_palette_color_cost_y(&mbmi->palette_mode_info, color_cache,
                                   n_cache, cpi->common.seq_params.bit_depth);
      palette_mode_cost +=
          av1_cost_color_map(x, 0, bsize, mbmi->tx_size, PALETTE_MAP);
      total_rate += palette_mode_cost;
    }
  }
  if (av1_filter_intra_allowed(&cpi->common, mbmi)) {
    total_rate += x->filter_intra_cost[mbmi->sb_type][use_filter_intra];
    if (use_filter_intra) {
      total_rate += x->filter_intra_mode_cost[mbmi->filter_intra_mode_info
                                                  .filter_intra_mode];
    }
  }
  if (av1_is_directional_mode(mbmi->mode)) {
    if (av1_use_angle_delta(bsize)) {
      total_rate += x->angle_delta_cost[mbmi->mode - V_PRED]
                                       [MAX_ANGLE_DELTA +
                                        mbmi->angle_delta[PLANE_TYPE_Y]];
    }
  }
  if (av1_allow_intrabc(&cpi->common))
    total_rate += x->intrabc_cost[use_intrabc];
  return total_rate;
}

// Return the rate cost for chroma prediction mode info. of intra blocks.
static int intra_mode_info_cost_uv(const AV1_COMP *cpi, const MACROBLOCK *x,
                                   const MB_MODE_INFO *mbmi, BLOCK_SIZE bsize,
                                   int mode_cost) {
  int total_rate = mode_cost;
  const int use_palette = mbmi->palette_mode_info.palette_size[1] > 0;
  const UV_PREDICTION_MODE mode = mbmi->uv_mode;
  // Can only activate one mode.
  assert(((mode != UV_DC_PRED) + use_palette + mbmi->use_intrabc) <= 1);

  const int try_palette =
      av1_allow_palette(cpi->common.allow_screen_content_tools, mbmi->sb_type);
  if (try_palette && mode == UV_DC_PRED) {
    const PALETTE_MODE_INFO *pmi = &mbmi->palette_mode_info;
    total_rate +=
        x->palette_uv_mode_cost[pmi->palette_size[0] > 0][use_palette];
    if (use_palette) {
      const int bsize_ctx = av1_get_palette_bsize_ctx(bsize);
      const int plt_size = pmi->palette_size[1];
      const MACROBLOCKD *xd = &x->e_mbd;
      const uint8_t *const color_map = xd->plane[1].color_index_map;
      int palette_mode_cost =
          x->palette_uv_size_cost[bsize_ctx][plt_size - PALETTE_MIN_SIZE] +
          write_uniform_cost(plt_size, color_map[0]);
      uint16_t color_cache[2 * PALETTE_MAX_SIZE];
      const int n_cache = av1_get_palette_cache(xd, 1, color_cache);
      palette_mode_cost += av1_palette_color_cost_uv(
          pmi, color_cache, n_cache, cpi->common.seq_params.bit_depth);
      palette_mode_cost +=
          av1_cost_color_map(x, 1, bsize, mbmi->tx_size, PALETTE_MAP);
      total_rate += palette_mode_cost;
    }
  }
  if (av1_is_directional_mode(get_uv_mode(mode))) {
    if (av1_use_angle_delta(bsize)) {
      total_rate +=
          x->angle_delta_cost[mode - V_PRED][mbmi->angle_delta[PLANE_TYPE_UV] +
                                             MAX_ANGLE_DELTA];
    }
  }
  return total_rate;
}

static int conditional_skipintra(PREDICTION_MODE mode,
                                 PREDICTION_MODE best_intra_mode) {
  if (mode == D113_PRED && best_intra_mode != V_PRED &&
      best_intra_mode != D135_PRED)
    return 1;
  if (mode == D67_PRED && best_intra_mode != V_PRED &&
      best_intra_mode != D45_PRED)
    return 1;
  if (mode == D203_PRED && best_intra_mode != H_PRED &&
      best_intra_mode != D45_PRED)
    return 1;
  if (mode == D157_PRED && best_intra_mode != H_PRED &&
      best_intra_mode != D135_PRED)
    return 1;
  return 0;
}

// Model based RD estimation for luma intra blocks.
static int64_t intra_model_yrd(const AV1_COMP *const cpi, MACROBLOCK *const x,
                               BLOCK_SIZE bsize, int mode_cost) {
  const AV1_COMMON *cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  assert(!is_inter_block(mbmi));
  RD_STATS this_rd_stats;
  int row, col;
  int64_t temp_sse, this_rd;
  TX_SIZE tx_size = tx_size_from_tx_mode(bsize, x->tx_mode_search_type);
  const int stepr = tx_size_high_unit[tx_size];
  const int stepc = tx_size_wide_unit[tx_size];
  const int max_blocks_wide = max_block_wide(xd, bsize, 0);
  const int max_blocks_high = max_block_high(xd, bsize, 0);
  mbmi->tx_size = tx_size;
  // Prediction.
  for (row = 0; row < max_blocks_high; row += stepr) {
    for (col = 0; col < max_blocks_wide; col += stepc) {
      av1_predict_intra_block_facade(cm, xd, 0, col, row, tx_size);
    }
  }
  // RD estimation.
  model_rd_sb_fn[cpi->sf.rt_sf.use_simple_rd_model ? MODELRD_LEGACY
                                                   : MODELRD_TYPE_INTRA](
      cpi, bsize, x, xd, 0, 0, &this_rd_stats.rate, &this_rd_stats.dist,
      &this_rd_stats.skip, &temp_sse, NULL, NULL, NULL);
  if (av1_is_directional_mode(mbmi->mode) && av1_use_angle_delta(bsize)) {
    mode_cost +=
        x->angle_delta_cost[mbmi->mode - V_PRED]
                           [MAX_ANGLE_DELTA + mbmi->angle_delta[PLANE_TYPE_Y]];
  }
  if (mbmi->mode == DC_PRED &&
      av1_filter_intra_allowed_bsize(cm, mbmi->sb_type)) {
    if (mbmi->filter_intra_mode_info.use_filter_intra) {
      const int mode = mbmi->filter_intra_mode_info.filter_intra_mode;
      mode_cost += x->filter_intra_cost[mbmi->sb_type][1] +
                   x->filter_intra_mode_cost[mode];
    } else {
      mode_cost += x->filter_intra_cost[mbmi->sb_type][0];
    }
  }
  this_rd =
      RDCOST(x->rdmult, this_rd_stats.rate + mode_cost, this_rd_stats.dist);
  return this_rd;
}

// Update the intra model yrd and prune the current mode if the new estimate
// y_rd > 1.5 * best_model_rd.
static AOM_INLINE int model_intra_yrd_and_prune(const AV1_COMP *const cpi,
                                                MACROBLOCK *x, BLOCK_SIZE bsize,
                                                int mode_info_cost,
                                                int64_t *best_model_rd) {
  const int64_t this_model_rd = intra_model_yrd(cpi, x, bsize, mode_info_cost);
  if (*best_model_rd != INT64_MAX &&
      this_model_rd > *best_model_rd + (*best_model_rd >> 1)) {
    return 1;
  } else if (this_model_rd < *best_model_rd) {
    *best_model_rd = this_model_rd;
  }
  return 0;
}

// Extends 'color_map' array from 'orig_width x orig_height' to 'new_width x
// new_height'. Extra rows and columns are filled in by copying last valid
// row/column.
static AOM_INLINE void extend_palette_color_map(uint8_t *const color_map,
                                                int orig_width, int orig_height,
                                                int new_width, int new_height) {
  int j;
  assert(new_width >= orig_width);
  assert(new_height >= orig_height);
  if (new_width == orig_width && new_height == orig_height) return;

  for (j = orig_height - 1; j >= 0; --j) {
    memmove(color_map + j * new_width, color_map + j * orig_width, orig_width);
    // Copy last column to extra columns.
    memset(color_map + j * new_width + orig_width,
           color_map[j * new_width + orig_width - 1], new_width - orig_width);
  }
  // Copy last row to extra rows.
  for (j = orig_height; j < new_height; ++j) {
    memcpy(color_map + j * new_width, color_map + (orig_height - 1) * new_width,
           new_width);
  }
}

// Bias toward using colors in the cache.
// TODO(huisu): Try other schemes to improve compression.
static AOM_INLINE void optimize_palette_colors(uint16_t *color_cache,
                                               int n_cache, int n_colors,
                                               int stride, int *centroids) {
  if (n_cache <= 0) return;
  for (int i = 0; i < n_colors * stride; i += stride) {
    int min_diff = abs(centroids[i] - (int)color_cache[0]);
    int idx = 0;
    for (int j = 1; j < n_cache; ++j) {
      const int this_diff = abs(centroids[i] - color_cache[j]);
      if (this_diff < min_diff) {
        min_diff = this_diff;
        idx = j;
      }
    }
    if (min_diff <= 1) centroids[i] = color_cache[idx];
  }
}

// Store best mode stats for winner mode processing
static void store_winner_mode_stats(const AV1_COMMON *const cm, MACROBLOCK *x,
                                    MB_MODE_INFO *mbmi, RD_STATS *rd_cost,
                                    RD_STATS *rd_cost_y, RD_STATS *rd_cost_uv,
                                    THR_MODES mode_index, uint8_t *color_map,
                                    BLOCK_SIZE bsize, int64_t this_rd,
                                    int enable_multiwinner_mode_process,
                                    int txfm_search_done) {
  WinnerModeStats *winner_mode_stats = x->winner_mode_stats;
  int mode_idx = 0;
  int is_palette_mode = mbmi->palette_mode_info.palette_size[PLANE_TYPE_Y] > 0;
  // Mode stat is not required when multiwinner mode processing is disabled
  if (!enable_multiwinner_mode_process) return;
  // Ignore mode with maximum rd
  if (this_rd == INT64_MAX) return;
  // TODO(any): Winner mode processing is currently not applicable for palette
  // mode in Inter frames. Clean-up the following code, once support is added
  if (!frame_is_intra_only(cm) && is_palette_mode) return;

  const int max_winner_mode_count = frame_is_intra_only(cm)
                                        ? MAX_WINNER_MODE_COUNT_INTRA
                                        : MAX_WINNER_MODE_COUNT_INTER;
  assert(x->winner_mode_count >= 0 &&
         x->winner_mode_count <= max_winner_mode_count);

  if (x->winner_mode_count) {
    // Find the mode which has higher rd cost than this_rd
    for (mode_idx = 0; mode_idx < x->winner_mode_count; mode_idx++)
      if (winner_mode_stats[mode_idx].rd > this_rd) break;

    if (mode_idx == max_winner_mode_count) {
      // No mode has higher rd cost than this_rd
      return;
    } else if (mode_idx < max_winner_mode_count - 1) {
      // Create a slot for current mode and move others to the next slot
      memmove(
          &winner_mode_stats[mode_idx + 1], &winner_mode_stats[mode_idx],
          (max_winner_mode_count - mode_idx - 1) * sizeof(*winner_mode_stats));
    }
  }
  // Add a mode stat for winner mode processing
  winner_mode_stats[mode_idx].mbmi = *mbmi;
  winner_mode_stats[mode_idx].rd = this_rd;
  winner_mode_stats[mode_idx].mode_index = mode_index;

  // Update rd stats required for inter frame
  if (!frame_is_intra_only(cm) && rd_cost && rd_cost_y && rd_cost_uv) {
    const MACROBLOCKD *xd = &x->e_mbd;
    const int skip_ctx = av1_get_skip_context(xd);
    const int is_intra_mode = av1_mode_defs[mode_index].mode < INTRA_MODE_END;
    const int skip = mbmi->skip && !is_intra_mode;

    winner_mode_stats[mode_idx].rd_cost = *rd_cost;
    if (txfm_search_done) {
      winner_mode_stats[mode_idx].rate_y =
          rd_cost_y->rate + x->skip_cost[skip_ctx][rd_cost->skip || skip];
      winner_mode_stats[mode_idx].rate_uv = rd_cost_uv->rate;
    }
  }

  if (color_map) {
    // Store color_index_map for palette mode
    const MACROBLOCKD *const xd = &x->e_mbd;
    int block_width, block_height;
    av1_get_block_dimensions(bsize, AOM_PLANE_Y, xd, &block_width,
                             &block_height, NULL, NULL);
    memcpy(winner_mode_stats[mode_idx].color_index_map, color_map,
           block_width * block_height * sizeof(color_map[0]));
  }

  x->winner_mode_count =
      AOMMIN(x->winner_mode_count + 1, max_winner_mode_count);
}

// Given the base colors as specified in centroids[], calculate the RD cost
// of palette mode.
static AOM_INLINE void palette_rd_y(
    const AV1_COMP *const cpi, MACROBLOCK *x, MB_MODE_INFO *mbmi,
    BLOCK_SIZE bsize, int dc_mode_cost, const int *data, int *centroids, int n,
    uint16_t *color_cache, int n_cache, MB_MODE_INFO *best_mbmi,
    uint8_t *best_palette_color_map, int64_t *best_rd, int64_t *best_model_rd,
    int *rate, int *rate_tokenonly, int64_t *distortion, int *skippable,
    int *beat_best_rd, PICK_MODE_CONTEXT *ctx, uint8_t *blk_skip,
    uint8_t *tx_type_map, int *beat_best_pallette_rd) {
  optimize_palette_colors(color_cache, n_cache, n, 1, centroids);
  int k = av1_remove_duplicates(centroids, n);
  if (k < PALETTE_MIN_SIZE) {
    // Too few unique colors to create a palette. And DC_PRED will work
    // well for that case anyway. So skip.
    return;
  }
  PALETTE_MODE_INFO *const pmi = &mbmi->palette_mode_info;
  if (cpi->common.seq_params.use_highbitdepth)
    for (int i = 0; i < k; ++i)
      pmi->palette_colors[i] = clip_pixel_highbd(
          (int)centroids[i], cpi->common.seq_params.bit_depth);
  else
    for (int i = 0; i < k; ++i)
      pmi->palette_colors[i] = clip_pixel(centroids[i]);
  pmi->palette_size[0] = k;
  MACROBLOCKD *const xd = &x->e_mbd;
  uint8_t *const color_map = xd->plane[0].color_index_map;
  int block_width, block_height, rows, cols;
  av1_get_block_dimensions(bsize, 0, xd, &block_width, &block_height, &rows,
                           &cols);
  av1_calc_indices(data, centroids, color_map, rows * cols, k, 1);
  extend_palette_color_map(color_map, cols, rows, block_width, block_height);

  const int palette_mode_cost =
      intra_mode_info_cost_y(cpi, x, mbmi, bsize, dc_mode_cost);
  if (model_intra_yrd_and_prune(cpi, x, bsize, palette_mode_cost,
                                best_model_rd)) {
    return;
  }

  RD_STATS tokenonly_rd_stats;
  super_block_yrd(cpi, x, &tokenonly_rd_stats, bsize, *best_rd);
  if (tokenonly_rd_stats.rate == INT_MAX) return;
  int this_rate = tokenonly_rd_stats.rate + palette_mode_cost;
  int64_t this_rd = RDCOST(x->rdmult, this_rate, tokenonly_rd_stats.dist);
  if (!xd->lossless[mbmi->segment_id] && block_signals_txsize(mbmi->sb_type)) {
    tokenonly_rd_stats.rate -= tx_size_cost(x, bsize, mbmi->tx_size);
  }
  // Collect mode stats for multiwinner mode processing
  const int txfm_search_done = 1;
  store_winner_mode_stats(
      &cpi->common, x, mbmi, NULL, NULL, NULL, THR_DC, color_map, bsize,
      this_rd, cpi->sf.winner_mode_sf.enable_multiwinner_mode_process,
      txfm_search_done);
  if (this_rd < *best_rd) {
    *best_rd = this_rd;
    // Setting beat_best_rd flag because current mode rd is better than best_rd.
    // This flag need to be updated only for palette evaluation in key frames
    if (beat_best_rd) *beat_best_rd = 1;
    memcpy(best_palette_color_map, color_map,
           block_width * block_height * sizeof(color_map[0]));
    *best_mbmi = *mbmi;
    memcpy(blk_skip, x->blk_skip, sizeof(x->blk_skip[0]) * ctx->num_4x4_blk);
    av1_copy_array(tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);
    if (rate) *rate = this_rate;
    if (rate_tokenonly) *rate_tokenonly = tokenonly_rd_stats.rate;
    if (distortion) *distortion = tokenonly_rd_stats.dist;
    if (skippable) *skippable = tokenonly_rd_stats.skip;
    if (beat_best_pallette_rd) *beat_best_pallette_rd = 1;
  }
}

static AOM_INLINE int perform_top_color_coarse_palette_search(
    const AV1_COMP *const cpi, MACROBLOCK *x, MB_MODE_INFO *mbmi,
    BLOCK_SIZE bsize, int dc_mode_cost, const int *data,
    const int *const top_colors, int start_n, int end_n, int step_size,
    uint16_t *color_cache, int n_cache, MB_MODE_INFO *best_mbmi,
    uint8_t *best_palette_color_map, int64_t *best_rd, int64_t *best_model_rd,
    int *rate, int *rate_tokenonly, int64_t *distortion, int *skippable,
    int *beat_best_rd, PICK_MODE_CONTEXT *ctx, uint8_t *best_blk_skip,
    uint8_t *tx_type_map) {
  int centroids[PALETTE_MAX_SIZE];
  int n = start_n;
  int top_color_winner = end_n + 1;
  while (1) {
    int beat_best_pallette_rd = 0;
    for (int i = 0; i < n; ++i) centroids[i] = top_colors[i];
    palette_rd_y(cpi, x, mbmi, bsize, dc_mode_cost, data, centroids, n,
                 color_cache, n_cache, best_mbmi, best_palette_color_map,
                 best_rd, best_model_rd, rate, rate_tokenonly, distortion,
                 skippable, beat_best_rd, ctx, best_blk_skip, tx_type_map,
                 &beat_best_pallette_rd);
    // Break if current palette colors is not winning
    if (beat_best_pallette_rd) top_color_winner = n;
    n += step_size;
    if (n > end_n) break;
  }
  return top_color_winner;
}

static AOM_INLINE int perform_k_means_coarse_palette_search(
    const AV1_COMP *const cpi, MACROBLOCK *x, MB_MODE_INFO *mbmi,
    BLOCK_SIZE bsize, int dc_mode_cost, const int *data, int lb, int ub,
    int start_n, int end_n, int step_size, uint16_t *color_cache, int n_cache,
    MB_MODE_INFO *best_mbmi, uint8_t *best_palette_color_map, int64_t *best_rd,
    int64_t *best_model_rd, int *rate, int *rate_tokenonly, int64_t *distortion,
    int *skippable, int *beat_best_rd, PICK_MODE_CONTEXT *ctx,
    uint8_t *best_blk_skip, uint8_t *tx_type_map, uint8_t *color_map,
    int data_points) {
  int centroids[PALETTE_MAX_SIZE];
  const int max_itr = 50;
  int n = start_n;
  int k_means_winner = end_n + 1;
  while (1) {
    int beat_best_pallette_rd = 0;
    for (int i = 0; i < n; ++i) {
      centroids[i] = lb + (2 * i + 1) * (ub - lb) / n / 2;
    }
    av1_k_means(data, centroids, color_map, data_points, n, 1, max_itr);
    palette_rd_y(cpi, x, mbmi, bsize, dc_mode_cost, data, centroids, n,
                 color_cache, n_cache, best_mbmi, best_palette_color_map,
                 best_rd, best_model_rd, rate, rate_tokenonly, distortion,
                 skippable, beat_best_rd, ctx, best_blk_skip, tx_type_map,
                 &beat_best_pallette_rd);
    // Break if current palette colors is not winning
    if (beat_best_pallette_rd) k_means_winner = n;
    n += step_size;
    if (n > end_n) break;
  }
  return k_means_winner;
}

// Perform palette search for top colors from minimum palette colors (/maximum)
// with a step-size of 1 (/-1)
static AOM_INLINE int perform_top_color_palette_search(
    const AV1_COMP *const cpi, MACROBLOCK *x, MB_MODE_INFO *mbmi,
    BLOCK_SIZE bsize, int dc_mode_cost, const int *data, int *top_colors,
    int start_n, int end_n, int step_size, uint16_t *color_cache, int n_cache,
    MB_MODE_INFO *best_mbmi, uint8_t *best_palette_color_map, int64_t *best_rd,
    int64_t *best_model_rd, int *rate, int *rate_tokenonly, int64_t *distortion,
    int *skippable, int *beat_best_rd, PICK_MODE_CONTEXT *ctx,
    uint8_t *best_blk_skip, uint8_t *tx_type_map) {
  int centroids[PALETTE_MAX_SIZE];
  int n = start_n;
  assert((step_size == -1) || (step_size == 1) || (step_size == 0) ||
         (step_size == 2));
  assert(IMPLIES(step_size == -1, start_n > end_n));
  assert(IMPLIES(step_size == 1, start_n < end_n));
  while (1) {
    int beat_best_pallette_rd = 0;
    for (int i = 0; i < n; ++i) centroids[i] = top_colors[i];
    palette_rd_y(cpi, x, mbmi, bsize, dc_mode_cost, data, centroids, n,
                 color_cache, n_cache, best_mbmi, best_palette_color_map,
                 best_rd, best_model_rd, rate, rate_tokenonly, distortion,
                 skippable, beat_best_rd, ctx, best_blk_skip, tx_type_map,
                 &beat_best_pallette_rd);
    // Break if current palette colors is not winning
    if ((cpi->sf.intra_sf.prune_palette_search_level == 2) &&
        !beat_best_pallette_rd)
      return n;
    n += step_size;
    if (n == end_n) break;
  }
  return n;
}
// Perform k-means based palette search from minimum palette colors (/maximum)
// with a step-size of 1 (/-1)
static AOM_INLINE int perform_k_means_palette_search(
    const AV1_COMP *const cpi, MACROBLOCK *x, MB_MODE_INFO *mbmi,
    BLOCK_SIZE bsize, int dc_mode_cost, const int *data, int lb, int ub,
    int start_n, int end_n, int step_size, uint16_t *color_cache, int n_cache,
    MB_MODE_INFO *best_mbmi, uint8_t *best_palette_color_map, int64_t *best_rd,
    int64_t *best_model_rd, int *rate, int *rate_tokenonly, int64_t *distortion,
    int *skippable, int *beat_best_rd, PICK_MODE_CONTEXT *ctx,
    uint8_t *best_blk_skip, uint8_t *tx_type_map, uint8_t *color_map,
    int data_points) {
  int centroids[PALETTE_MAX_SIZE];
  const int max_itr = 50;
  int n = start_n;
  assert((step_size == -1) || (step_size == 1) || (step_size == 0) ||
         (step_size == 2));
  assert(IMPLIES(step_size == -1, start_n > end_n));
  assert(IMPLIES(step_size == 1, start_n < end_n));
  while (1) {
    int beat_best_pallette_rd = 0;
    for (int i = 0; i < n; ++i) {
      centroids[i] = lb + (2 * i + 1) * (ub - lb) / n / 2;
    }
    av1_k_means(data, centroids, color_map, data_points, n, 1, max_itr);
    palette_rd_y(cpi, x, mbmi, bsize, dc_mode_cost, data, centroids, n,
                 color_cache, n_cache, best_mbmi, best_palette_color_map,
                 best_rd, best_model_rd, rate, rate_tokenonly, distortion,
                 skippable, beat_best_rd, ctx, best_blk_skip, tx_type_map,
                 &beat_best_pallette_rd);
    // Break if current palette colors is not winning
    if ((cpi->sf.intra_sf.prune_palette_search_level == 2) &&
        !beat_best_pallette_rd)
      return n;
    n += step_size;
    if (n == end_n) break;
  }
  return n;
}

#define START_N_STAGE2(x)                         \
  ((x == PALETTE_MIN_SIZE) ? PALETTE_MIN_SIZE + 1 \
                           : AOMMAX(x - 1, PALETTE_MIN_SIZE));
#define END_N_STAGE2(x, end_n) \
  ((x == end_n) ? x - 1 : AOMMIN(x + 1, PALETTE_MAX_SIZE));

static AOM_INLINE void update_start_end_stage_2(int *start_n_stage2,
                                                int *end_n_stage2,
                                                int *step_size_stage2,
                                                int winner, int end_n) {
  *start_n_stage2 = START_N_STAGE2(winner);
  *end_n_stage2 = END_N_STAGE2(winner, end_n);
  *step_size_stage2 = *end_n_stage2 - *start_n_stage2;
}

// Start index and step size below are chosen to evaluate unique
// candidates in neighbor search, in case a winner candidate is found in
// coarse search. Example,
// 1) 8 colors (end_n = 8): 2,3,4,5,6,7,8. start_n is chosen as 2 and step
// size is chosen as 3. Therefore, coarse search will evaluate 2, 5 and 8.
// If winner is found at 5, then 4 and 6 are evaluated. Similarly, for 2
// (3) and 8 (7).
// 2) 7 colors (end_n = 7): 2,3,4,5,6,7. If start_n is chosen as 2 (same
// as for 8 colors) then step size should also be 2, to cover all
// candidates. Coarse search will evaluate 2, 4 and 6. If winner is either
// 2 or 4, 3 will be evaluated. Instead, if start_n=3 and step_size=3,
// coarse search will evaluate 3 and 6. For the winner, unique neighbors
// (3: 2,4 or 6: 5,7) would be evaluated.

// start index for coarse palette search for dominant colors and k-means
static const uint8_t start_n_lookup_table[PALETTE_MAX_SIZE + 1] = { 0, 0, 0,
                                                                    3, 3, 2,
                                                                    3, 3, 2 };
// step size for coarse palette search for dominant colors and k-means
static const uint8_t step_size_lookup_table[PALETTE_MAX_SIZE + 1] = { 0, 0, 0,
                                                                      3, 3, 3,
                                                                      3, 3, 3 };

static void rd_pick_palette_intra_sby(
    const AV1_COMP *const cpi, MACROBLOCK *x, BLOCK_SIZE bsize,
    int dc_mode_cost, MB_MODE_INFO *best_mbmi, uint8_t *best_palette_color_map,
    int64_t *best_rd, int64_t *best_model_rd, int *rate, int *rate_tokenonly,
    int64_t *distortion, int *skippable, int *beat_best_rd,
    PICK_MODE_CONTEXT *ctx, uint8_t *best_blk_skip, uint8_t *tx_type_map) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  assert(!is_inter_block(mbmi));
  assert(av1_allow_palette(cpi->common.allow_screen_content_tools, bsize));

  const int src_stride = x->plane[0].src.stride;
  const uint8_t *const src = x->plane[0].src.buf;
  int block_width, block_height, rows, cols;
  av1_get_block_dimensions(bsize, 0, xd, &block_width, &block_height, &rows,
                           &cols);
  const SequenceHeader *const seq_params = &cpi->common.seq_params;
  const int is_hbd = seq_params->use_highbitdepth;
  const int bit_depth = seq_params->bit_depth;
  int count_buf[1 << 12];  // Maximum (1 << 12) color levels.
  int colors;
  if (is_hbd) {
    colors = av1_count_colors_highbd(src, src_stride, rows, cols, bit_depth,
                                     count_buf);
  } else {
    colors = av1_count_colors(src, src_stride, rows, cols, count_buf);
  }

  uint8_t *const color_map = xd->plane[0].color_index_map;
  if (colors > 1 && colors <= 64) {
    int *const data = x->palette_buffer->kmeans_data_buf;
    int centroids[PALETTE_MAX_SIZE];
    int lb, ub;
    if (is_hbd) {
      int *data_pt = data;
      const uint16_t *src_pt = CONVERT_TO_SHORTPTR(src);
      lb = ub = src_pt[0];
      for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
          const int val = src_pt[c];
          data_pt[c] = val;
          lb = AOMMIN(lb, val);
          ub = AOMMAX(ub, val);
        }
        src_pt += src_stride;
        data_pt += cols;
      }
    } else {
      int *data_pt = data;
      const uint8_t *src_pt = src;
      lb = ub = src[0];
      for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
          const int val = src_pt[c];
          data_pt[c] = val;
          lb = AOMMIN(lb, val);
          ub = AOMMAX(ub, val);
        }
        src_pt += src_stride;
        data_pt += cols;
      }
    }

    mbmi->mode = DC_PRED;
    mbmi->filter_intra_mode_info.use_filter_intra = 0;

    uint16_t color_cache[2 * PALETTE_MAX_SIZE];
    const int n_cache = av1_get_palette_cache(xd, 0, color_cache);

    // Find the dominant colors, stored in top_colors[].
    int top_colors[PALETTE_MAX_SIZE] = { 0 };
    for (int i = 0; i < AOMMIN(colors, PALETTE_MAX_SIZE); ++i) {
      int max_count = 0;
      for (int j = 0; j < (1 << bit_depth); ++j) {
        if (count_buf[j] > max_count) {
          max_count = count_buf[j];
          top_colors[i] = j;
        }
      }
      assert(max_count > 0);
      count_buf[top_colors[i]] = 0;
    }

    // Try the dominant colors directly.
    // TODO(huisu@google.com): Try to avoid duplicate computation in cases
    // where the dominant colors and the k-means results are similar.
    if ((cpi->sf.intra_sf.prune_palette_search_level == 1) &&
        (colors > PALETTE_MIN_SIZE)) {
      const int end_n = AOMMIN(colors, PALETTE_MAX_SIZE);
      assert(PALETTE_MAX_SIZE == 8);
      assert(PALETTE_MIN_SIZE == 2);
      // Choose the start index and step size for coarse search based on number
      // of colors
      const int start_n = start_n_lookup_table[end_n];
      const int step_size = step_size_lookup_table[end_n];
      // Perform top color coarse palette search to find the winner candidate
      const int top_color_winner = perform_top_color_coarse_palette_search(
          cpi, x, mbmi, bsize, dc_mode_cost, data, top_colors, start_n, end_n,
          step_size, color_cache, n_cache, best_mbmi, best_palette_color_map,
          best_rd, best_model_rd, rate, rate_tokenonly, distortion, skippable,
          beat_best_rd, ctx, best_blk_skip, tx_type_map);
      // Evaluate neighbors for the winner color (if winner is found) in the
      // above coarse search for dominant colors
      if (top_color_winner <= end_n) {
        int start_n_stage2, end_n_stage2, step_size_stage2;
        update_start_end_stage_2(&start_n_stage2, &end_n_stage2,
                                 &step_size_stage2, top_color_winner, end_n);
        // perform finer search for the winner candidate
        perform_top_color_palette_search(
            cpi, x, mbmi, bsize, dc_mode_cost, data, top_colors, start_n_stage2,
            end_n_stage2 + step_size_stage2, step_size_stage2, color_cache,
            n_cache, best_mbmi, best_palette_color_map, best_rd, best_model_rd,
            rate, rate_tokenonly, distortion, skippable, beat_best_rd, ctx,
            best_blk_skip, tx_type_map);
      }
      // K-means clustering.
      // Perform k-means coarse palette search to find the winner candidate
      const int k_means_winner = perform_k_means_coarse_palette_search(
          cpi, x, mbmi, bsize, dc_mode_cost, data, lb, ub, start_n, end_n,
          step_size, color_cache, n_cache, best_mbmi, best_palette_color_map,
          best_rd, best_model_rd, rate, rate_tokenonly, distortion, skippable,
          beat_best_rd, ctx, best_blk_skip, tx_type_map, color_map,
          rows * cols);
      // Evaluate neighbors for the winner color (if winner is found) in the
      // above coarse search for k-means
      if (k_means_winner <= end_n) {
        int start_n_stage2, end_n_stage2, step_size_stage2;
        update_start_end_stage_2(&start_n_stage2, &end_n_stage2,
                                 &step_size_stage2, k_means_winner, end_n);
        // perform finer search for the winner candidate
        perform_k_means_palette_search(
            cpi, x, mbmi, bsize, dc_mode_cost, data, lb, ub, start_n_stage2,
            end_n_stage2 + step_size_stage2, step_size_stage2, color_cache,
            n_cache, best_mbmi, best_palette_color_map, best_rd, best_model_rd,
            rate, rate_tokenonly, distortion, skippable, beat_best_rd, ctx,
            best_blk_skip, tx_type_map, color_map, rows * cols);
      }
    } else {
      const int start_n = AOMMIN(colors, PALETTE_MAX_SIZE),
                end_n = PALETTE_MIN_SIZE;
      // Perform top color palette search from start_n
      const int top_color_winner = perform_top_color_palette_search(
          cpi, x, mbmi, bsize, dc_mode_cost, data, top_colors, start_n,
          end_n - 1, -1, color_cache, n_cache, best_mbmi,
          best_palette_color_map, best_rd, best_model_rd, rate, rate_tokenonly,
          distortion, skippable, beat_best_rd, ctx, best_blk_skip, tx_type_map);

      if (top_color_winner > end_n) {
        // Perform top color palette search in reverse order for the remaining
        // colors
        perform_top_color_palette_search(
            cpi, x, mbmi, bsize, dc_mode_cost, data, top_colors, end_n,
            top_color_winner, 1, color_cache, n_cache, best_mbmi,
            best_palette_color_map, best_rd, best_model_rd, rate,
            rate_tokenonly, distortion, skippable, beat_best_rd, ctx,
            best_blk_skip, tx_type_map);
      }
      // K-means clustering.
      if (colors == PALETTE_MIN_SIZE) {
        // Special case: These colors automatically become the centroids.
        assert(colors == 2);
        centroids[0] = lb;
        centroids[1] = ub;
        palette_rd_y(cpi, x, mbmi, bsize, dc_mode_cost, data, centroids, colors,
                     color_cache, n_cache, best_mbmi, best_palette_color_map,
                     best_rd, best_model_rd, rate, rate_tokenonly, distortion,
                     skippable, beat_best_rd, ctx, best_blk_skip, tx_type_map,
                     NULL);
      } else {
        // Perform k-means palette search from start_n
        const int k_means_winner = perform_k_means_palette_search(
            cpi, x, mbmi, bsize, dc_mode_cost, data, lb, ub, start_n, end_n - 1,
            -1, color_cache, n_cache, best_mbmi, best_palette_color_map,
            best_rd, best_model_rd, rate, rate_tokenonly, distortion, skippable,
            beat_best_rd, ctx, best_blk_skip, tx_type_map, color_map,
            rows * cols);
        if (k_means_winner > end_n) {
          // Perform k-means palette search in reverse order for the remaining
          // colors
          perform_k_means_palette_search(
              cpi, x, mbmi, bsize, dc_mode_cost, data, lb, ub, end_n,
              k_means_winner, 1, color_cache, n_cache, best_mbmi,
              best_palette_color_map, best_rd, best_model_rd, rate,
              rate_tokenonly, distortion, skippable, beat_best_rd, ctx,
              best_blk_skip, tx_type_map, color_map, rows * cols);
        }
      }
    }
  }

  if (best_mbmi->palette_mode_info.palette_size[0] > 0) {
    memcpy(color_map, best_palette_color_map,
           block_width * block_height * sizeof(best_palette_color_map[0]));
  }
  *mbmi = *best_mbmi;
}

// Return 1 if an filter intra mode is selected; return 0 otherwise.
static int rd_pick_filter_intra_sby(const AV1_COMP *const cpi, MACROBLOCK *x,
                                    int *rate, int *rate_tokenonly,
                                    int64_t *distortion, int *skippable,
                                    BLOCK_SIZE bsize, int mode_cost,
                                    int64_t *best_rd, int64_t *best_model_rd,
                                    PICK_MODE_CONTEXT *ctx) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  int filter_intra_selected_flag = 0;
  FILTER_INTRA_MODE mode;
  TX_SIZE best_tx_size = TX_8X8;
  FILTER_INTRA_MODE_INFO filter_intra_mode_info;
  uint8_t best_tx_type_map[MAX_MIB_SIZE * MAX_MIB_SIZE];
  (void)ctx;
  av1_zero(filter_intra_mode_info);
  mbmi->filter_intra_mode_info.use_filter_intra = 1;
  mbmi->mode = DC_PRED;
  mbmi->palette_mode_info.palette_size[0] = 0;

  for (mode = 0; mode < FILTER_INTRA_MODES; ++mode) {
    int64_t this_rd;
    RD_STATS tokenonly_rd_stats;
    mbmi->filter_intra_mode_info.filter_intra_mode = mode;

    if (model_intra_yrd_and_prune(cpi, x, bsize, mode_cost, best_model_rd)) {
      continue;
    }
    super_block_yrd(cpi, x, &tokenonly_rd_stats, bsize, *best_rd);
    if (tokenonly_rd_stats.rate == INT_MAX) continue;
    const int this_rate =
        tokenonly_rd_stats.rate +
        intra_mode_info_cost_y(cpi, x, mbmi, bsize, mode_cost);
    this_rd = RDCOST(x->rdmult, this_rate, tokenonly_rd_stats.dist);

    // Collect mode stats for multiwinner mode processing
    const int txfm_search_done = 1;
    store_winner_mode_stats(
        &cpi->common, x, mbmi, NULL, NULL, NULL, 0, NULL, bsize, this_rd,
        cpi->sf.winner_mode_sf.enable_multiwinner_mode_process,
        txfm_search_done);
    if (this_rd < *best_rd) {
      *best_rd = this_rd;
      best_tx_size = mbmi->tx_size;
      filter_intra_mode_info = mbmi->filter_intra_mode_info;
      av1_copy_array(best_tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);
      memcpy(ctx->blk_skip, x->blk_skip,
             sizeof(x->blk_skip[0]) * ctx->num_4x4_blk);
      *rate = this_rate;
      *rate_tokenonly = tokenonly_rd_stats.rate;
      *distortion = tokenonly_rd_stats.dist;
      *skippable = tokenonly_rd_stats.skip;
      filter_intra_selected_flag = 1;
    }
  }

  if (filter_intra_selected_flag) {
    mbmi->mode = DC_PRED;
    mbmi->tx_size = best_tx_size;
    mbmi->filter_intra_mode_info = filter_intra_mode_info;
    av1_copy_array(ctx->tx_type_map, best_tx_type_map, ctx->num_4x4_blk);
    return 1;
  } else {
    return 0;
  }
}

// Run RD calculation with given luma intra prediction angle., and return
// the RD cost. Update the best mode info. if the RD cost is the best so far.
static int64_t calc_rd_given_intra_angle(
    const AV1_COMP *const cpi, MACROBLOCK *x, BLOCK_SIZE bsize, int mode_cost,
    int64_t best_rd_in, int8_t angle_delta, int max_angle_delta, int *rate,
    RD_STATS *rd_stats, int *best_angle_delta, TX_SIZE *best_tx_size,
    int64_t *best_rd, int64_t *best_model_rd, uint8_t *best_tx_type_map,
    uint8_t *best_blk_skip, int skip_model_rd) {
  RD_STATS tokenonly_rd_stats;
  int64_t this_rd;
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  const int n4 = bsize_to_num_blk(bsize);
  assert(!is_inter_block(mbmi));
  mbmi->angle_delta[PLANE_TYPE_Y] = angle_delta;
  if (!skip_model_rd) {
    if (model_intra_yrd_and_prune(cpi, x, bsize, mode_cost, best_model_rd)) {
      return INT64_MAX;
    }
  }
  super_block_yrd(cpi, x, &tokenonly_rd_stats, bsize, best_rd_in);
  if (tokenonly_rd_stats.rate == INT_MAX) return INT64_MAX;

  int this_rate =
      mode_cost + tokenonly_rd_stats.rate +
      x->angle_delta_cost[mbmi->mode - V_PRED][max_angle_delta + angle_delta];
  this_rd = RDCOST(x->rdmult, this_rate, tokenonly_rd_stats.dist);

  if (this_rd < *best_rd) {
    memcpy(best_blk_skip, x->blk_skip, sizeof(best_blk_skip[0]) * n4);
    av1_copy_array(best_tx_type_map, xd->tx_type_map, n4);
    *best_rd = this_rd;
    *best_angle_delta = mbmi->angle_delta[PLANE_TYPE_Y];
    *best_tx_size = mbmi->tx_size;
    *rate = this_rate;
    rd_stats->rate = tokenonly_rd_stats.rate;
    rd_stats->dist = tokenonly_rd_stats.dist;
    rd_stats->skip = tokenonly_rd_stats.skip;
  }
  return this_rd;
}

// With given luma directional intra prediction mode, pick the best angle delta
// Return the RD cost corresponding to the best angle delta.
static int64_t rd_pick_intra_angle_sby(const AV1_COMP *const cpi, MACROBLOCK *x,
                                       int *rate, RD_STATS *rd_stats,
                                       BLOCK_SIZE bsize, int mode_cost,
                                       int64_t best_rd, int64_t *best_model_rd,
                                       int skip_model_rd_for_zero_deg) {
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  assert(!is_inter_block(mbmi));

  int best_angle_delta = 0;
  int64_t rd_cost[2 * (MAX_ANGLE_DELTA + 2)];
  TX_SIZE best_tx_size = mbmi->tx_size;
  uint8_t best_blk_skip[MAX_MIB_SIZE * MAX_MIB_SIZE];
  uint8_t best_tx_type_map[MAX_MIB_SIZE * MAX_MIB_SIZE];

  for (int i = 0; i < 2 * (MAX_ANGLE_DELTA + 2); ++i) rd_cost[i] = INT64_MAX;

  int first_try = 1;
  for (int angle_delta = 0; angle_delta <= MAX_ANGLE_DELTA; angle_delta += 2) {
    for (int i = 0; i < 2; ++i) {
      const int64_t best_rd_in =
          (best_rd == INT64_MAX) ? INT64_MAX
                                 : (best_rd + (best_rd >> (first_try ? 3 : 5)));
      const int64_t this_rd = calc_rd_given_intra_angle(
          cpi, x, bsize, mode_cost, best_rd_in, (1 - 2 * i) * angle_delta,
          MAX_ANGLE_DELTA, rate, rd_stats, &best_angle_delta, &best_tx_size,
          &best_rd, best_model_rd, best_tx_type_map, best_blk_skip,
          (skip_model_rd_for_zero_deg & !angle_delta));
      rd_cost[2 * angle_delta + i] = this_rd;
      if (first_try && this_rd == INT64_MAX) return best_rd;
      first_try = 0;
      if (angle_delta == 0) {
        rd_cost[1] = this_rd;
        break;
      }
    }
  }

  assert(best_rd != INT64_MAX);
  for (int angle_delta = 1; angle_delta <= MAX_ANGLE_DELTA; angle_delta += 2) {
    for (int i = 0; i < 2; ++i) {
      int skip_search = 0;
      const int64_t rd_thresh = best_rd + (best_rd >> 5);
      if (rd_cost[2 * (angle_delta + 1) + i] > rd_thresh &&
          rd_cost[2 * (angle_delta - 1) + i] > rd_thresh)
        skip_search = 1;
      if (!skip_search) {
        calc_rd_given_intra_angle(
            cpi, x, bsize, mode_cost, best_rd, (1 - 2 * i) * angle_delta,
            MAX_ANGLE_DELTA, rate, rd_stats, &best_angle_delta, &best_tx_size,
            &best_rd, best_model_rd, best_tx_type_map, best_blk_skip, 0);
      }
    }
  }

  if (rd_stats->rate != INT_MAX) {
    mbmi->tx_size = best_tx_size;
    mbmi->angle_delta[PLANE_TYPE_Y] = best_angle_delta;
    const int n4 = bsize_to_num_blk(bsize);
    memcpy(x->blk_skip, best_blk_skip, sizeof(best_blk_skip[0]) * n4);
    av1_copy_array(xd->tx_type_map, best_tx_type_map, n4);
  }
  return best_rd;
}

// Given selected prediction mode, search for the best tx type and size.
static AOM_INLINE int intra_block_yrd(const AV1_COMP *const cpi, MACROBLOCK *x,
                                      BLOCK_SIZE bsize, const int *bmode_costs,
                                      int64_t *best_rd, int *rate,
                                      int *rate_tokenonly, int64_t *distortion,
                                      int *skippable, MB_MODE_INFO *best_mbmi,
                                      PICK_MODE_CONTEXT *ctx) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  RD_STATS rd_stats;
  // In order to improve txfm search avoid rd based breakouts during winner
  // mode evaluation. Hence passing ref_best_rd as a maximum value
  super_block_yrd(cpi, x, &rd_stats, bsize, INT64_MAX);
  if (rd_stats.rate == INT_MAX) return 0;
  int this_rate_tokenonly = rd_stats.rate;
  if (!xd->lossless[mbmi->segment_id] && block_signals_txsize(mbmi->sb_type)) {
    // super_block_yrd above includes the cost of the tx_size in the
    // tokenonly rate, but for intra blocks, tx_size is always coded
    // (prediction granularity), so we account for it in the full rate,
    // not the tokenonly rate.
    this_rate_tokenonly -= tx_size_cost(x, bsize, mbmi->tx_size);
  }
  const int this_rate =
      rd_stats.rate +
      intra_mode_info_cost_y(cpi, x, mbmi, bsize, bmode_costs[mbmi->mode]);
  const int64_t this_rd = RDCOST(x->rdmult, this_rate, rd_stats.dist);
  if (this_rd < *best_rd) {
    *best_mbmi = *mbmi;
    *best_rd = this_rd;
    *rate = this_rate;
    *rate_tokenonly = this_rate_tokenonly;
    *distortion = rd_stats.dist;
    *skippable = rd_stats.skip;
    av1_copy_array(ctx->blk_skip, x->blk_skip, ctx->num_4x4_blk);
    av1_copy_array(ctx->tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);
    return 1;
  }
  return 0;
}

#define BINS 32
static const float intra_hog_model_bias[DIRECTIONAL_MODES] = {
  0.450578f,  0.695518f,  -0.717944f, -0.639894f,
  -0.602019f, -0.453454f, 0.055857f,  -0.465480f,
};

static const float intra_hog_model_weights[BINS * DIRECTIONAL_MODES] = {
  -3.076402f, -3.757063f, -3.275266f, -3.180665f, -3.452105f, -3.216593f,
  -2.871212f, -3.134296f, -1.822324f, -2.401411f, -1.541016f, -1.195322f,
  -0.434156f, 0.322868f,  2.260546f,  3.368715f,  3.989290f,  3.308487f,
  2.277893f,  0.923793f,  0.026412f,  -0.385174f, -0.718622f, -1.408867f,
  -1.050558f, -2.323941f, -2.225827f, -2.585453f, -3.054283f, -2.875087f,
  -2.985709f, -3.447155f, 3.758139f,  3.204353f,  2.170998f,  0.826587f,
  -0.269665f, -0.702068f, -1.085776f, -2.175249f, -1.623180f, -2.975142f,
  -2.779629f, -3.190799f, -3.521900f, -3.375480f, -3.319355f, -3.897389f,
  -3.172334f, -3.594528f, -2.879132f, -2.547777f, -2.921023f, -2.281844f,
  -1.818988f, -2.041771f, -0.618268f, -1.396458f, -0.567153f, -0.285868f,
  -0.088058f, 0.753494f,  2.092413f,  3.215266f,  -3.300277f, -2.748658f,
  -2.315784f, -2.423671f, -2.257283f, -2.269583f, -2.196660f, -2.301076f,
  -2.646516f, -2.271319f, -2.254366f, -2.300102f, -2.217960f, -2.473300f,
  -2.116866f, -2.528246f, -3.314712f, -1.701010f, -0.589040f, -0.088077f,
  0.813112f,  1.702213f,  2.653045f,  3.351749f,  3.243554f,  3.199409f,
  2.437856f,  1.468854f,  0.533039f,  -0.099065f, -0.622643f, -2.200732f,
  -4.228861f, -2.875263f, -1.273956f, -0.433280f, 0.803771f,  1.975043f,
  3.179528f,  3.939064f,  3.454379f,  3.689386f,  3.116411f,  1.970991f,
  0.798406f,  -0.628514f, -1.252546f, -2.825176f, -4.090178f, -3.777448f,
  -3.227314f, -3.479403f, -3.320569f, -3.159372f, -2.729202f, -2.722341f,
  -3.054913f, -2.742923f, -2.612703f, -2.662632f, -2.907314f, -3.117794f,
  -3.102660f, -3.970972f, -4.891357f, -3.935582f, -3.347758f, -2.721924f,
  -2.219011f, -1.702391f, -0.866529f, -0.153743f, 0.107733f,  1.416882f,
  2.572884f,  3.607755f,  3.974820f,  3.997783f,  2.970459f,  0.791687f,
  -1.478921f, -1.228154f, -1.216955f, -1.765932f, -1.951003f, -1.985301f,
  -1.975881f, -1.985593f, -2.422371f, -2.419978f, -2.531288f, -2.951853f,
  -3.071380f, -3.277027f, -3.373539f, -4.462010f, -0.967888f, 0.805524f,
  2.794130f,  3.685984f,  3.745195f,  3.252444f,  2.316108f,  1.399146f,
  -0.136519f, -0.162811f, -1.004357f, -1.667911f, -1.964662f, -2.937579f,
  -3.019533f, -3.942766f, -5.102767f, -3.882073f, -3.532027f, -3.451956f,
  -2.944015f, -2.643064f, -2.529872f, -2.077290f, -2.809965f, -1.803734f,
  -1.783593f, -1.662585f, -1.415484f, -1.392673f, -0.788794f, -1.204819f,
  -1.998864f, -1.182102f, -0.892110f, -1.317415f, -1.359112f, -1.522867f,
  -1.468552f, -1.779072f, -2.332959f, -2.160346f, -2.329387f, -2.631259f,
  -2.744936f, -3.052494f, -2.787363f, -3.442548f, -4.245075f, -3.032172f,
  -2.061609f, -1.768116f, -1.286072f, -0.706587f, -0.192413f, 0.386938f,
  0.716997f,  1.481393f,  2.216702f,  2.737986f,  3.109809f,  3.226084f,
  2.490098f,  -0.095827f, -3.864816f, -3.507248f, -3.128925f, -2.908251f,
  -2.883836f, -2.881411f, -2.524377f, -2.624478f, -2.399573f, -2.367718f,
  -1.918255f, -1.926277f, -1.694584f, -1.723790f, -0.966491f, -1.183115f,
  -1.430687f, 0.872896f,  2.766550f,  3.610080f,  3.578041f,  3.334928f,
  2.586680f,  1.895721f,  1.122195f,  0.488519f,  -0.140689f, -0.799076f,
  -1.222860f, -1.502437f, -1.900969f, -3.206816f,
};

static void generate_hog(const uint8_t *src, int stride, int rows, int cols,
                         float *hist) {
  const float step = (float)PI / BINS;
  float total = 0.1f;
  src += stride;
  for (int r = 1; r < rows - 1; ++r) {
    for (int c = 1; c < cols - 1; ++c) {
      const uint8_t *above = &src[c - stride];
      const uint8_t *below = &src[c + stride];
      const uint8_t *left = &src[c - 1];
      const uint8_t *right = &src[c + 1];
      // Calculate gradient using Sobel fitlers.
      const int dx = (right[-stride] + 2 * right[0] + right[stride]) -
                     (left[-stride] + 2 * left[0] + left[stride]);
      const int dy = (below[-1] + 2 * below[0] + below[1]) -
                     (above[-1] + 2 * above[0] + above[1]);
      if (dx == 0 && dy == 0) continue;
      const int temp = abs(dx) + abs(dy);
      if (!temp) continue;
      total += temp;
      if (dx == 0) {
        hist[0] += temp / 2;
        hist[BINS - 1] += temp / 2;
      } else {
        const float angle = atanf(dy * 1.0f / dx);
        int idx = (int)roundf(angle / step) + BINS / 2;
        idx = AOMMIN(idx, BINS - 1);
        idx = AOMMAX(idx, 0);
        hist[idx] += temp;
      }
    }
    src += stride;
  }

  for (int i = 0; i < BINS; ++i) hist[i] /= total;
}

static void generate_hog_hbd(const uint8_t *src8, int stride, int rows,
                             int cols, float *hist) {
  const float step = (float)PI / BINS;
  float total = 0.1f;
  uint16_t *src = CONVERT_TO_SHORTPTR(src8);
  src += stride;
  for (int r = 1; r < rows - 1; ++r) {
    for (int c = 1; c < cols - 1; ++c) {
      const uint16_t *above = &src[c - stride];
      const uint16_t *below = &src[c + stride];
      const uint16_t *left = &src[c - 1];
      const uint16_t *right = &src[c + 1];
      // Calculate gradient using Sobel fitlers.
      const int dx = (right[-stride] + 2 * right[0] + right[stride]) -
                     (left[-stride] + 2 * left[0] + left[stride]);
      const int dy = (below[-1] + 2 * below[0] + below[1]) -
                     (above[-1] + 2 * above[0] + above[1]);
      if (dx == 0 && dy == 0) continue;
      const int temp = abs(dx) + abs(dy);
      if (!temp) continue;
      total += temp;
      if (dx == 0) {
        hist[0] += temp / 2;
        hist[BINS - 1] += temp / 2;
      } else {
        const float angle = atanf(dy * 1.0f / dx);
        int idx = (int)roundf(angle / step) + BINS / 2;
        idx = AOMMIN(idx, BINS - 1);
        idx = AOMMAX(idx, 0);
        hist[idx] += temp;
      }
    }
    src += stride;
  }

  for (int i = 0; i < BINS; ++i) hist[i] /= total;
}

static void prune_intra_mode_with_hog(const MACROBLOCK *x, BLOCK_SIZE bsize,
                                      float th,
                                      uint8_t *directional_mode_skip_mask) {
  aom_clear_system_state();

  const int bh = block_size_high[bsize];
  const int bw = block_size_wide[bsize];
  const MACROBLOCKD *xd = &x->e_mbd;
  const int rows =
      (xd->mb_to_bottom_edge >= 0) ? bh : (xd->mb_to_bottom_edge >> 3) + bh;
  const int cols =
      (xd->mb_to_right_edge >= 0) ? bw : (xd->mb_to_right_edge >> 3) + bw;
  const int src_stride = x->plane[0].src.stride;
  const uint8_t *src = x->plane[0].src.buf;
  float hist[BINS] = { 0.0f };
  if (is_cur_buf_hbd(xd)) {
    generate_hog_hbd(src, src_stride, rows, cols, hist);
  } else {
    generate_hog(src, src_stride, rows, cols, hist);
  }

  for (int i = 0; i < DIRECTIONAL_MODES; ++i) {
    float this_score = intra_hog_model_bias[i];
    const float *weights = &intra_hog_model_weights[i * BINS];
    for (int j = 0; j < BINS; ++j) {
      this_score += weights[j] * hist[j];
    }
    if (this_score < th) directional_mode_skip_mask[i + 1] = 1;
  }

  aom_clear_system_state();
}

#undef BINS

// This function is used only for intra_only frames
static int64_t rd_pick_intra_sby_mode(const AV1_COMP *const cpi, MACROBLOCK *x,
                                      int *rate, int *rate_tokenonly,
                                      int64_t *distortion, int *skippable,
                                      BLOCK_SIZE bsize, int64_t best_rd,
                                      PICK_MODE_CONTEXT *ctx) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  assert(!is_inter_block(mbmi));
  int64_t best_model_rd = INT64_MAX;
  int is_directional_mode;
  uint8_t directional_mode_skip_mask[INTRA_MODES] = { 0 };
  // Flag to check rd of any intra mode is better than best_rd passed to this
  // function
  int beat_best_rd = 0;
  const int *bmode_costs;
  PALETTE_MODE_INFO *const pmi = &mbmi->palette_mode_info;
  const int try_palette =
      cpi->oxcf.enable_palette &&
      av1_allow_palette(cpi->common.allow_screen_content_tools, mbmi->sb_type);
  uint8_t *best_palette_color_map =
      try_palette ? x->palette_buffer->best_palette_color_map : NULL;
  const MB_MODE_INFO *above_mi = xd->above_mbmi;
  const MB_MODE_INFO *left_mi = xd->left_mbmi;
  const PREDICTION_MODE A = av1_above_block_mode(above_mi);
  const PREDICTION_MODE L = av1_left_block_mode(left_mi);
  const int above_ctx = intra_mode_context[A];
  const int left_ctx = intra_mode_context[L];
  bmode_costs = x->y_mode_costs[above_ctx][left_ctx];

  mbmi->angle_delta[PLANE_TYPE_Y] = 0;
  if (cpi->sf.intra_sf.intra_pruning_with_hog) {
    prune_intra_mode_with_hog(x, bsize,
                              cpi->sf.intra_sf.intra_pruning_with_hog_thresh,
                              directional_mode_skip_mask);
  }
  mbmi->filter_intra_mode_info.use_filter_intra = 0;
  pmi->palette_size[0] = 0;

  // Set params for mode evaluation
  set_mode_eval_params(cpi, x, MODE_EVAL);

  MB_MODE_INFO best_mbmi = *mbmi;
  av1_zero(x->winner_mode_stats);
  x->winner_mode_count = 0;
  // Initialize best mode stats for winner mode processing
  const int txfm_search_done = 1;
  store_winner_mode_stats(
      &cpi->common, x, mbmi, NULL, NULL, NULL, 0, NULL, bsize, best_rd,
      cpi->sf.winner_mode_sf.enable_multiwinner_mode_process, txfm_search_done);
  /* Y Search for intra prediction mode */
  for (int mode_idx = INTRA_MODE_START; mode_idx < INTRA_MODE_END; ++mode_idx) {
    RD_STATS this_rd_stats;
    int this_rate, this_rate_tokenonly, s;
    int64_t this_distortion, this_rd;
    mbmi->mode = intra_rd_search_mode_order[mode_idx];
    if ((!cpi->oxcf.enable_smooth_intra ||
         cpi->sf.intra_sf.disable_smooth_intra) &&
        (mbmi->mode == SMOOTH_PRED || mbmi->mode == SMOOTH_H_PRED ||
         mbmi->mode == SMOOTH_V_PRED))
      continue;
    if (!cpi->oxcf.enable_paeth_intra && mbmi->mode == PAETH_PRED) continue;
    mbmi->angle_delta[PLANE_TYPE_Y] = 0;

    if (model_intra_yrd_and_prune(cpi, x, bsize, bmode_costs[mbmi->mode],
                                  &best_model_rd)) {
      continue;
    }

    is_directional_mode = av1_is_directional_mode(mbmi->mode);
    if (is_directional_mode && directional_mode_skip_mask[mbmi->mode]) continue;
    if (is_directional_mode && av1_use_angle_delta(bsize) &&
        cpi->oxcf.enable_angle_delta) {
      this_rd_stats.rate = INT_MAX;
      rd_pick_intra_angle_sby(cpi, x, &this_rate, &this_rd_stats, bsize,
                              bmode_costs[mbmi->mode], best_rd, &best_model_rd,
                              1);
    } else {
      super_block_yrd(cpi, x, &this_rd_stats, bsize, best_rd);
    }
    this_rate_tokenonly = this_rd_stats.rate;
    this_distortion = this_rd_stats.dist;
    s = this_rd_stats.skip;

    if (this_rate_tokenonly == INT_MAX) continue;

    if (!xd->lossless[mbmi->segment_id] &&
        block_signals_txsize(mbmi->sb_type)) {
      // super_block_yrd above includes the cost of the tx_size in the
      // tokenonly rate, but for intra blocks, tx_size is always coded
      // (prediction granularity), so we account for it in the full rate,
      // not the tokenonly rate.
      this_rate_tokenonly -= tx_size_cost(x, bsize, mbmi->tx_size);
    }
    this_rate =
        this_rd_stats.rate +
        intra_mode_info_cost_y(cpi, x, mbmi, bsize, bmode_costs[mbmi->mode]);
    this_rd = RDCOST(x->rdmult, this_rate, this_distortion);
    // Collect mode stats for multiwinner mode processing
    store_winner_mode_stats(
        &cpi->common, x, mbmi, NULL, NULL, NULL, 0, NULL, bsize, this_rd,
        cpi->sf.winner_mode_sf.enable_multiwinner_mode_process,
        txfm_search_done);
    if (this_rd < best_rd) {
      best_mbmi = *mbmi;
      best_rd = this_rd;
      // Setting beat_best_rd flag because current mode rd is better than
      // best_rd passed to this function
      beat_best_rd = 1;
      *rate = this_rate;
      *rate_tokenonly = this_rate_tokenonly;
      *distortion = this_distortion;
      *skippable = s;
      memcpy(ctx->blk_skip, x->blk_skip,
             sizeof(x->blk_skip[0]) * ctx->num_4x4_blk);
      av1_copy_array(ctx->tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);
    }
  }

  if (try_palette) {
    rd_pick_palette_intra_sby(
        cpi, x, bsize, bmode_costs[DC_PRED], &best_mbmi, best_palette_color_map,
        &best_rd, &best_model_rd, rate, rate_tokenonly, distortion, skippable,
        &beat_best_rd, ctx, ctx->blk_skip, ctx->tx_type_map);
  }

  if (beat_best_rd && av1_filter_intra_allowed_bsize(&cpi->common, bsize)) {
    if (rd_pick_filter_intra_sby(cpi, x, rate, rate_tokenonly, distortion,
                                 skippable, bsize, bmode_costs[DC_PRED],
                                 &best_rd, &best_model_rd, ctx)) {
      best_mbmi = *mbmi;
    }
  }
  // No mode is identified with less rd value than best_rd passed to this
  // function. In such cases winner mode processing is not necessary and return
  // best_rd as INT64_MAX to indicate best mode is not identified
  if (!beat_best_rd) return INT64_MAX;

  // In multi-winner mode processing, perform tx search for few best modes
  // identified during mode evaluation. Winner mode processing uses best tx
  // configuration for tx search.
  if (cpi->sf.winner_mode_sf.enable_multiwinner_mode_process) {
    int best_mode_idx = 0;
    int block_width, block_height;
    uint8_t *color_map_dst = xd->plane[PLANE_TYPE_Y].color_index_map;
    av1_get_block_dimensions(bsize, AOM_PLANE_Y, xd, &block_width,
                             &block_height, NULL, NULL);

    for (int mode_idx = 0; mode_idx < x->winner_mode_count; mode_idx++) {
      *mbmi = x->winner_mode_stats[mode_idx].mbmi;
      if (is_winner_mode_processing_enabled(cpi, mbmi, mbmi->mode)) {
        // Restore color_map of palette mode before winner mode processing
        if (mbmi->palette_mode_info.palette_size[0] > 0) {
          uint8_t *color_map_src =
              x->winner_mode_stats[mode_idx].color_index_map;
          memcpy(color_map_dst, color_map_src,
                 block_width * block_height * sizeof(*color_map_src));
        }
        // Set params for winner mode evaluation
        set_mode_eval_params(cpi, x, WINNER_MODE_EVAL);

        // Winner mode processing
        // If previous searches use only the default tx type/no R-D optimization
        // of quantized coeffs, do an extra search for the best tx type/better
        // R-D optimization of quantized coeffs
        if (intra_block_yrd(cpi, x, bsize, bmode_costs, &best_rd, rate,
                            rate_tokenonly, distortion, skippable, &best_mbmi,
                            ctx))
          best_mode_idx = mode_idx;
      }
    }
    // Copy color_map of palette mode for final winner mode
    if (best_mbmi.palette_mode_info.palette_size[0] > 0) {
      uint8_t *color_map_src =
          x->winner_mode_stats[best_mode_idx].color_index_map;
      memcpy(color_map_dst, color_map_src,
             block_width * block_height * sizeof(*color_map_src));
    }
  } else {
    // If previous searches use only the default tx type/no R-D optimization of
    // quantized coeffs, do an extra search for the best tx type/better R-D
    // optimization of quantized coeffs
    if (is_winner_mode_processing_enabled(cpi, mbmi, best_mbmi.mode)) {
      // Set params for winner mode evaluation
      set_mode_eval_params(cpi, x, WINNER_MODE_EVAL);
      *mbmi = best_mbmi;
      intra_block_yrd(cpi, x, bsize, bmode_costs, &best_rd, rate,
                      rate_tokenonly, distortion, skippable, &best_mbmi, ctx);
    }
  }
  *mbmi = best_mbmi;
  av1_copy_array(xd->tx_type_map, ctx->tx_type_map, ctx->num_4x4_blk);
  return best_rd;
}

// Return value 0: early termination triggered, no valid rd cost available;
//              1: rd cost values are valid.
static int super_block_uvrd(const AV1_COMP *const cpi, MACROBLOCK *x,
                            RD_STATS *rd_stats, BLOCK_SIZE bsize,
                            int64_t ref_best_rd) {
  av1_init_rd_stats(rd_stats);
  int is_cost_valid = 1;
  if (ref_best_rd < 0) is_cost_valid = 0;
  if (x->skip_chroma_rd || !is_cost_valid) return is_cost_valid;

  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  struct macroblockd_plane *const pd = &xd->plane[AOM_PLANE_U];
  int plane;
  const int is_inter = is_inter_block(mbmi);
  int64_t this_rd = 0, skip_rd = 0;
  const BLOCK_SIZE plane_bsize =
      get_plane_block_size(bsize, pd->subsampling_x, pd->subsampling_y);

  if (is_inter && is_cost_valid) {
    for (plane = 1; plane < MAX_MB_PLANE; ++plane)
      av1_subtract_plane(x, plane_bsize, plane);
  }

  if (is_cost_valid) {
    const TX_SIZE uv_tx_size = av1_get_tx_size(AOM_PLANE_U, xd);
    for (plane = 1; plane < MAX_MB_PLANE; ++plane) {
      RD_STATS pn_rd_stats;
      int64_t chroma_ref_best_rd = ref_best_rd;
      // For inter blocks, refined ref_best_rd is used for early exit
      // For intra blocks, even though current rd crosses ref_best_rd, early
      // exit is not recommended as current rd is used for gating subsequent
      // modes as well (say, for angular modes)
      // TODO(any): Extend the early exit mechanism for intra modes as well
      if (cpi->sf.inter_sf.perform_best_rd_based_gating_for_chroma &&
          is_inter && chroma_ref_best_rd != INT64_MAX)
        chroma_ref_best_rd = ref_best_rd - AOMMIN(this_rd, skip_rd);
      txfm_rd_in_plane(x, cpi, &pn_rd_stats, chroma_ref_best_rd, 0, plane,
                       plane_bsize, uv_tx_size,
                       cpi->sf.rd_sf.use_fast_coef_costing, FTXS_NONE, 0);
      if (pn_rd_stats.rate == INT_MAX) {
        is_cost_valid = 0;
        break;
      }
      av1_merge_rd_stats(rd_stats, &pn_rd_stats);
      this_rd = RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist);
      skip_rd = RDCOST(x->rdmult, 0, rd_stats->sse);
      if (AOMMIN(this_rd, skip_rd) > ref_best_rd) {
        is_cost_valid = 0;
        break;
      }
    }
  }

  if (!is_cost_valid) {
    // reset cost value
    av1_invalid_rd_stats(rd_stats);
  }

  return is_cost_valid;
}

// Pick transform type for a transform block of tx_size.
static AOM_INLINE void tx_type_rd(const AV1_COMP *cpi, MACROBLOCK *x,
                                  TX_SIZE tx_size, int blk_row, int blk_col,
                                  int plane, int block, int plane_bsize,
                                  TXB_CTX *txb_ctx, RD_STATS *rd_stats,
                                  FAST_TX_SEARCH_MODE ftxs_mode,
                                  int64_t ref_rdcost,
                                  TXB_RD_INFO *rd_info_array) {
  const struct macroblock_plane *const p = &x->plane[plane];
  const uint16_t cur_joint_ctx =
      (txb_ctx->dc_sign_ctx << 8) + txb_ctx->txb_skip_ctx;
  MACROBLOCKD *xd = &x->e_mbd;
  const int tx_type_map_idx =
      plane ? 0 : blk_row * xd->tx_type_map_stride + blk_col;
  // Look up RD and terminate early in case when we've already processed exactly
  // the same residual with exactly the same entropy context.
  if (rd_info_array != NULL && rd_info_array->valid &&
      rd_info_array->entropy_context == cur_joint_ctx) {
    if (plane == 0) xd->tx_type_map[tx_type_map_idx] = rd_info_array->tx_type;
    const TX_TYPE ref_tx_type =
        av1_get_tx_type(&x->e_mbd, get_plane_type(plane), blk_row, blk_col,
                        tx_size, cpi->common.reduced_tx_set_used);
    if (ref_tx_type == rd_info_array->tx_type) {
      rd_stats->rate += rd_info_array->rate;
      rd_stats->dist += rd_info_array->dist;
      rd_stats->sse += rd_info_array->sse;
      rd_stats->skip &= rd_info_array->eob == 0;
      p->eobs[block] = rd_info_array->eob;
      p->txb_entropy_ctx[block] = rd_info_array->txb_entropy_ctx;
      return;
    }
  }

  RD_STATS this_rd_stats;
  search_txk_type(cpi, x, plane, block, blk_row, blk_col, plane_bsize, tx_size,
                  txb_ctx, ftxs_mode, 0, 0, ref_rdcost, &this_rd_stats);

  av1_merge_rd_stats(rd_stats, &this_rd_stats);

  // Save RD results for possible reuse in future.
  if (rd_info_array != NULL) {
    rd_info_array->valid = 1;
    rd_info_array->entropy_context = cur_joint_ctx;
    rd_info_array->rate = this_rd_stats.rate;
    rd_info_array->dist = this_rd_stats.dist;
    rd_info_array->sse = this_rd_stats.sse;
    rd_info_array->eob = p->eobs[block];
    rd_info_array->txb_entropy_ctx = p->txb_entropy_ctx[block];
    if (plane == 0) rd_info_array->tx_type = xd->tx_type_map[tx_type_map_idx];
  }
}

static float get_dev(float mean, double x2_sum, int num) {
  const float e_x2 = (float)(x2_sum / num);
  const float diff = e_x2 - mean * mean;
  const float dev = (diff > 0) ? sqrtf(diff) : 0;
  return dev;
}

// Feature used by the model to predict tx split: the mean and standard
// deviation values of the block and sub-blocks.
static AOM_INLINE void get_mean_dev_features(const int16_t *data, int stride,
                                             int bw, int bh, float *feature) {
  const int16_t *const data_ptr = &data[0];
  const int subh = (bh >= bw) ? (bh >> 1) : bh;
  const int subw = (bw >= bh) ? (bw >> 1) : bw;
  const int num = bw * bh;
  const int sub_num = subw * subh;
  int feature_idx = 2;
  int total_x_sum = 0;
  int64_t total_x2_sum = 0;
  int blk_idx = 0;
  double mean2_sum = 0.0f;
  float dev_sum = 0.0f;

  for (int row = 0; row < bh; row += subh) {
    for (int col = 0; col < bw; col += subw) {
      int x_sum;
      int64_t x2_sum;
      // TODO(any): Write a SIMD version. Clear registers.
      aom_get_blk_sse_sum(data_ptr + row * stride + col, stride, subw, subh,
                          &x_sum, &x2_sum);
      total_x_sum += x_sum;
      total_x2_sum += x2_sum;

      aom_clear_system_state();
      const float mean = (float)x_sum / sub_num;
      const float dev = get_dev(mean, (double)x2_sum, sub_num);
      feature[feature_idx++] = mean;
      feature[feature_idx++] = dev;
      mean2_sum += (double)(mean * mean);
      dev_sum += dev;
      blk_idx++;
    }
  }

  const float lvl0_mean = (float)total_x_sum / num;
  feature[0] = lvl0_mean;
  feature[1] = get_dev(lvl0_mean, (double)total_x2_sum, num);

  if (blk_idx > 1) {
    // Deviation of means.
    feature[feature_idx++] = get_dev(lvl0_mean, mean2_sum, blk_idx);
    // Mean of deviations.
    feature[feature_idx++] = dev_sum / blk_idx;
  }
}

static int ml_predict_tx_split(MACROBLOCK *x, BLOCK_SIZE bsize, int blk_row,
                               int blk_col, TX_SIZE tx_size) {
  const NN_CONFIG *nn_config = av1_tx_split_nnconfig_map[tx_size];
  if (!nn_config) return -1;

  const int diff_stride = block_size_wide[bsize];
  const int16_t *diff =
      x->plane[0].src_diff + 4 * blk_row * diff_stride + 4 * blk_col;
  const int bw = tx_size_wide[tx_size];
  const int bh = tx_size_high[tx_size];
  aom_clear_system_state();

  float features[64] = { 0.0f };
  get_mean_dev_features(diff, diff_stride, bw, bh, features);

  float score = 0.0f;
  av1_nn_predict(features, nn_config, 1, &score);
  aom_clear_system_state();

  int int_score = (int)(score * 10000);
  return clamp(int_score, -80000, 80000);
}

typedef struct {
  int64_t rd;
  int txb_entropy_ctx;
  TX_TYPE tx_type;
} TxCandidateInfo;

static AOM_INLINE void try_tx_block_no_split(
    const AV1_COMP *cpi, MACROBLOCK *x, int blk_row, int blk_col, int block,
    TX_SIZE tx_size, int depth, BLOCK_SIZE plane_bsize,
    const ENTROPY_CONTEXT *ta, const ENTROPY_CONTEXT *tl,
    int txfm_partition_ctx, RD_STATS *rd_stats, int64_t ref_best_rd,
    FAST_TX_SEARCH_MODE ftxs_mode, TXB_RD_INFO_NODE *rd_info_node,
    TxCandidateInfo *no_split) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  struct macroblock_plane *const p = &x->plane[0];
  const int bw = mi_size_wide[plane_bsize];

  no_split->rd = INT64_MAX;
  no_split->txb_entropy_ctx = 0;
  no_split->tx_type = TX_TYPES;

  const ENTROPY_CONTEXT *const pta = ta + blk_col;
  const ENTROPY_CONTEXT *const ptl = tl + blk_row;

  const TX_SIZE txs_ctx = get_txsize_entropy_ctx(tx_size);
  TXB_CTX txb_ctx;
  get_txb_ctx(plane_bsize, tx_size, 0, pta, ptl, &txb_ctx);
  const int zero_blk_rate = x->coeff_costs[txs_ctx][PLANE_TYPE_Y]
                                .txb_skip_cost[txb_ctx.txb_skip_ctx][1];
  rd_stats->zero_rate = zero_blk_rate;
  const int index = av1_get_txb_size_index(plane_bsize, blk_row, blk_col);
  mbmi->inter_tx_size[index] = tx_size;
  tx_type_rd(cpi, x, tx_size, blk_row, blk_col, 0, block, plane_bsize, &txb_ctx,
             rd_stats, ftxs_mode, ref_best_rd,
             rd_info_node != NULL ? rd_info_node->rd_info_array : NULL);
  assert(rd_stats->rate < INT_MAX);

  if ((RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist) >=
           RDCOST(x->rdmult, zero_blk_rate, rd_stats->sse) ||
       rd_stats->skip == 1) &&
      !xd->lossless[mbmi->segment_id]) {
#if CONFIG_RD_DEBUG
    av1_update_txb_coeff_cost(rd_stats, 0, tx_size, blk_row, blk_col,
                              zero_blk_rate - rd_stats->rate);
#endif  // CONFIG_RD_DEBUG
    rd_stats->rate = zero_blk_rate;
    rd_stats->dist = rd_stats->sse;
    rd_stats->skip = 1;
    set_blk_skip(x, 0, blk_row * bw + blk_col, 1);
    p->eobs[block] = 0;
    update_txk_array(xd, blk_row, blk_col, tx_size, DCT_DCT);
  } else {
    set_blk_skip(x, 0, blk_row * bw + blk_col, 0);
    rd_stats->skip = 0;
  }

  if (tx_size > TX_4X4 && depth < MAX_VARTX_DEPTH)
    rd_stats->rate += x->txfm_partition_cost[txfm_partition_ctx][0];

  no_split->rd = RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist);
  no_split->txb_entropy_ctx = p->txb_entropy_ctx[block];
  no_split->tx_type =
      xd->tx_type_map[blk_row * xd->tx_type_map_stride + blk_col];
}

static AOM_INLINE void select_tx_block(
    const AV1_COMP *cpi, MACROBLOCK *x, int blk_row, int blk_col, int block,
    TX_SIZE tx_size, int depth, BLOCK_SIZE plane_bsize, ENTROPY_CONTEXT *ta,
    ENTROPY_CONTEXT *tl, TXFM_CONTEXT *tx_above, TXFM_CONTEXT *tx_left,
    RD_STATS *rd_stats, int64_t prev_level_rd, int64_t ref_best_rd,
    int *is_cost_valid, FAST_TX_SEARCH_MODE ftxs_mode,
    TXB_RD_INFO_NODE *rd_info_node);

static AOM_INLINE void try_tx_block_split(
    const AV1_COMP *cpi, MACROBLOCK *x, int blk_row, int blk_col, int block,
    TX_SIZE tx_size, int depth, BLOCK_SIZE plane_bsize, ENTROPY_CONTEXT *ta,
    ENTROPY_CONTEXT *tl, TXFM_CONTEXT *tx_above, TXFM_CONTEXT *tx_left,
    int txfm_partition_ctx, int64_t no_split_rd, int64_t ref_best_rd,
    FAST_TX_SEARCH_MODE ftxs_mode, TXB_RD_INFO_NODE *rd_info_node,
    RD_STATS *split_rd_stats, int64_t *split_rd) {
  assert(tx_size < TX_SIZES_ALL);
  MACROBLOCKD *const xd = &x->e_mbd;
  const int max_blocks_high = max_block_high(xd, plane_bsize, 0);
  const int max_blocks_wide = max_block_wide(xd, plane_bsize, 0);
  const TX_SIZE sub_txs = sub_tx_size_map[tx_size];
  const int bsw = tx_size_wide_unit[sub_txs];
  const int bsh = tx_size_high_unit[sub_txs];
  const int sub_step = bsw * bsh;
  const int nblks =
      (tx_size_high_unit[tx_size] / bsh) * (tx_size_wide_unit[tx_size] / bsw);
  assert(nblks > 0);
  int blk_idx = 0;
  int64_t tmp_rd = 0;
  *split_rd = INT64_MAX;
  split_rd_stats->rate = x->txfm_partition_cost[txfm_partition_ctx][1];

  for (int r = 0; r < tx_size_high_unit[tx_size]; r += bsh) {
    for (int c = 0; c < tx_size_wide_unit[tx_size]; c += bsw, ++blk_idx) {
      assert(blk_idx < 4);
      const int offsetr = blk_row + r;
      const int offsetc = blk_col + c;
      if (offsetr >= max_blocks_high || offsetc >= max_blocks_wide) continue;

      RD_STATS this_rd_stats;
      int this_cost_valid = 1;
      select_tx_block(
          cpi, x, offsetr, offsetc, block, sub_txs, depth + 1, plane_bsize, ta,
          tl, tx_above, tx_left, &this_rd_stats, no_split_rd / nblks,
          ref_best_rd - tmp_rd, &this_cost_valid, ftxs_mode,
          (rd_info_node != NULL) ? rd_info_node->children[blk_idx] : NULL);
      if (!this_cost_valid) return;
      av1_merge_rd_stats(split_rd_stats, &this_rd_stats);
      tmp_rd = RDCOST(x->rdmult, split_rd_stats->rate, split_rd_stats->dist);
      if (no_split_rd < tmp_rd) return;
      block += sub_step;
    }
  }

  *split_rd = tmp_rd;
}

// Search for the best tx partition/type for a given luma block.
static AOM_INLINE void select_tx_block(
    const AV1_COMP *cpi, MACROBLOCK *x, int blk_row, int blk_col, int block,
    TX_SIZE tx_size, int depth, BLOCK_SIZE plane_bsize, ENTROPY_CONTEXT *ta,
    ENTROPY_CONTEXT *tl, TXFM_CONTEXT *tx_above, TXFM_CONTEXT *tx_left,
    RD_STATS *rd_stats, int64_t prev_level_rd, int64_t ref_best_rd,
    int *is_cost_valid, FAST_TX_SEARCH_MODE ftxs_mode,
    TXB_RD_INFO_NODE *rd_info_node) {
  assert(tx_size < TX_SIZES_ALL);
  av1_init_rd_stats(rd_stats);
  if (ref_best_rd < 0) {
    *is_cost_valid = 0;
    return;
  }

  MACROBLOCKD *const xd = &x->e_mbd;
  const int max_blocks_high = max_block_high(xd, plane_bsize, 0);
  const int max_blocks_wide = max_block_wide(xd, plane_bsize, 0);
  if (blk_row >= max_blocks_high || blk_col >= max_blocks_wide) return;

  const int bw = mi_size_wide[plane_bsize];
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const int ctx = txfm_partition_context(tx_above + blk_col, tx_left + blk_row,
                                         mbmi->sb_type, tx_size);
  struct macroblock_plane *const p = &x->plane[0];

  const int try_no_split =
      cpi->oxcf.enable_tx64 || txsize_sqr_up_map[tx_size] != TX_64X64;
  int try_split = tx_size > TX_4X4 && depth < MAX_VARTX_DEPTH;
#if CONFIG_DIST_8X8
  if (x->using_dist_8x8)
    try_split &= tx_size_wide[tx_size] >= 16 && tx_size_high[tx_size] >= 16;
#endif
  TxCandidateInfo no_split = { INT64_MAX, 0, TX_TYPES };

  // TX no split
  if (try_no_split) {
    try_tx_block_no_split(cpi, x, blk_row, blk_col, block, tx_size, depth,
                          plane_bsize, ta, tl, ctx, rd_stats, ref_best_rd,
                          ftxs_mode, rd_info_node, &no_split);

    if (cpi->sf.tx_sf.adaptive_txb_search_level &&
        (no_split.rd -
         (no_split.rd >> (1 + cpi->sf.tx_sf.adaptive_txb_search_level))) >
            ref_best_rd) {
      *is_cost_valid = 0;
      return;
    }

    if (cpi->sf.tx_sf.txb_split_cap) {
      if (p->eobs[block] == 0) try_split = 0;
    }

    if (cpi->sf.tx_sf.adaptive_txb_search_level &&
        (no_split.rd -
         (no_split.rd >> (2 + cpi->sf.tx_sf.adaptive_txb_search_level))) >
            prev_level_rd) {
      try_split = 0;
    }
  }

  if (x->e_mbd.bd == 8 && try_split &&
      !(ref_best_rd == INT64_MAX && no_split.rd == INT64_MAX)) {
    const int threshold = cpi->sf.tx_sf.tx_type_search.ml_tx_split_thresh;
    if (threshold >= 0) {
      const int split_score =
          ml_predict_tx_split(x, plane_bsize, blk_row, blk_col, tx_size);
      if (split_score < -threshold) try_split = 0;
    }
  }

  // TX split
  int64_t split_rd = INT64_MAX;
  RD_STATS split_rd_stats;
  av1_init_rd_stats(&split_rd_stats);
  if (try_split) {
    try_tx_block_split(cpi, x, blk_row, blk_col, block, tx_size, depth,
                       plane_bsize, ta, tl, tx_above, tx_left, ctx, no_split.rd,
                       AOMMIN(no_split.rd, ref_best_rd), ftxs_mode,
                       rd_info_node, &split_rd_stats, &split_rd);
  }

  if (no_split.rd < split_rd) {
    ENTROPY_CONTEXT *pta = ta + blk_col;
    ENTROPY_CONTEXT *ptl = tl + blk_row;
    const TX_SIZE tx_size_selected = tx_size;
    p->txb_entropy_ctx[block] = no_split.txb_entropy_ctx;
    av1_set_txb_context(x, 0, block, tx_size_selected, pta, ptl);
    txfm_partition_update(tx_above + blk_col, tx_left + blk_row, tx_size,
                          tx_size);
    for (int idy = 0; idy < tx_size_high_unit[tx_size]; ++idy) {
      for (int idx = 0; idx < tx_size_wide_unit[tx_size]; ++idx) {
        const int index =
            av1_get_txb_size_index(plane_bsize, blk_row + idy, blk_col + idx);
        mbmi->inter_tx_size[index] = tx_size_selected;
      }
    }
    mbmi->tx_size = tx_size_selected;
    update_txk_array(xd, blk_row, blk_col, tx_size, no_split.tx_type);
    set_blk_skip(x, 0, blk_row * bw + blk_col, rd_stats->skip);
  } else {
    *rd_stats = split_rd_stats;
    if (split_rd == INT64_MAX) *is_cost_valid = 0;
  }
}

static int64_t select_tx_size_and_type(const AV1_COMP *cpi, MACROBLOCK *x,
                                       RD_STATS *rd_stats, BLOCK_SIZE bsize,
                                       int64_t ref_best_rd,
                                       TXB_RD_INFO_NODE *rd_info_tree) {
  MACROBLOCKD *const xd = &x->e_mbd;
  assert(is_inter_block(xd->mi[0]));
  assert(bsize < BLOCK_SIZES_ALL);

  // TODO(debargha): enable this as a speed feature where the
  // select_inter_block_yrd() function above will use a simplified search
  // such as not using full optimize, but the inter_block_yrd() function
  // will use more complex search given that the transform partitions have
  // already been decided.

  const int fast_tx_search = x->tx_size_search_method > USE_FULL_RD;
  int64_t rd_thresh = ref_best_rd;
  if (fast_tx_search && rd_thresh < INT64_MAX) {
    if (INT64_MAX - rd_thresh > (rd_thresh >> 3)) rd_thresh += (rd_thresh >> 3);
  }
  assert(rd_thresh > 0);

  const FAST_TX_SEARCH_MODE ftxs_mode =
      fast_tx_search ? FTXS_DCT_AND_1D_DCT_ONLY : FTXS_NONE;
  const struct macroblockd_plane *const pd = &xd->plane[0];
  const BLOCK_SIZE plane_bsize =
      get_plane_block_size(bsize, pd->subsampling_x, pd->subsampling_y);
  assert(plane_bsize < BLOCK_SIZES_ALL);
  const int mi_width = mi_size_wide[plane_bsize];
  const int mi_height = mi_size_high[plane_bsize];
  ENTROPY_CONTEXT ctxa[MAX_MIB_SIZE];
  ENTROPY_CONTEXT ctxl[MAX_MIB_SIZE];
  TXFM_CONTEXT tx_above[MAX_MIB_SIZE];
  TXFM_CONTEXT tx_left[MAX_MIB_SIZE];
  av1_get_entropy_contexts(bsize, pd, ctxa, ctxl);
  memcpy(tx_above, xd->above_txfm_context, sizeof(TXFM_CONTEXT) * mi_width);
  memcpy(tx_left, xd->left_txfm_context, sizeof(TXFM_CONTEXT) * mi_height);

  const int skip_ctx = av1_get_skip_context(xd);
  const int s0 = x->skip_cost[skip_ctx][0];
  const int s1 = x->skip_cost[skip_ctx][1];
  const int init_depth = get_search_init_depth(mi_width, mi_height, 1, &cpi->sf,
                                               x->tx_size_search_method);
  const TX_SIZE max_tx_size = max_txsize_rect_lookup[plane_bsize];
  const int bh = tx_size_high_unit[max_tx_size];
  const int bw = tx_size_wide_unit[max_tx_size];
  const int step = bw * bh;
  int64_t skip_rd = RDCOST(x->rdmult, s1, 0);
  int64_t this_rd = RDCOST(x->rdmult, s0, 0);
  int block = 0;

  av1_init_rd_stats(rd_stats);
  for (int idy = 0; idy < mi_height; idy += bh) {
    for (int idx = 0; idx < mi_width; idx += bw) {
      const int64_t best_rd_sofar =
          (rd_thresh == INT64_MAX) ? INT64_MAX
                                   : (rd_thresh - (AOMMIN(skip_rd, this_rd)));
      int is_cost_valid = 1;
      RD_STATS pn_rd_stats;
      select_tx_block(cpi, x, idy, idx, block, max_tx_size, init_depth,
                      plane_bsize, ctxa, ctxl, tx_above, tx_left, &pn_rd_stats,
                      INT64_MAX, best_rd_sofar, &is_cost_valid, ftxs_mode,
                      rd_info_tree);
      if (!is_cost_valid || pn_rd_stats.rate == INT_MAX) {
        av1_invalid_rd_stats(rd_stats);
        return INT64_MAX;
      }
      av1_merge_rd_stats(rd_stats, &pn_rd_stats);
      skip_rd = RDCOST(x->rdmult, s1, rd_stats->sse);
      this_rd = RDCOST(x->rdmult, rd_stats->rate + s0, rd_stats->dist);
      block += step;
      if (rd_info_tree != NULL) rd_info_tree += 1;
    }
  }

  if (skip_rd <= this_rd) {
    rd_stats->skip = 1;
  } else {
    rd_stats->skip = 0;
  }

  if (rd_stats->rate == INT_MAX) return INT64_MAX;

  // If fast_tx_search is true, only DCT and 1D DCT were tested in
  // select_inter_block_yrd() above. Do a better search for tx type with
  // tx sizes already decided.
  if (fast_tx_search) {
    if (!inter_block_yrd(cpi, x, rd_stats, bsize, ref_best_rd, FTXS_NONE))
      return INT64_MAX;
  }

  int64_t rd;
  if (rd_stats->skip) {
    rd = RDCOST(x->rdmult, s1, rd_stats->sse);
  } else {
    rd = RDCOST(x->rdmult, rd_stats->rate + s0, rd_stats->dist);
    if (!xd->lossless[xd->mi[0]->segment_id])
      rd = AOMMIN(rd, RDCOST(x->rdmult, s1, rd_stats->sse));
  }

  return rd;
}

// Finds rd cost for a y block, given the transform size partitions
static AOM_INLINE void tx_block_yrd(
    const AV1_COMP *cpi, MACROBLOCK *x, int blk_row, int blk_col, int block,
    TX_SIZE tx_size, BLOCK_SIZE plane_bsize, int depth,
    ENTROPY_CONTEXT *above_ctx, ENTROPY_CONTEXT *left_ctx,
    TXFM_CONTEXT *tx_above, TXFM_CONTEXT *tx_left, int64_t ref_best_rd,
    RD_STATS *rd_stats, FAST_TX_SEARCH_MODE ftxs_mode) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const int max_blocks_high = max_block_high(xd, plane_bsize, 0);
  const int max_blocks_wide = max_block_wide(xd, plane_bsize, 0);

  assert(tx_size < TX_SIZES_ALL);

  if (blk_row >= max_blocks_high || blk_col >= max_blocks_wide) return;

  const TX_SIZE plane_tx_size = mbmi->inter_tx_size[av1_get_txb_size_index(
      plane_bsize, blk_row, blk_col)];

  int ctx = txfm_partition_context(tx_above + blk_col, tx_left + blk_row,
                                   mbmi->sb_type, tx_size);

  av1_init_rd_stats(rd_stats);
  if (tx_size == plane_tx_size) {
    ENTROPY_CONTEXT *ta = above_ctx + blk_col;
    ENTROPY_CONTEXT *tl = left_ctx + blk_row;
    const TX_SIZE txs_ctx = get_txsize_entropy_ctx(tx_size);
    TXB_CTX txb_ctx;
    get_txb_ctx(plane_bsize, tx_size, 0, ta, tl, &txb_ctx);

    const int zero_blk_rate = x->coeff_costs[txs_ctx][get_plane_type(0)]
                                  .txb_skip_cost[txb_ctx.txb_skip_ctx][1];
    rd_stats->zero_rate = zero_blk_rate;
    tx_type_rd(cpi, x, tx_size, blk_row, blk_col, 0, block, plane_bsize,
               &txb_ctx, rd_stats, ftxs_mode, ref_best_rd, NULL);
    const int mi_width = mi_size_wide[plane_bsize];
    if (RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist) >=
            RDCOST(x->rdmult, zero_blk_rate, rd_stats->sse) ||
        rd_stats->skip == 1) {
      rd_stats->rate = zero_blk_rate;
      rd_stats->dist = rd_stats->sse;
      rd_stats->skip = 1;
      set_blk_skip(x, 0, blk_row * mi_width + blk_col, 1);
      x->plane[0].eobs[block] = 0;
      x->plane[0].txb_entropy_ctx[block] = 0;
      update_txk_array(xd, blk_row, blk_col, tx_size, DCT_DCT);
    } else {
      rd_stats->skip = 0;
      set_blk_skip(x, 0, blk_row * mi_width + blk_col, 0);
    }
    if (tx_size > TX_4X4 && depth < MAX_VARTX_DEPTH)
      rd_stats->rate += x->txfm_partition_cost[ctx][0];
    av1_set_txb_context(x, 0, block, tx_size, ta, tl);
    txfm_partition_update(tx_above + blk_col, tx_left + blk_row, tx_size,
                          tx_size);
  } else {
    const TX_SIZE sub_txs = sub_tx_size_map[tx_size];
    const int bsw = tx_size_wide_unit[sub_txs];
    const int bsh = tx_size_high_unit[sub_txs];
    const int step = bsh * bsw;
    RD_STATS pn_rd_stats;
    int64_t this_rd = 0;
    assert(bsw > 0 && bsh > 0);

    for (int row = 0; row < tx_size_high_unit[tx_size]; row += bsh) {
      for (int col = 0; col < tx_size_wide_unit[tx_size]; col += bsw) {
        const int offsetr = blk_row + row;
        const int offsetc = blk_col + col;

        if (offsetr >= max_blocks_high || offsetc >= max_blocks_wide) continue;

        av1_init_rd_stats(&pn_rd_stats);
        tx_block_yrd(cpi, x, offsetr, offsetc, block, sub_txs, plane_bsize,
                     depth + 1, above_ctx, left_ctx, tx_above, tx_left,
                     ref_best_rd - this_rd, &pn_rd_stats, ftxs_mode);
        if (pn_rd_stats.rate == INT_MAX) {
          av1_invalid_rd_stats(rd_stats);
          return;
        }
        av1_merge_rd_stats(rd_stats, &pn_rd_stats);
        this_rd += RDCOST(x->rdmult, pn_rd_stats.rate, pn_rd_stats.dist);
        block += step;
      }
    }

    if (tx_size > TX_4X4 && depth < MAX_VARTX_DEPTH)
      rd_stats->rate += x->txfm_partition_cost[ctx][1];
  }
}

// Return value 0: early termination triggered, no valid rd cost available;
//              1: rd cost values are valid.
static int inter_block_yrd(const AV1_COMP *cpi, MACROBLOCK *x,
                           RD_STATS *rd_stats, BLOCK_SIZE bsize,
                           int64_t ref_best_rd, FAST_TX_SEARCH_MODE ftxs_mode) {
  MACROBLOCKD *const xd = &x->e_mbd;
  int is_cost_valid = 1;
  int64_t this_rd = 0;

  if (ref_best_rd < 0) is_cost_valid = 0;

  av1_init_rd_stats(rd_stats);

  if (is_cost_valid) {
    const struct macroblockd_plane *const pd = &xd->plane[0];
    const BLOCK_SIZE plane_bsize =
        get_plane_block_size(bsize, pd->subsampling_x, pd->subsampling_y);
    const int mi_width = mi_size_wide[plane_bsize];
    const int mi_height = mi_size_high[plane_bsize];
    const TX_SIZE max_tx_size = get_vartx_max_txsize(xd, plane_bsize, 0);
    const int bh = tx_size_high_unit[max_tx_size];
    const int bw = tx_size_wide_unit[max_tx_size];
    const int init_depth = get_search_init_depth(
        mi_width, mi_height, 1, &cpi->sf, x->tx_size_search_method);
    int idx, idy;
    int block = 0;
    int step = tx_size_wide_unit[max_tx_size] * tx_size_high_unit[max_tx_size];
    ENTROPY_CONTEXT ctxa[MAX_MIB_SIZE];
    ENTROPY_CONTEXT ctxl[MAX_MIB_SIZE];
    TXFM_CONTEXT tx_above[MAX_MIB_SIZE];
    TXFM_CONTEXT tx_left[MAX_MIB_SIZE];
    RD_STATS pn_rd_stats;

    av1_get_entropy_contexts(bsize, pd, ctxa, ctxl);
    memcpy(tx_above, xd->above_txfm_context, sizeof(TXFM_CONTEXT) * mi_width);
    memcpy(tx_left, xd->left_txfm_context, sizeof(TXFM_CONTEXT) * mi_height);

    for (idy = 0; idy < mi_height; idy += bh) {
      for (idx = 0; idx < mi_width; idx += bw) {
        av1_init_rd_stats(&pn_rd_stats);
        tx_block_yrd(cpi, x, idy, idx, block, max_tx_size, plane_bsize,
                     init_depth, ctxa, ctxl, tx_above, tx_left,
                     ref_best_rd - this_rd, &pn_rd_stats, ftxs_mode);
        if (pn_rd_stats.rate == INT_MAX) {
          av1_invalid_rd_stats(rd_stats);
          return 0;
        }
        av1_merge_rd_stats(rd_stats, &pn_rd_stats);
        this_rd +=
            AOMMIN(RDCOST(x->rdmult, pn_rd_stats.rate, pn_rd_stats.dist),
                   RDCOST(x->rdmult, pn_rd_stats.zero_rate, pn_rd_stats.sse));
        block += step;
      }
    }
  }

  const int skip_ctx = av1_get_skip_context(xd);
  const int s0 = x->skip_cost[skip_ctx][0];
  const int s1 = x->skip_cost[skip_ctx][1];
  int64_t skip_rd = RDCOST(x->rdmult, s1, rd_stats->sse);
  this_rd = RDCOST(x->rdmult, rd_stats->rate + s0, rd_stats->dist);
  if (skip_rd < this_rd) {
    this_rd = skip_rd;
    rd_stats->rate = 0;
    rd_stats->dist = rd_stats->sse;
    rd_stats->skip = 1;
  }
  if (this_rd > ref_best_rd) is_cost_valid = 0;

  if (!is_cost_valid) {
    // reset cost value
    av1_invalid_rd_stats(rd_stats);
  }
  return is_cost_valid;
}

static int find_tx_size_rd_info(TXB_RD_RECORD *cur_record,
                                const uint32_t hash) {
  // Linear search through the circular buffer to find matching hash.
  for (int i = cur_record->index_start - 1; i >= 0; i--) {
    if (cur_record->hash_vals[i] == hash) return i;
  }
  for (int i = cur_record->num - 1; i >= cur_record->index_start; i--) {
    if (cur_record->hash_vals[i] == hash) return i;
  }
  int index;
  // If not found - add new RD info into the buffer and return its index
  if (cur_record->num < TX_SIZE_RD_RECORD_BUFFER_LEN) {
    index = (cur_record->index_start + cur_record->num) %
            TX_SIZE_RD_RECORD_BUFFER_LEN;
    cur_record->num++;
  } else {
    index = cur_record->index_start;
    cur_record->index_start =
        (cur_record->index_start + 1) % TX_SIZE_RD_RECORD_BUFFER_LEN;
  }

  cur_record->hash_vals[index] = hash;
  av1_zero(cur_record->tx_rd_info[index]);
  return index;
}

typedef struct {
  int leaf;
  int8_t children[4];
} RD_RECORD_IDX_NODE;

static const RD_RECORD_IDX_NODE rd_record_tree_8x8[] = {
  { 1, { 0 } },
};

static const RD_RECORD_IDX_NODE rd_record_tree_8x16[] = {
  { 0, { 1, 2, -1, -1 } },
  { 1, { 0, 0, 0, 0 } },
  { 1, { 0, 0, 0, 0 } },
};

static const RD_RECORD_IDX_NODE rd_record_tree_16x8[] = {
  { 0, { 1, 2, -1, -1 } },
  { 1, { 0 } },
  { 1, { 0 } },
};

static const RD_RECORD_IDX_NODE rd_record_tree_16x16[] = {
  { 0, { 1, 2, 3, 4 } }, { 1, { 0 } }, { 1, { 0 } }, { 1, { 0 } }, { 1, { 0 } },
};

static const RD_RECORD_IDX_NODE rd_record_tree_1_2[] = {
  { 0, { 1, 2, -1, -1 } },
  { 0, { 3, 4, 5, 6 } },
  { 0, { 7, 8, 9, 10 } },
};

static const RD_RECORD_IDX_NODE rd_record_tree_2_1[] = {
  { 0, { 1, 2, -1, -1 } },
  { 0, { 3, 4, 7, 8 } },
  { 0, { 5, 6, 9, 10 } },
};

static const RD_RECORD_IDX_NODE rd_record_tree_sqr[] = {
  { 0, { 1, 2, 3, 4 } },     { 0, { 5, 6, 9, 10 } },    { 0, { 7, 8, 11, 12 } },
  { 0, { 13, 14, 17, 18 } }, { 0, { 15, 16, 19, 20 } },
};

static const RD_RECORD_IDX_NODE rd_record_tree_64x128[] = {
  { 0, { 2, 3, 4, 5 } },     { 0, { 6, 7, 8, 9 } },
  { 0, { 10, 11, 14, 15 } }, { 0, { 12, 13, 16, 17 } },
  { 0, { 18, 19, 22, 23 } }, { 0, { 20, 21, 24, 25 } },
  { 0, { 26, 27, 30, 31 } }, { 0, { 28, 29, 32, 33 } },
  { 0, { 34, 35, 38, 39 } }, { 0, { 36, 37, 40, 41 } },
};

static const RD_RECORD_IDX_NODE rd_record_tree_128x64[] = {
  { 0, { 2, 3, 6, 7 } },     { 0, { 4, 5, 8, 9 } },
  { 0, { 10, 11, 18, 19 } }, { 0, { 12, 13, 20, 21 } },
  { 0, { 14, 15, 22, 23 } }, { 0, { 16, 17, 24, 25 } },
  { 0, { 26, 27, 34, 35 } }, { 0, { 28, 29, 36, 37 } },
  { 0, { 30, 31, 38, 39 } }, { 0, { 32, 33, 40, 41 } },
};

static const RD_RECORD_IDX_NODE rd_record_tree_128x128[] = {
  { 0, { 4, 5, 8, 9 } },     { 0, { 6, 7, 10, 11 } },
  { 0, { 12, 13, 16, 17 } }, { 0, { 14, 15, 18, 19 } },
  { 0, { 20, 21, 28, 29 } }, { 0, { 22, 23, 30, 31 } },
  { 0, { 24, 25, 32, 33 } }, { 0, { 26, 27, 34, 35 } },
  { 0, { 36, 37, 44, 45 } }, { 0, { 38, 39, 46, 47 } },
  { 0, { 40, 41, 48, 49 } }, { 0, { 42, 43, 50, 51 } },
  { 0, { 52, 53, 60, 61 } }, { 0, { 54, 55, 62, 63 } },
  { 0, { 56, 57, 64, 65 } }, { 0, { 58, 59, 66, 67 } },
  { 0, { 68, 69, 76, 77 } }, { 0, { 70, 71, 78, 79 } },
  { 0, { 72, 73, 80, 81 } }, { 0, { 74, 75, 82, 83 } },
};

static const RD_RECORD_IDX_NODE rd_record_tree_1_4[] = {
  { 0, { 1, -1, 2, -1 } },
  { 0, { 3, 4, -1, -1 } },
  { 0, { 5, 6, -1, -1 } },
};

static const RD_RECORD_IDX_NODE rd_record_tree_4_1[] = {
  { 0, { 1, 2, -1, -1 } },
  { 0, { 3, 4, -1, -1 } },
  { 0, { 5, 6, -1, -1 } },
};

static const RD_RECORD_IDX_NODE *rd_record_tree[BLOCK_SIZES_ALL] = {
  NULL,                    // BLOCK_4X4
  NULL,                    // BLOCK_4X8
  NULL,                    // BLOCK_8X4
  rd_record_tree_8x8,      // BLOCK_8X8
  rd_record_tree_8x16,     // BLOCK_8X16
  rd_record_tree_16x8,     // BLOCK_16X8
  rd_record_tree_16x16,    // BLOCK_16X16
  rd_record_tree_1_2,      // BLOCK_16X32
  rd_record_tree_2_1,      // BLOCK_32X16
  rd_record_tree_sqr,      // BLOCK_32X32
  rd_record_tree_1_2,      // BLOCK_32X64
  rd_record_tree_2_1,      // BLOCK_64X32
  rd_record_tree_sqr,      // BLOCK_64X64
  rd_record_tree_64x128,   // BLOCK_64X128
  rd_record_tree_128x64,   // BLOCK_128X64
  rd_record_tree_128x128,  // BLOCK_128X128
  NULL,                    // BLOCK_4X16
  NULL,                    // BLOCK_16X4
  rd_record_tree_1_4,      // BLOCK_8X32
  rd_record_tree_4_1,      // BLOCK_32X8
  rd_record_tree_1_4,      // BLOCK_16X64
  rd_record_tree_4_1,      // BLOCK_64X16
};

static const int rd_record_tree_size[BLOCK_SIZES_ALL] = {
  0,                                                            // BLOCK_4X4
  0,                                                            // BLOCK_4X8
  0,                                                            // BLOCK_8X4
  sizeof(rd_record_tree_8x8) / sizeof(RD_RECORD_IDX_NODE),      // BLOCK_8X8
  sizeof(rd_record_tree_8x16) / sizeof(RD_RECORD_IDX_NODE),     // BLOCK_8X16
  sizeof(rd_record_tree_16x8) / sizeof(RD_RECORD_IDX_NODE),     // BLOCK_16X8
  sizeof(rd_record_tree_16x16) / sizeof(RD_RECORD_IDX_NODE),    // BLOCK_16X16
  sizeof(rd_record_tree_1_2) / sizeof(RD_RECORD_IDX_NODE),      // BLOCK_16X32
  sizeof(rd_record_tree_2_1) / sizeof(RD_RECORD_IDX_NODE),      // BLOCK_32X16
  sizeof(rd_record_tree_sqr) / sizeof(RD_RECORD_IDX_NODE),      // BLOCK_32X32
  sizeof(rd_record_tree_1_2) / sizeof(RD_RECORD_IDX_NODE),      // BLOCK_32X64
  sizeof(rd_record_tree_2_1) / sizeof(RD_RECORD_IDX_NODE),      // BLOCK_64X32
  sizeof(rd_record_tree_sqr) / sizeof(RD_RECORD_IDX_NODE),      // BLOCK_64X64
  sizeof(rd_record_tree_64x128) / sizeof(RD_RECORD_IDX_NODE),   // BLOCK_64X128
  sizeof(rd_record_tree_128x64) / sizeof(RD_RECORD_IDX_NODE),   // BLOCK_128X64
  sizeof(rd_record_tree_128x128) / sizeof(RD_RECORD_IDX_NODE),  // BLOCK_128X128
  0,                                                            // BLOCK_4X16
  0,                                                            // BLOCK_16X4
  sizeof(rd_record_tree_1_4) / sizeof(RD_RECORD_IDX_NODE),      // BLOCK_8X32
  sizeof(rd_record_tree_4_1) / sizeof(RD_RECORD_IDX_NODE),      // BLOCK_32X8
  sizeof(rd_record_tree_1_4) / sizeof(RD_RECORD_IDX_NODE),      // BLOCK_16X64
  sizeof(rd_record_tree_4_1) / sizeof(RD_RECORD_IDX_NODE),      // BLOCK_64X16
};

static INLINE void init_rd_record_tree(TXB_RD_INFO_NODE *tree,
                                       BLOCK_SIZE bsize) {
  const RD_RECORD_IDX_NODE *rd_record = rd_record_tree[bsize];
  const int size = rd_record_tree_size[bsize];
  for (int i = 0; i < size; ++i) {
    if (rd_record[i].leaf) {
      av1_zero(tree[i].children);
    } else {
      for (int j = 0; j < 4; ++j) {
        const int8_t idx = rd_record[i].children[j];
        tree[i].children[j] = idx > 0 ? &tree[idx] : NULL;
      }
    }
  }
}

// Go through all TX blocks that could be used in TX size search, compute
// residual hash values for them and find matching RD info that stores previous
// RD search results for these TX blocks. The idea is to prevent repeated
// rate/distortion computations that happen because of the combination of
// partition and TX size search. The resulting RD info records are returned in
// the form of a quadtree for easier access in actual TX size search.
static int find_tx_size_rd_records(MACROBLOCK *x, BLOCK_SIZE bsize,
                                   TXB_RD_INFO_NODE *dst_rd_info) {
  TXB_RD_RECORD *rd_records_table[4] = { x->txb_rd_record_8X8,
                                         x->txb_rd_record_16X16,
                                         x->txb_rd_record_32X32,
                                         x->txb_rd_record_64X64 };
  const TX_SIZE max_square_tx_size = max_txsize_lookup[bsize];
  const int bw = block_size_wide[bsize];
  const int bh = block_size_high[bsize];

  // Hashing is performed only for square TX sizes larger than TX_4X4
  if (max_square_tx_size < TX_8X8) return 0;
  const int diff_stride = bw;
  const struct macroblock_plane *const p = &x->plane[0];
  const int16_t *diff = &p->src_diff[0];
  init_rd_record_tree(dst_rd_info, bsize);
  // Coordinates of the top-left corner of current block within the superblock
  // measured in pixels:
  const int mi_row = x->e_mbd.mi_row;
  const int mi_col = x->e_mbd.mi_col;
  const int mi_row_in_sb = (mi_row % MAX_MIB_SIZE) << MI_SIZE_LOG2;
  const int mi_col_in_sb = (mi_col % MAX_MIB_SIZE) << MI_SIZE_LOG2;
  int cur_rd_info_idx = 0;
  int cur_tx_depth = 0;
  TX_SIZE cur_tx_size = max_txsize_rect_lookup[bsize];
  while (cur_tx_depth <= MAX_VARTX_DEPTH) {
    const int cur_tx_bw = tx_size_wide[cur_tx_size];
    const int cur_tx_bh = tx_size_high[cur_tx_size];
    if (cur_tx_bw < 8 || cur_tx_bh < 8) break;
    const TX_SIZE next_tx_size = sub_tx_size_map[cur_tx_size];
    const int tx_size_idx = cur_tx_size - TX_8X8;
    for (int row = 0; row < bh; row += cur_tx_bh) {
      for (int col = 0; col < bw; col += cur_tx_bw) {
        if (cur_tx_bw != cur_tx_bh) {
          // Use dummy nodes for all rectangular transforms within the
          // TX size search tree.
          dst_rd_info[cur_rd_info_idx].rd_info_array = NULL;
        } else {
          // Get spatial location of this TX block within the superblock
          // (measured in cur_tx_bsize units).
          const int row_in_sb = (mi_row_in_sb + row) / cur_tx_bh;
          const int col_in_sb = (mi_col_in_sb + col) / cur_tx_bw;

          int16_t hash_data[MAX_SB_SQUARE];
          int16_t *cur_hash_row = hash_data;
          const int16_t *cur_diff_row = diff + row * diff_stride + col;
          for (int i = 0; i < cur_tx_bh; i++) {
            memcpy(cur_hash_row, cur_diff_row, sizeof(*hash_data) * cur_tx_bw);
            cur_hash_row += cur_tx_bw;
            cur_diff_row += diff_stride;
          }
          const int hash = av1_get_crc32c_value(&x->mb_rd_record.crc_calculator,
                                                (uint8_t *)hash_data,
                                                2 * cur_tx_bw * cur_tx_bh);
          // Find corresponding RD info based on the hash value.
          const int record_idx =
              row_in_sb * (MAX_MIB_SIZE >> (tx_size_idx + 1)) + col_in_sb;
          TXB_RD_RECORD *records = &rd_records_table[tx_size_idx][record_idx];
          int idx = find_tx_size_rd_info(records, hash);
          dst_rd_info[cur_rd_info_idx].rd_info_array =
              &records->tx_rd_info[idx];
        }
        ++cur_rd_info_idx;
      }
    }
    cur_tx_size = next_tx_size;
    ++cur_tx_depth;
  }
  return 1;
}

// Search for best transform size and type for luma inter blocks.
static AOM_INLINE void pick_tx_size_type_yrd(const AV1_COMP *cpi, MACROBLOCK *x,
                                             RD_STATS *rd_stats,
                                             BLOCK_SIZE bsize,
                                             int64_t ref_best_rd) {
  const AV1_COMMON *cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  assert(is_inter_block(xd->mi[0]));

  av1_invalid_rd_stats(rd_stats);

  if (cpi->sf.tx_sf.model_based_prune_tx_search_level &&
      ref_best_rd != INT64_MAX) {
    int model_rate;
    int64_t model_dist;
    int model_skip;
    model_rd_sb_fn[MODELRD_TYPE_TX_SEARCH_PRUNE](
        cpi, bsize, x, xd, 0, 0, &model_rate, &model_dist, &model_skip, NULL,
        NULL, NULL, NULL);
    const int64_t model_rd = RDCOST(x->rdmult, model_rate, model_dist);
    // If the modeled rd is a lot worse than the best so far, breakout.
    // TODO(debargha, urvang): Improve the model and make the check below
    // tighter.
    assert(cpi->sf.tx_sf.model_based_prune_tx_search_level >= 0 &&
           cpi->sf.tx_sf.model_based_prune_tx_search_level <= 2);
    static const int prune_factor_by8[] = { 3, 5 };
    if (!model_skip &&
        ((model_rd *
          prune_factor_by8[cpi->sf.tx_sf.model_based_prune_tx_search_level -
                           1]) >>
         3) > ref_best_rd)
      return;
  }

  uint32_t hash = 0;
  int32_t match_index = -1;
  MB_RD_RECORD *mb_rd_record = NULL;
  const int mi_row = x->e_mbd.mi_row;
  const int mi_col = x->e_mbd.mi_col;
  const int within_border =
      mi_row >= xd->tile.mi_row_start &&
      (mi_row + mi_size_high[bsize] < xd->tile.mi_row_end) &&
      mi_col >= xd->tile.mi_col_start &&
      (mi_col + mi_size_wide[bsize] < xd->tile.mi_col_end);
  const int is_mb_rd_hash_enabled =
      (within_border && cpi->sf.rd_sf.use_mb_rd_hash);
  const int n4 = bsize_to_num_blk(bsize);
  if (is_mb_rd_hash_enabled) {
    hash = get_block_residue_hash(x, bsize);
    mb_rd_record = &x->mb_rd_record;
    match_index = find_mb_rd_info(mb_rd_record, ref_best_rd, hash);
    if (match_index != -1) {
      MB_RD_INFO *tx_rd_info = &mb_rd_record->tx_rd_info[match_index];
      fetch_tx_rd_info(n4, tx_rd_info, rd_stats, x);
      return;
    }
  }

  // If we predict that skip is the optimal RD decision - set the respective
  // context and terminate early.
  int64_t dist;
  if (x->predict_skip_level &&
      predict_skip_flag(x, bsize, &dist, cm->reduced_tx_set_used)) {
    set_skip_flag(x, rd_stats, bsize, dist);
    // Save the RD search results into tx_rd_record.
    if (is_mb_rd_hash_enabled)
      save_tx_rd_info(n4, hash, x, rd_stats, mb_rd_record);
    return;
  }
#if CONFIG_SPEED_STATS
  ++x->tx_search_count;
#endif  // CONFIG_SPEED_STATS

  // Precompute residual hashes and find existing or add new RD records to
  // store and reuse rate and distortion values to speed up TX size search.
  TXB_RD_INFO_NODE matched_rd_info[4 + 16 + 64];
  int found_rd_info = 0;
  if (ref_best_rd != INT64_MAX && within_border &&
      cpi->sf.tx_sf.use_inter_txb_hash) {
    found_rd_info = find_tx_size_rd_records(x, bsize, matched_rd_info);
  }

  int found = 0;
  RD_STATS this_rd_stats;
  av1_init_rd_stats(&this_rd_stats);
  const int64_t rd =
      select_tx_size_and_type(cpi, x, &this_rd_stats, bsize, ref_best_rd,
                              found_rd_info ? matched_rd_info : NULL);

  if (rd < INT64_MAX) {
    *rd_stats = this_rd_stats;
    found = 1;
  }

  // We should always find at least one candidate unless ref_best_rd is less
  // than INT64_MAX (in which case, all the calls to select_tx_size_fix_type
  // might have failed to find something better)
  assert(IMPLIES(!found, ref_best_rd != INT64_MAX));
  if (!found) return;

  // Save the RD search results into tx_rd_record.
  if (is_mb_rd_hash_enabled) {
    assert(mb_rd_record != NULL);
    save_tx_rd_info(n4, hash, x, rd_stats, mb_rd_record);
  }
}

static AOM_INLINE void rd_pick_palette_intra_sbuv(
    const AV1_COMP *const cpi, MACROBLOCK *x, int dc_mode_cost,
    uint8_t *best_palette_color_map, MB_MODE_INFO *const best_mbmi,
    int64_t *best_rd, int *rate, int *rate_tokenonly, int64_t *distortion,
    int *skippable) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  assert(!is_inter_block(mbmi));
  assert(
      av1_allow_palette(cpi->common.allow_screen_content_tools, mbmi->sb_type));
  PALETTE_MODE_INFO *const pmi = &mbmi->palette_mode_info;
  const BLOCK_SIZE bsize = mbmi->sb_type;
  const SequenceHeader *const seq_params = &cpi->common.seq_params;
  int this_rate;
  int64_t this_rd;
  int colors_u, colors_v, colors;
  const int src_stride = x->plane[1].src.stride;
  const uint8_t *const src_u = x->plane[1].src.buf;
  const uint8_t *const src_v = x->plane[2].src.buf;
  uint8_t *const color_map = xd->plane[1].color_index_map;
  RD_STATS tokenonly_rd_stats;
  int plane_block_width, plane_block_height, rows, cols;
  av1_get_block_dimensions(bsize, 1, xd, &plane_block_width,
                           &plane_block_height, &rows, &cols);

  mbmi->uv_mode = UV_DC_PRED;

  int count_buf[1 << 12];  // Maximum (1 << 12) color levels.
  if (seq_params->use_highbitdepth) {
    colors_u = av1_count_colors_highbd(src_u, src_stride, rows, cols,
                                       seq_params->bit_depth, count_buf);
    colors_v = av1_count_colors_highbd(src_v, src_stride, rows, cols,
                                       seq_params->bit_depth, count_buf);
  } else {
    colors_u = av1_count_colors(src_u, src_stride, rows, cols, count_buf);
    colors_v = av1_count_colors(src_v, src_stride, rows, cols, count_buf);
  }

  uint16_t color_cache[2 * PALETTE_MAX_SIZE];
  const int n_cache = av1_get_palette_cache(xd, 1, color_cache);

  colors = colors_u > colors_v ? colors_u : colors_v;
  if (colors > 1 && colors <= 64) {
    int r, c, n, i, j;
    const int max_itr = 50;
    int lb_u, ub_u, val_u;
    int lb_v, ub_v, val_v;
    int *const data = x->palette_buffer->kmeans_data_buf;
    int centroids[2 * PALETTE_MAX_SIZE];

    uint16_t *src_u16 = CONVERT_TO_SHORTPTR(src_u);
    uint16_t *src_v16 = CONVERT_TO_SHORTPTR(src_v);
    if (seq_params->use_highbitdepth) {
      lb_u = src_u16[0];
      ub_u = src_u16[0];
      lb_v = src_v16[0];
      ub_v = src_v16[0];
    } else {
      lb_u = src_u[0];
      ub_u = src_u[0];
      lb_v = src_v[0];
      ub_v = src_v[0];
    }

    for (r = 0; r < rows; ++r) {
      for (c = 0; c < cols; ++c) {
        if (seq_params->use_highbitdepth) {
          val_u = src_u16[r * src_stride + c];
          val_v = src_v16[r * src_stride + c];
          data[(r * cols + c) * 2] = val_u;
          data[(r * cols + c) * 2 + 1] = val_v;
        } else {
          val_u = src_u[r * src_stride + c];
          val_v = src_v[r * src_stride + c];
          data[(r * cols + c) * 2] = val_u;
          data[(r * cols + c) * 2 + 1] = val_v;
        }
        if (val_u < lb_u)
          lb_u = val_u;
        else if (val_u > ub_u)
          ub_u = val_u;
        if (val_v < lb_v)
          lb_v = val_v;
        else if (val_v > ub_v)
          ub_v = val_v;
      }
    }

    for (n = colors > PALETTE_MAX_SIZE ? PALETTE_MAX_SIZE : colors; n >= 2;
         --n) {
      for (i = 0; i < n; ++i) {
        centroids[i * 2] = lb_u + (2 * i + 1) * (ub_u - lb_u) / n / 2;
        centroids[i * 2 + 1] = lb_v + (2 * i + 1) * (ub_v - lb_v) / n / 2;
      }
      av1_k_means(data, centroids, color_map, rows * cols, n, 2, max_itr);
      optimize_palette_colors(color_cache, n_cache, n, 2, centroids);
      // Sort the U channel colors in ascending order.
      for (i = 0; i < 2 * (n - 1); i += 2) {
        int min_idx = i;
        int min_val = centroids[i];
        for (j = i + 2; j < 2 * n; j += 2)
          if (centroids[j] < min_val) min_val = centroids[j], min_idx = j;
        if (min_idx != i) {
          int temp_u = centroids[i], temp_v = centroids[i + 1];
          centroids[i] = centroids[min_idx];
          centroids[i + 1] = centroids[min_idx + 1];
          centroids[min_idx] = temp_u, centroids[min_idx + 1] = temp_v;
        }
      }
      av1_calc_indices(data, centroids, color_map, rows * cols, n, 2);
      extend_palette_color_map(color_map, cols, rows, plane_block_width,
                               plane_block_height);
      pmi->palette_size[1] = n;
      for (i = 1; i < 3; ++i) {
        for (j = 0; j < n; ++j) {
          if (seq_params->use_highbitdepth)
            pmi->palette_colors[i * PALETTE_MAX_SIZE + j] = clip_pixel_highbd(
                (int)centroids[j * 2 + i - 1], seq_params->bit_depth);
          else
            pmi->palette_colors[i * PALETTE_MAX_SIZE + j] =
                clip_pixel((int)centroids[j * 2 + i - 1]);
        }
      }

      super_block_uvrd(cpi, x, &tokenonly_rd_stats, bsize, *best_rd);
      if (tokenonly_rd_stats.rate == INT_MAX) continue;
      this_rate = tokenonly_rd_stats.rate +
                  intra_mode_info_cost_uv(cpi, x, mbmi, bsize, dc_mode_cost);
      this_rd = RDCOST(x->rdmult, this_rate, tokenonly_rd_stats.dist);
      if (this_rd < *best_rd) {
        *best_rd = this_rd;
        *best_mbmi = *mbmi;
        memcpy(best_palette_color_map, color_map,
               plane_block_width * plane_block_height *
                   sizeof(best_palette_color_map[0]));
        *rate = this_rate;
        *distortion = tokenonly_rd_stats.dist;
        *rate_tokenonly = tokenonly_rd_stats.rate;
        *skippable = tokenonly_rd_stats.skip;
      }
    }
  }
  if (best_mbmi->palette_mode_info.palette_size[1] > 0) {
    memcpy(color_map, best_palette_color_map,
           plane_block_width * plane_block_height *
               sizeof(best_palette_color_map[0]));
  }
}

// Run RD calculation with given chroma intra prediction angle., and return
// the RD cost. Update the best mode info. if the RD cost is the best so far.
static int64_t pick_intra_angle_routine_sbuv(
    const AV1_COMP *const cpi, MACROBLOCK *x, BLOCK_SIZE bsize,
    int rate_overhead, int64_t best_rd_in, int *rate, RD_STATS *rd_stats,
    int *best_angle_delta, int64_t *best_rd) {
  MB_MODE_INFO *mbmi = x->e_mbd.mi[0];
  assert(!is_inter_block(mbmi));
  int this_rate;
  int64_t this_rd;
  RD_STATS tokenonly_rd_stats;

  if (!super_block_uvrd(cpi, x, &tokenonly_rd_stats, bsize, best_rd_in))
    return INT64_MAX;
  this_rate = tokenonly_rd_stats.rate +
              intra_mode_info_cost_uv(cpi, x, mbmi, bsize, rate_overhead);
  this_rd = RDCOST(x->rdmult, this_rate, tokenonly_rd_stats.dist);
  if (this_rd < *best_rd) {
    *best_rd = this_rd;
    *best_angle_delta = mbmi->angle_delta[PLANE_TYPE_UV];
    *rate = this_rate;
    rd_stats->rate = tokenonly_rd_stats.rate;
    rd_stats->dist = tokenonly_rd_stats.dist;
    rd_stats->skip = tokenonly_rd_stats.skip;
  }
  return this_rd;
}

// With given chroma directional intra prediction mode, pick the best angle
// delta. Return true if a RD cost that is smaller than the input one is found.
static int rd_pick_intra_angle_sbuv(const AV1_COMP *const cpi, MACROBLOCK *x,
                                    BLOCK_SIZE bsize, int rate_overhead,
                                    int64_t best_rd, int *rate,
                                    RD_STATS *rd_stats) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  assert(!is_inter_block(mbmi));
  int i, angle_delta, best_angle_delta = 0;
  int64_t this_rd, best_rd_in, rd_cost[2 * (MAX_ANGLE_DELTA + 2)];

  rd_stats->rate = INT_MAX;
  rd_stats->skip = 0;
  rd_stats->dist = INT64_MAX;
  for (i = 0; i < 2 * (MAX_ANGLE_DELTA + 2); ++i) rd_cost[i] = INT64_MAX;

  for (angle_delta = 0; angle_delta <= MAX_ANGLE_DELTA; angle_delta += 2) {
    for (i = 0; i < 2; ++i) {
      best_rd_in = (best_rd == INT64_MAX)
                       ? INT64_MAX
                       : (best_rd + (best_rd >> ((angle_delta == 0) ? 3 : 5)));
      mbmi->angle_delta[PLANE_TYPE_UV] = (1 - 2 * i) * angle_delta;
      this_rd = pick_intra_angle_routine_sbuv(cpi, x, bsize, rate_overhead,
                                              best_rd_in, rate, rd_stats,
                                              &best_angle_delta, &best_rd);
      rd_cost[2 * angle_delta + i] = this_rd;
      if (angle_delta == 0) {
        if (this_rd == INT64_MAX) return 0;
        rd_cost[1] = this_rd;
        break;
      }
    }
  }

  assert(best_rd != INT64_MAX);
  for (angle_delta = 1; angle_delta <= MAX_ANGLE_DELTA; angle_delta += 2) {
    int64_t rd_thresh;
    for (i = 0; i < 2; ++i) {
      int skip_search = 0;
      rd_thresh = best_rd + (best_rd >> 5);
      if (rd_cost[2 * (angle_delta + 1) + i] > rd_thresh &&
          rd_cost[2 * (angle_delta - 1) + i] > rd_thresh)
        skip_search = 1;
      if (!skip_search) {
        mbmi->angle_delta[PLANE_TYPE_UV] = (1 - 2 * i) * angle_delta;
        pick_intra_angle_routine_sbuv(cpi, x, bsize, rate_overhead, best_rd,
                                      rate, rd_stats, &best_angle_delta,
                                      &best_rd);
      }
    }
  }

  mbmi->angle_delta[PLANE_TYPE_UV] = best_angle_delta;
  return rd_stats->rate != INT_MAX;
}

#define PLANE_SIGN_TO_JOINT_SIGN(plane, a, b) \
  (plane == CFL_PRED_U ? a * CFL_SIGNS + b - 1 : b * CFL_SIGNS + a - 1)
static int cfl_rd_pick_alpha(MACROBLOCK *const x, const AV1_COMP *const cpi,
                             TX_SIZE tx_size, int64_t best_rd) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const MACROBLOCKD_PLANE *pd = &xd->plane[AOM_PLANE_U];
  const BLOCK_SIZE plane_bsize =
      get_plane_block_size(mbmi->sb_type, pd->subsampling_x, pd->subsampling_y);

  assert(is_cfl_allowed(xd) && cpi->oxcf.enable_cfl_intra);
  assert(plane_bsize < BLOCK_SIZES_ALL);
  if (!xd->lossless[mbmi->segment_id]) {
    assert(block_size_wide[plane_bsize] == tx_size_wide[tx_size]);
    assert(block_size_high[plane_bsize] == tx_size_high[tx_size]);
  }

  xd->cfl.use_dc_pred_cache = 1;
  const int64_t mode_rd =
      RDCOST(x->rdmult,
             x->intra_uv_mode_cost[CFL_ALLOWED][mbmi->mode][UV_CFL_PRED], 0);
  int64_t best_rd_uv[CFL_JOINT_SIGNS][CFL_PRED_PLANES];
  int best_c[CFL_JOINT_SIGNS][CFL_PRED_PLANES];
#if CONFIG_DEBUG
  int best_rate_uv[CFL_JOINT_SIGNS][CFL_PRED_PLANES];
#endif  // CONFIG_DEBUG

  for (int plane = 0; plane < CFL_PRED_PLANES; plane++) {
    RD_STATS rd_stats;
    av1_init_rd_stats(&rd_stats);
    for (int joint_sign = 0; joint_sign < CFL_JOINT_SIGNS; joint_sign++) {
      best_rd_uv[joint_sign][plane] = INT64_MAX;
      best_c[joint_sign][plane] = 0;
    }
    // Collect RD stats for an alpha value of zero in this plane.
    // Skip i == CFL_SIGN_ZERO as (0, 0) is invalid.
    for (int i = CFL_SIGN_NEG; i < CFL_SIGNS; i++) {
      const int8_t joint_sign =
          PLANE_SIGN_TO_JOINT_SIGN(plane, CFL_SIGN_ZERO, i);
      if (i == CFL_SIGN_NEG) {
        mbmi->cfl_alpha_idx = 0;
        mbmi->cfl_alpha_signs = joint_sign;
        txfm_rd_in_plane(x, cpi, &rd_stats, best_rd, 0, plane + 1, plane_bsize,
                         tx_size, cpi->sf.rd_sf.use_fast_coef_costing,
                         FTXS_NONE, 0);
        if (rd_stats.rate == INT_MAX) break;
      }
      const int alpha_rate = x->cfl_cost[joint_sign][plane][0];
      best_rd_uv[joint_sign][plane] =
          RDCOST(x->rdmult, rd_stats.rate + alpha_rate, rd_stats.dist);
#if CONFIG_DEBUG
      best_rate_uv[joint_sign][plane] = rd_stats.rate;
#endif  // CONFIG_DEBUG
    }
  }

  int8_t best_joint_sign = -1;

  for (int plane = 0; plane < CFL_PRED_PLANES; plane++) {
    for (int pn_sign = CFL_SIGN_NEG; pn_sign < CFL_SIGNS; pn_sign++) {
      int progress = 0;
      for (int c = 0; c < CFL_ALPHABET_SIZE; c++) {
        int flag = 0;
        RD_STATS rd_stats;
        if (c > 2 && progress < c) break;
        av1_init_rd_stats(&rd_stats);
        for (int i = 0; i < CFL_SIGNS; i++) {
          const int8_t joint_sign = PLANE_SIGN_TO_JOINT_SIGN(plane, pn_sign, i);
          if (i == 0) {
            mbmi->cfl_alpha_idx = (c << CFL_ALPHABET_SIZE_LOG2) + c;
            mbmi->cfl_alpha_signs = joint_sign;
            txfm_rd_in_plane(x, cpi, &rd_stats, best_rd, 0, plane + 1,
                             plane_bsize, tx_size,
                             cpi->sf.rd_sf.use_fast_coef_costing, FTXS_NONE, 0);
            if (rd_stats.rate == INT_MAX) break;
          }
          const int alpha_rate = x->cfl_cost[joint_sign][plane][c];
          int64_t this_rd =
              RDCOST(x->rdmult, rd_stats.rate + alpha_rate, rd_stats.dist);
          if (this_rd >= best_rd_uv[joint_sign][plane]) continue;
          best_rd_uv[joint_sign][plane] = this_rd;
          best_c[joint_sign][plane] = c;
#if CONFIG_DEBUG
          best_rate_uv[joint_sign][plane] = rd_stats.rate;
#endif  // CONFIG_DEBUG
          flag = 2;
          if (best_rd_uv[joint_sign][!plane] == INT64_MAX) continue;
          this_rd += mode_rd + best_rd_uv[joint_sign][!plane];
          if (this_rd >= best_rd) continue;
          best_rd = this_rd;
          best_joint_sign = joint_sign;
        }
        progress += flag;
      }
    }
  }

  int best_rate_overhead = INT_MAX;
  uint8_t ind = 0;
  if (best_joint_sign >= 0) {
    const int u = best_c[best_joint_sign][CFL_PRED_U];
    const int v = best_c[best_joint_sign][CFL_PRED_V];
    ind = (u << CFL_ALPHABET_SIZE_LOG2) + v;
    best_rate_overhead = x->cfl_cost[best_joint_sign][CFL_PRED_U][u] +
                         x->cfl_cost[best_joint_sign][CFL_PRED_V][v];
#if CONFIG_DEBUG
    xd->cfl.rate = x->intra_uv_mode_cost[CFL_ALLOWED][mbmi->mode][UV_CFL_PRED] +
                   best_rate_overhead +
                   best_rate_uv[best_joint_sign][CFL_PRED_U] +
                   best_rate_uv[best_joint_sign][CFL_PRED_V];
#endif  // CONFIG_DEBUG
  } else {
    best_joint_sign = 0;
  }

  mbmi->cfl_alpha_idx = ind;
  mbmi->cfl_alpha_signs = best_joint_sign;
  xd->cfl.use_dc_pred_cache = 0;
  xd->cfl.dc_pred_is_cached[0] = 0;
  xd->cfl.dc_pred_is_cached[1] = 0;
  return best_rate_overhead;
}

static AOM_INLINE void init_sbuv_mode(MB_MODE_INFO *const mbmi) {
  mbmi->uv_mode = UV_DC_PRED;
  mbmi->palette_mode_info.palette_size[1] = 0;
}

static int64_t rd_pick_intra_sbuv_mode(const AV1_COMP *const cpi, MACROBLOCK *x,
                                       int *rate, int *rate_tokenonly,
                                       int64_t *distortion, int *skippable,
                                       BLOCK_SIZE bsize, TX_SIZE max_tx_size) {
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  assert(!is_inter_block(mbmi));
  MB_MODE_INFO best_mbmi = *mbmi;
  int64_t best_rd = INT64_MAX, this_rd;

  for (int mode_idx = 0; mode_idx < UV_INTRA_MODES; ++mode_idx) {
    int this_rate;
    RD_STATS tokenonly_rd_stats;
    UV_PREDICTION_MODE mode = uv_rd_search_mode_order[mode_idx];
    const int is_directional_mode = av1_is_directional_mode(get_uv_mode(mode));
    if (!(cpi->sf.intra_sf.intra_uv_mode_mask[txsize_sqr_up_map[max_tx_size]] &
          (1 << mode)))
      continue;
    if (!cpi->oxcf.enable_smooth_intra && mode >= UV_SMOOTH_PRED &&
        mode <= UV_SMOOTH_H_PRED)
      continue;

    if (!cpi->oxcf.enable_paeth_intra && mode == UV_PAETH_PRED) continue;

    mbmi->uv_mode = mode;
    int cfl_alpha_rate = 0;
    if (mode == UV_CFL_PRED) {
      if (!is_cfl_allowed(xd) || !cpi->oxcf.enable_cfl_intra) continue;
      assert(!is_directional_mode);
      const TX_SIZE uv_tx_size = av1_get_tx_size(AOM_PLANE_U, xd);
      cfl_alpha_rate = cfl_rd_pick_alpha(x, cpi, uv_tx_size, best_rd);
      if (cfl_alpha_rate == INT_MAX) continue;
    }
    mbmi->angle_delta[PLANE_TYPE_UV] = 0;
    if (is_directional_mode && av1_use_angle_delta(mbmi->sb_type) &&
        cpi->oxcf.enable_angle_delta) {
      const int rate_overhead =
          x->intra_uv_mode_cost[is_cfl_allowed(xd)][mbmi->mode][mode];
      if (!rd_pick_intra_angle_sbuv(cpi, x, bsize, rate_overhead, best_rd,
                                    &this_rate, &tokenonly_rd_stats))
        continue;
    } else {
      if (!super_block_uvrd(cpi, x, &tokenonly_rd_stats, bsize, best_rd)) {
        continue;
      }
    }
    const int mode_cost =
        x->intra_uv_mode_cost[is_cfl_allowed(xd)][mbmi->mode][mode] +
        cfl_alpha_rate;
    this_rate = tokenonly_rd_stats.rate +
                intra_mode_info_cost_uv(cpi, x, mbmi, bsize, mode_cost);
    if (mode == UV_CFL_PRED) {
      assert(is_cfl_allowed(xd) && cpi->oxcf.enable_cfl_intra);
#if CONFIG_DEBUG
      if (!xd->lossless[mbmi->segment_id])
        assert(xd->cfl.rate == tokenonly_rd_stats.rate + mode_cost);
#endif  // CONFIG_DEBUG
    }
    this_rd = RDCOST(x->rdmult, this_rate, tokenonly_rd_stats.dist);

    if (this_rd < best_rd) {
      best_mbmi = *mbmi;
      best_rd = this_rd;
      *rate = this_rate;
      *rate_tokenonly = tokenonly_rd_stats.rate;
      *distortion = tokenonly_rd_stats.dist;
      *skippable = tokenonly_rd_stats.skip;
    }
  }

  const int try_palette =
      cpi->oxcf.enable_palette &&
      av1_allow_palette(cpi->common.allow_screen_content_tools, mbmi->sb_type);
  if (try_palette) {
    uint8_t *best_palette_color_map = x->palette_buffer->best_palette_color_map;
    rd_pick_palette_intra_sbuv(
        cpi, x,
        x->intra_uv_mode_cost[is_cfl_allowed(xd)][mbmi->mode][UV_DC_PRED],
        best_palette_color_map, &best_mbmi, &best_rd, rate, rate_tokenonly,
        distortion, skippable);
  }

  *mbmi = best_mbmi;
  // Make sure we actually chose a mode
  assert(best_rd < INT64_MAX);
  return best_rd;
}

static AOM_INLINE void choose_intra_uv_mode(
    const AV1_COMP *const cpi, MACROBLOCK *const x, BLOCK_SIZE bsize,
    TX_SIZE max_tx_size, int *rate_uv, int *rate_uv_tokenonly, int64_t *dist_uv,
    int *skip_uv, UV_PREDICTION_MODE *mode_uv) {
  const AV1_COMMON *const cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  // Use an estimated rd for uv_intra based on DC_PRED if the
  // appropriate speed flag is set.
  init_sbuv_mode(mbmi);
  if (x->skip_chroma_rd) {
    *rate_uv = 0;
    *rate_uv_tokenonly = 0;
    *dist_uv = 0;
    *skip_uv = 1;
    *mode_uv = UV_DC_PRED;
    return;
  }

  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  xd->cfl.is_chroma_reference =
      is_chroma_reference(mi_row, mi_col, bsize, cm->seq_params.subsampling_x,
                          cm->seq_params.subsampling_y);
  // Only store reconstructed luma when there's chroma RDO. When there's no
  // chroma RDO, the reconstructed luma will be stored in encode_superblock().
  xd->cfl.store_y = store_cfl_required_rdo(cm, x);
  if (xd->cfl.store_y) {
    // Restore reconstructed luma values.
    av1_encode_intra_block_plane(cpi, x, mbmi->sb_type, AOM_PLANE_Y,
                                 cpi->optimize_seg_arr[mbmi->segment_id]);
    xd->cfl.store_y = 0;
  }
  rd_pick_intra_sbuv_mode(cpi, x, rate_uv, rate_uv_tokenonly, dist_uv, skip_uv,
                          bsize, max_tx_size);
  *mode_uv = mbmi->uv_mode;
}

static int cost_mv_ref(const MACROBLOCK *const x, PREDICTION_MODE mode,
                       int16_t mode_context) {
  if (is_inter_compound_mode(mode)) {
    return x
        ->inter_compound_mode_cost[mode_context][INTER_COMPOUND_OFFSET(mode)];
  }

  int mode_cost = 0;
  int16_t mode_ctx = mode_context & NEWMV_CTX_MASK;

  assert(is_inter_mode(mode));

  if (mode == NEWMV) {
    mode_cost = x->newmv_mode_cost[mode_ctx][0];
    return mode_cost;
  } else {
    mode_cost = x->newmv_mode_cost[mode_ctx][1];
    mode_ctx = (mode_context >> GLOBALMV_OFFSET) & GLOBALMV_CTX_MASK;

    if (mode == GLOBALMV) {
      mode_cost += x->zeromv_mode_cost[mode_ctx][0];
      return mode_cost;
    } else {
      mode_cost += x->zeromv_mode_cost[mode_ctx][1];
      mode_ctx = (mode_context >> REFMV_OFFSET) & REFMV_CTX_MASK;
      mode_cost += x->refmv_mode_cost[mode_ctx][mode != NEARESTMV];
      return mode_cost;
    }
  }
}

static INLINE int get_interinter_compound_mask_rate(
    const MACROBLOCK *const x, const MB_MODE_INFO *const mbmi) {
  const COMPOUND_TYPE compound_type = mbmi->interinter_comp.type;
  // This function will be called only for COMPOUND_WEDGE and COMPOUND_DIFFWTD
  if (compound_type == COMPOUND_WEDGE) {
    return av1_is_wedge_used(mbmi->sb_type)
               ? av1_cost_literal(1) +
                     x->wedge_idx_cost[mbmi->sb_type]
                                      [mbmi->interinter_comp.wedge_index]
               : 0;
  } else {
    assert(compound_type == COMPOUND_DIFFWTD);
    return av1_cost_literal(1);
  }
}

static INLINE int mv_check_bounds(const MvLimits *mv_limits, const MV *mv) {
  return (mv->row >> 3) < mv_limits->row_min ||
         (mv->row >> 3) > mv_limits->row_max ||
         (mv->col >> 3) < mv_limits->col_min ||
         (mv->col >> 3) > mv_limits->col_max;
}

static INLINE PREDICTION_MODE get_single_mode(PREDICTION_MODE this_mode,
                                              int ref_idx, int is_comp_pred) {
  PREDICTION_MODE single_mode;
  if (is_comp_pred) {
    single_mode =
        ref_idx ? compound_ref1_mode(this_mode) : compound_ref0_mode(this_mode);
  } else {
    single_mode = this_mode;
  }
  return single_mode;
}

static AOM_INLINE void joint_motion_search(const AV1_COMP *cpi, MACROBLOCK *x,
                                           BLOCK_SIZE bsize, int_mv *cur_mv,
                                           const uint8_t *mask, int mask_stride,
                                           int *rate_mv) {
  const AV1_COMMON *const cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  const int pw = block_size_wide[bsize];
  const int ph = block_size_high[bsize];
  const int plane = 0;
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  // This function should only ever be called for compound modes
  assert(has_second_ref(mbmi));
  const int_mv init_mv[2] = { cur_mv[0], cur_mv[1] };
  const int refs[2] = { mbmi->ref_frame[0], mbmi->ref_frame[1] };
  int_mv ref_mv[2];
  int ite, ref;

  // Get the prediction block from the 'other' reference frame.
  const int_interpfilters interp_filters =
      av1_broadcast_interp_filter(EIGHTTAP_REGULAR);

  InterPredParams inter_pred_params;
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;

  // Do joint motion search in compound mode to get more accurate mv.
  struct buf_2d backup_yv12[2][MAX_MB_PLANE];
  int last_besterr[2] = { INT_MAX, INT_MAX };
  const YV12_BUFFER_CONFIG *const scaled_ref_frame[2] = {
    av1_get_scaled_ref_frame(cpi, refs[0]),
    av1_get_scaled_ref_frame(cpi, refs[1])
  };

  // Prediction buffer from second frame.
  DECLARE_ALIGNED(16, uint8_t, second_pred16[MAX_SB_SQUARE * sizeof(uint16_t)]);
  uint8_t *second_pred = get_buf_by_bd(xd, second_pred16);

  MV *const best_mv = &x->best_mv.as_mv;
  const int search_range = SEARCH_RANGE_8P;
  const int sadpb = x->sadperbit16;
  // Allow joint search multiple times iteratively for each reference frame
  // and break out of the search loop if it couldn't find a better mv.
  for (ite = 0; ite < 4; ite++) {
    struct buf_2d ref_yv12[2];
    int bestsme = INT_MAX;
    MvLimits tmp_mv_limits = x->mv_limits;
    int id = ite % 2;  // Even iterations search in the first reference frame,
                       // odd iterations search in the second. The predictor
                       // found for the 'other' reference frame is factored in.
    if (ite >= 2 && cur_mv[!id].as_int == init_mv[!id].as_int) {
      if (cur_mv[id].as_int == init_mv[id].as_int) {
        break;
      } else {
        int_mv cur_int_mv, init_int_mv;
        cur_int_mv.as_mv.col = cur_mv[id].as_mv.col >> 3;
        cur_int_mv.as_mv.row = cur_mv[id].as_mv.row >> 3;
        init_int_mv.as_mv.row = init_mv[id].as_mv.row >> 3;
        init_int_mv.as_mv.col = init_mv[id].as_mv.col >> 3;
        if (cur_int_mv.as_int == init_int_mv.as_int) {
          break;
        }
      }
    }
    for (ref = 0; ref < 2; ++ref) {
      ref_mv[ref] = av1_get_ref_mv(x, ref);
      // Swap out the reference frame for a version that's been scaled to
      // match the resolution of the current frame, allowing the existing
      // motion search code to be used without additional modifications.
      if (scaled_ref_frame[ref]) {
        int i;
        for (i = 0; i < num_planes; i++)
          backup_yv12[ref][i] = xd->plane[i].pre[ref];
        av1_setup_pre_planes(xd, ref, scaled_ref_frame[ref], mi_row, mi_col,
                             NULL, num_planes);
      }
    }

    assert(IMPLIES(scaled_ref_frame[0] != NULL,
                   cm->width == scaled_ref_frame[0]->y_crop_width &&
                       cm->height == scaled_ref_frame[0]->y_crop_height));
    assert(IMPLIES(scaled_ref_frame[1] != NULL,
                   cm->width == scaled_ref_frame[1]->y_crop_width &&
                       cm->height == scaled_ref_frame[1]->y_crop_height));

    // Initialize based on (possibly scaled) prediction buffers.
    ref_yv12[0] = xd->plane[plane].pre[0];
    ref_yv12[1] = xd->plane[plane].pre[1];

    av1_init_inter_params(&inter_pred_params, pw, ph, mi_row * MI_SIZE,
                          mi_col * MI_SIZE, 0, 0, xd->bd, is_cur_buf_hbd(xd), 0,
                          &cm->sf_identity, &ref_yv12[!id], interp_filters);
    inter_pred_params.conv_params = get_conv_params(0, 0, xd->bd);

    // Since we have scaled the reference frames to match the size of the
    // current frame we must use a unit scaling factor during mode selection.
    av1_build_inter_predictor(second_pred, pw, &cur_mv[!id].as_mv,
                              &inter_pred_params);

    const int order_idx = id != 0;
    av1_dist_wtd_comp_weight_assign(
        cm, mbmi, order_idx, &xd->jcp_param.fwd_offset,
        &xd->jcp_param.bck_offset, &xd->jcp_param.use_dist_wtd_comp_avg, 1);

    // Do full-pixel compound motion search on the current reference frame.
    if (id) xd->plane[plane].pre[0] = ref_yv12[id];
    av1_set_mv_search_range(&x->mv_limits, &ref_mv[id].as_mv);

    // Use the mv result from the single mode as mv predictor.
    *best_mv = cur_mv[id].as_mv;

    best_mv->col >>= 3;
    best_mv->row >>= 3;

    // Small-range full-pixel motion search.
    bestsme = av1_refining_search_8p_c(
        x, sadpb, search_range, &cpi->fn_ptr[bsize], mask, mask_stride, id,
        &ref_mv[id].as_mv, second_pred, &x->plane[0].src, &ref_yv12[id]);
    if (bestsme < INT_MAX) {
      if (mask)
        bestsme = av1_get_mvpred_mask_var(
            x, best_mv, &ref_mv[id].as_mv, second_pred, mask, mask_stride, id,
            &cpi->fn_ptr[bsize], &x->plane[0].src, &ref_yv12[id], 1);
      else
        bestsme = av1_get_mvpred_av_var(x, best_mv, &ref_mv[id].as_mv,
                                        second_pred, &cpi->fn_ptr[bsize],
                                        &x->plane[0].src, &ref_yv12[id], 1);
    }

    x->mv_limits = tmp_mv_limits;

    // Restore the pointer to the first (possibly scaled) prediction buffer.
    if (id) xd->plane[plane].pre[0] = ref_yv12[0];

    for (ref = 0; ref < 2; ++ref) {
      if (scaled_ref_frame[ref]) {
        // Swap back the original buffers for subpel motion search.
        for (int i = 0; i < num_planes; i++) {
          xd->plane[i].pre[ref] = backup_yv12[ref][i];
        }
        // Re-initialize based on unscaled prediction buffers.
        ref_yv12[ref] = xd->plane[plane].pre[ref];
      }
    }

    // Do sub-pixel compound motion search on the current reference frame.
    if (id) xd->plane[plane].pre[0] = ref_yv12[id];

    if (cpi->common.cur_frame_force_integer_mv) {
      x->best_mv.as_mv.row *= 8;
      x->best_mv.as_mv.col *= 8;
    }
    if (bestsme < INT_MAX && cpi->common.cur_frame_force_integer_mv == 0) {
      int dis; /* TODO: use dis in distortion calculation later. */
      unsigned int sse;
      bestsme = cpi->find_fractional_mv_step(
          x, cm, mi_row, mi_col, &ref_mv[id].as_mv,
          cpi->common.allow_high_precision_mv, x->errorperbit,
          &cpi->fn_ptr[bsize], 0, cpi->sf.mv_sf.subpel_iters_per_step, NULL,
          x->nmv_vec_cost, x->mv_cost_stack, &dis, &sse, second_pred, mask,
          mask_stride, id, pw, ph, cpi->sf.mv_sf.use_accurate_subpel_search, 1);
    }

    // Restore the pointer to the first prediction buffer.
    if (id) xd->plane[plane].pre[0] = ref_yv12[0];
    if (bestsme < last_besterr[id]) {
      cur_mv[id].as_mv = *best_mv;
      last_besterr[id] = bestsme;
    } else {
      break;
    }
  }

  *rate_mv = 0;

  for (ref = 0; ref < 2; ++ref) {
    const int_mv curr_ref_mv = av1_get_ref_mv(x, ref);
    *rate_mv +=
        av1_mv_bit_cost(&cur_mv[ref].as_mv, &curr_ref_mv.as_mv, x->nmv_vec_cost,
                        x->mv_cost_stack, MV_COST_WEIGHT);
  }
}

static AOM_INLINE void estimate_ref_frame_costs(
    const AV1_COMMON *cm, const MACROBLOCKD *xd, const MACROBLOCK *x,
    int segment_id, unsigned int *ref_costs_single,
    unsigned int (*ref_costs_comp)[REF_FRAMES]) {
  int seg_ref_active =
      segfeature_active(&cm->seg, segment_id, SEG_LVL_REF_FRAME);
  if (seg_ref_active) {
    memset(ref_costs_single, 0, REF_FRAMES * sizeof(*ref_costs_single));
    int ref_frame;
    for (ref_frame = 0; ref_frame < REF_FRAMES; ++ref_frame)
      memset(ref_costs_comp[ref_frame], 0,
             REF_FRAMES * sizeof((*ref_costs_comp)[0]));
  } else {
    int intra_inter_ctx = av1_get_intra_inter_context(xd);
    ref_costs_single[INTRA_FRAME] = x->intra_inter_cost[intra_inter_ctx][0];
    unsigned int base_cost = x->intra_inter_cost[intra_inter_ctx][1];

    for (int i = LAST_FRAME; i <= ALTREF_FRAME; ++i)
      ref_costs_single[i] = base_cost;

    const int ctx_p1 = av1_get_pred_context_single_ref_p1(xd);
    const int ctx_p2 = av1_get_pred_context_single_ref_p2(xd);
    const int ctx_p3 = av1_get_pred_context_single_ref_p3(xd);
    const int ctx_p4 = av1_get_pred_context_single_ref_p4(xd);
    const int ctx_p5 = av1_get_pred_context_single_ref_p5(xd);
    const int ctx_p6 = av1_get_pred_context_single_ref_p6(xd);

    // Determine cost of a single ref frame, where frame types are represented
    // by a tree:
    // Level 0: add cost whether this ref is a forward or backward ref
    ref_costs_single[LAST_FRAME] += x->single_ref_cost[ctx_p1][0][0];
    ref_costs_single[LAST2_FRAME] += x->single_ref_cost[ctx_p1][0][0];
    ref_costs_single[LAST3_FRAME] += x->single_ref_cost[ctx_p1][0][0];
    ref_costs_single[GOLDEN_FRAME] += x->single_ref_cost[ctx_p1][0][0];
    ref_costs_single[BWDREF_FRAME] += x->single_ref_cost[ctx_p1][0][1];
    ref_costs_single[ALTREF2_FRAME] += x->single_ref_cost[ctx_p1][0][1];
    ref_costs_single[ALTREF_FRAME] += x->single_ref_cost[ctx_p1][0][1];

    // Level 1: if this ref is forward ref,
    // add cost whether it is last/last2 or last3/golden
    ref_costs_single[LAST_FRAME] += x->single_ref_cost[ctx_p3][2][0];
    ref_costs_single[LAST2_FRAME] += x->single_ref_cost[ctx_p3][2][0];
    ref_costs_single[LAST3_FRAME] += x->single_ref_cost[ctx_p3][2][1];
    ref_costs_single[GOLDEN_FRAME] += x->single_ref_cost[ctx_p3][2][1];

    // Level 1: if this ref is backward ref
    // then add cost whether this ref is altref or backward ref
    ref_costs_single[BWDREF_FRAME] += x->single_ref_cost[ctx_p2][1][0];
    ref_costs_single[ALTREF2_FRAME] += x->single_ref_cost[ctx_p2][1][0];
    ref_costs_single[ALTREF_FRAME] += x->single_ref_cost[ctx_p2][1][1];

    // Level 2: further add cost whether this ref is last or last2
    ref_costs_single[LAST_FRAME] += x->single_ref_cost[ctx_p4][3][0];
    ref_costs_single[LAST2_FRAME] += x->single_ref_cost[ctx_p4][3][1];

    // Level 2: last3 or golden
    ref_costs_single[LAST3_FRAME] += x->single_ref_cost[ctx_p5][4][0];
    ref_costs_single[GOLDEN_FRAME] += x->single_ref_cost[ctx_p5][4][1];

    // Level 2: bwdref or altref2
    ref_costs_single[BWDREF_FRAME] += x->single_ref_cost[ctx_p6][5][0];
    ref_costs_single[ALTREF2_FRAME] += x->single_ref_cost[ctx_p6][5][1];

    if (cm->current_frame.reference_mode != SINGLE_REFERENCE) {
      // Similar to single ref, determine cost of compound ref frames.
      // cost_compound_refs = cost_first_ref + cost_second_ref
      const int bwdref_comp_ctx_p = av1_get_pred_context_comp_bwdref_p(xd);
      const int bwdref_comp_ctx_p1 = av1_get_pred_context_comp_bwdref_p1(xd);
      const int ref_comp_ctx_p = av1_get_pred_context_comp_ref_p(xd);
      const int ref_comp_ctx_p1 = av1_get_pred_context_comp_ref_p1(xd);
      const int ref_comp_ctx_p2 = av1_get_pred_context_comp_ref_p2(xd);

      const int comp_ref_type_ctx = av1_get_comp_reference_type_context(xd);
      unsigned int ref_bicomp_costs[REF_FRAMES] = { 0 };

      ref_bicomp_costs[LAST_FRAME] = ref_bicomp_costs[LAST2_FRAME] =
          ref_bicomp_costs[LAST3_FRAME] = ref_bicomp_costs[GOLDEN_FRAME] =
              base_cost + x->comp_ref_type_cost[comp_ref_type_ctx][1];
      ref_bicomp_costs[BWDREF_FRAME] = ref_bicomp_costs[ALTREF2_FRAME] = 0;
      ref_bicomp_costs[ALTREF_FRAME] = 0;

      // cost of first ref frame
      ref_bicomp_costs[LAST_FRAME] += x->comp_ref_cost[ref_comp_ctx_p][0][0];
      ref_bicomp_costs[LAST2_FRAME] += x->comp_ref_cost[ref_comp_ctx_p][0][0];
      ref_bicomp_costs[LAST3_FRAME] += x->comp_ref_cost[ref_comp_ctx_p][0][1];
      ref_bicomp_costs[GOLDEN_FRAME] += x->comp_ref_cost[ref_comp_ctx_p][0][1];

      ref_bicomp_costs[LAST_FRAME] += x->comp_ref_cost[ref_comp_ctx_p1][1][0];
      ref_bicomp_costs[LAST2_FRAME] += x->comp_ref_cost[ref_comp_ctx_p1][1][1];

      ref_bicomp_costs[LAST3_FRAME] += x->comp_ref_cost[ref_comp_ctx_p2][2][0];
      ref_bicomp_costs[GOLDEN_FRAME] += x->comp_ref_cost[ref_comp_ctx_p2][2][1];

      // cost of second ref frame
      ref_bicomp_costs[BWDREF_FRAME] +=
          x->comp_bwdref_cost[bwdref_comp_ctx_p][0][0];
      ref_bicomp_costs[ALTREF2_FRAME] +=
          x->comp_bwdref_cost[bwdref_comp_ctx_p][0][0];
      ref_bicomp_costs[ALTREF_FRAME] +=
          x->comp_bwdref_cost[bwdref_comp_ctx_p][0][1];

      ref_bicomp_costs[BWDREF_FRAME] +=
          x->comp_bwdref_cost[bwdref_comp_ctx_p1][1][0];
      ref_bicomp_costs[ALTREF2_FRAME] +=
          x->comp_bwdref_cost[bwdref_comp_ctx_p1][1][1];

      // cost: if one ref frame is forward ref, the other ref is backward ref
      int ref0, ref1;
      for (ref0 = LAST_FRAME; ref0 <= GOLDEN_FRAME; ++ref0) {
        for (ref1 = BWDREF_FRAME; ref1 <= ALTREF_FRAME; ++ref1) {
          ref_costs_comp[ref0][ref1] =
              ref_bicomp_costs[ref0] + ref_bicomp_costs[ref1];
        }
      }

      // cost: if both ref frames are the same side.
      const int uni_comp_ref_ctx_p = av1_get_pred_context_uni_comp_ref_p(xd);
      const int uni_comp_ref_ctx_p1 = av1_get_pred_context_uni_comp_ref_p1(xd);
      const int uni_comp_ref_ctx_p2 = av1_get_pred_context_uni_comp_ref_p2(xd);
      ref_costs_comp[LAST_FRAME][LAST2_FRAME] =
          base_cost + x->comp_ref_type_cost[comp_ref_type_ctx][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p][0][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p1][1][0];
      ref_costs_comp[LAST_FRAME][LAST3_FRAME] =
          base_cost + x->comp_ref_type_cost[comp_ref_type_ctx][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p][0][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p1][1][1] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p2][2][0];
      ref_costs_comp[LAST_FRAME][GOLDEN_FRAME] =
          base_cost + x->comp_ref_type_cost[comp_ref_type_ctx][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p][0][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p1][1][1] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p2][2][1];
      ref_costs_comp[BWDREF_FRAME][ALTREF_FRAME] =
          base_cost + x->comp_ref_type_cost[comp_ref_type_ctx][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p][0][1];
    } else {
      int ref0, ref1;
      for (ref0 = LAST_FRAME; ref0 <= GOLDEN_FRAME; ++ref0) {
        for (ref1 = BWDREF_FRAME; ref1 <= ALTREF_FRAME; ++ref1)
          ref_costs_comp[ref0][ref1] = 512;
      }
      ref_costs_comp[LAST_FRAME][LAST2_FRAME] = 512;
      ref_costs_comp[LAST_FRAME][LAST3_FRAME] = 512;
      ref_costs_comp[LAST_FRAME][GOLDEN_FRAME] = 512;
      ref_costs_comp[BWDREF_FRAME][ALTREF_FRAME] = 512;
    }
  }
}

static AOM_INLINE void store_coding_context(
#if CONFIG_INTERNAL_STATS
    MACROBLOCK *x, PICK_MODE_CONTEXT *ctx, int mode_index,
#else
    MACROBLOCK *x, PICK_MODE_CONTEXT *ctx,
#endif  // CONFIG_INTERNAL_STATS
    int64_t comp_pred_diff[REFERENCE_MODES], int skippable) {
  MACROBLOCKD *const xd = &x->e_mbd;

  // Take a snapshot of the coding context so it can be
  // restored if we decide to encode this way
  ctx->rd_stats.skip = x->force_skip;
  ctx->skippable = skippable;
#if CONFIG_INTERNAL_STATS
  ctx->best_mode_index = mode_index;
#endif  // CONFIG_INTERNAL_STATS
  ctx->mic = *xd->mi[0];
  ctx->mbmi_ext = *x->mbmi_ext;
  ctx->single_pred_diff = (int)comp_pred_diff[SINGLE_REFERENCE];
  ctx->comp_pred_diff = (int)comp_pred_diff[COMPOUND_REFERENCE];
  ctx->hybrid_pred_diff = (int)comp_pred_diff[REFERENCE_MODE_SELECT];
}

static AOM_INLINE void setup_buffer_ref_mvs_inter(
    const AV1_COMP *const cpi, MACROBLOCK *x, MV_REFERENCE_FRAME ref_frame,
    BLOCK_SIZE block_size, struct buf_2d yv12_mb[REF_FRAMES][MAX_MB_PLANE]) {
  const AV1_COMMON *cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  const YV12_BUFFER_CONFIG *scaled_ref_frame =
      av1_get_scaled_ref_frame(cpi, ref_frame);
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  MB_MODE_INFO_EXT *const mbmi_ext = x->mbmi_ext;
  const struct scale_factors *const sf =
      get_ref_scale_factors_const(cm, ref_frame);
  const YV12_BUFFER_CONFIG *yv12 = get_ref_frame_yv12_buf(cm, ref_frame);
  assert(yv12 != NULL);

  if (scaled_ref_frame) {
    // Setup pred block based on scaled reference, because av1_mv_pred() doesn't
    // support scaling.
    av1_setup_pred_block(xd, yv12_mb[ref_frame], scaled_ref_frame, NULL, NULL,
                         num_planes);
  } else {
    av1_setup_pred_block(xd, yv12_mb[ref_frame], yv12, sf, sf, num_planes);
  }

  // Gets an initial list of candidate vectors from neighbours and orders them
  av1_find_mv_refs(cm, xd, mbmi, ref_frame, mbmi_ext->ref_mv_count,
                   xd->ref_mv_stack, xd->weight, NULL, mbmi_ext->global_mvs,
                   mbmi_ext->mode_context);
  // TODO(Ravi): Populate mbmi_ext->ref_mv_stack[ref_frame][4] and
  // mbmi_ext->weight[ref_frame][4] inside av1_find_mv_refs.
  av1_copy_usable_ref_mv_stack_and_weight(xd, mbmi_ext, ref_frame);
  // Further refinement that is encode side only to test the top few candidates
  // in full and choose the best as the center point for subsequent searches.
  // The current implementation doesn't support scaling.
  av1_mv_pred(cpi, x, yv12_mb[ref_frame][0].buf, yv12_mb[ref_frame][0].stride,
              ref_frame, block_size);

  // Go back to unscaled reference.
  if (scaled_ref_frame) {
    // We had temporarily setup pred block based on scaled reference above. Go
    // back to unscaled reference now, for subsequent use.
    av1_setup_pred_block(xd, yv12_mb[ref_frame], yv12, sf, sf, num_planes);
  }
}

static AOM_INLINE void single_motion_search(const AV1_COMP *const cpi,
                                            MACROBLOCK *x, BLOCK_SIZE bsize,
                                            int ref_idx, int *rate_mv) {
  MACROBLOCKD *xd = &x->e_mbd;
  const AV1_COMMON *cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  MB_MODE_INFO *mbmi = xd->mi[0];
  struct buf_2d backup_yv12[MAX_MB_PLANE] = { { 0, 0, 0, 0, 0 } };
  int bestsme = INT_MAX;
  const int ref = mbmi->ref_frame[ref_idx];
  MvLimits tmp_mv_limits = x->mv_limits;
  const YV12_BUFFER_CONFIG *scaled_ref_frame =
      av1_get_scaled_ref_frame(cpi, ref);
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;

  if (scaled_ref_frame) {
    // Swap out the reference frame for a version that's been scaled to
    // match the resolution of the current frame, allowing the existing
    // full-pixel motion search code to be used without additional
    // modifications.
    for (int i = 0; i < num_planes; i++) {
      backup_yv12[i] = xd->plane[i].pre[ref_idx];
    }
    av1_setup_pre_planes(xd, ref_idx, scaled_ref_frame, mi_row, mi_col, NULL,
                         num_planes);
  }

  // Work out the size of the first step in the mv step search.
  // 0 here is maximum length first step. 1 is AOMMAX >> 1 etc.
  int step_param;
  if (cpi->sf.mv_sf.auto_mv_step_size && cm->show_frame) {
    // Take the weighted average of the step_params based on the last frame's
    // max mv magnitude and that based on the best ref mvs of the current
    // block for the given reference.
    step_param =
        (av1_init_search_range(x->max_mv_context[ref]) + cpi->mv_step_param) /
        2;
  } else {
    step_param = cpi->mv_step_param;
  }

  if (cpi->sf.mv_sf.adaptive_motion_search && bsize < cm->seq_params.sb_size) {
    int boffset =
        2 * (mi_size_wide_log2[cm->seq_params.sb_size] -
             AOMMIN(mi_size_high_log2[bsize], mi_size_wide_log2[bsize]));
    step_param = AOMMAX(step_param, boffset);
  }

  if (cpi->sf.mv_sf.adaptive_motion_search) {
    int bwl = mi_size_wide_log2[bsize];
    int bhl = mi_size_high_log2[bsize];
    int tlevel = x->pred_mv_sad[ref] >> (bwl + bhl + 4);

    if (tlevel < 5) {
      step_param += 2;
      step_param = AOMMIN(step_param, MAX_MVSEARCH_STEPS - 1);
    }

    // prev_mv_sad is not setup for dynamically scaled frames.
    if (cpi->oxcf.resize_mode != RESIZE_RANDOM) {
      int i;
      for (i = LAST_FRAME; i <= ALTREF_FRAME && cm->show_frame; ++i) {
        if ((x->pred_mv_sad[ref] >> 3) > x->pred_mv_sad[i]) {
          x->pred_mv[ref].row = 0;
          x->pred_mv[ref].col = 0;
          x->best_mv.as_int = INVALID_MV;

          if (scaled_ref_frame) {
            // Swap back the original buffers before returning.
            for (int j = 0; j < num_planes; ++j)
              xd->plane[j].pre[ref_idx] = backup_yv12[j];
          }
          return;
        }
      }
    }
  }

  const MV ref_mv = av1_get_ref_mv(x, ref_idx).as_mv;
  // Note: MV limits are modified here. Always restore the original values
  // after full-pixel motion search.
  av1_set_mv_search_range(&x->mv_limits, &ref_mv);

  MV mvp_full;
  if (mbmi->motion_mode != SIMPLE_TRANSLATION)
    mvp_full = mbmi->mv[0].as_mv;
  else
    mvp_full = ref_mv;

  mvp_full.col >>= 3;
  mvp_full.row >>= 3;

  const int sadpb = x->sadperbit16;
  int cost_list[5];
  x->best_mv.as_int = x->second_best_mv.as_int = INVALID_MV;
  switch (mbmi->motion_mode) {
    case SIMPLE_TRANSLATION:
      bestsme = av1_full_pixel_search(
          cpi, x, bsize, &mvp_full, step_param, 1, cpi->sf.mv_sf.search_method,
          0, sadpb, cond_cost_list(cpi, cost_list), &ref_mv, INT_MAX, 1,
          (MI_SIZE * mi_col), (MI_SIZE * mi_row), 0, &cpi->ss_cfg[SS_CFG_SRC],
          0);
      break;
    case OBMC_CAUSAL:
      bestsme = av1_obmc_full_pixel_search(
          cpi, x, &mvp_full, step_param, sadpb,
          MAX_MVSEARCH_STEPS - 1 - step_param, 1, &cpi->fn_ptr[bsize], &ref_mv,
          &(x->best_mv.as_mv), 0, &cpi->ss_cfg[SS_CFG_SRC]);
      break;
    default: assert(0 && "Invalid motion mode!\n");
  }

  if (scaled_ref_frame) {
    // Swap back the original buffers for subpel motion search.
    for (int i = 0; i < num_planes; i++) {
      xd->plane[i].pre[ref_idx] = backup_yv12[i];
    }
  }

  x->mv_limits = tmp_mv_limits;

  if (cpi->common.cur_frame_force_integer_mv) {
    x->best_mv.as_mv.row *= 8;
    x->best_mv.as_mv.col *= 8;
  }
  const int use_fractional_mv =
      bestsme < INT_MAX && cpi->common.cur_frame_force_integer_mv == 0;
  if (use_fractional_mv) {
    int dis; /* TODO: use dis in distortion calculation later. */
    switch (mbmi->motion_mode) {
      case SIMPLE_TRANSLATION:
        if (cpi->sf.mv_sf.use_accurate_subpel_search) {
          const int try_second = x->second_best_mv.as_int != INVALID_MV &&
                                 x->second_best_mv.as_int != x->best_mv.as_int;
          const int pw = block_size_wide[bsize];
          const int ph = block_size_high[bsize];
          const int best_mv_var = cpi->find_fractional_mv_step(
              x, cm, mi_row, mi_col, &ref_mv, cm->allow_high_precision_mv,
              x->errorperbit, &cpi->fn_ptr[bsize],
              cpi->sf.mv_sf.subpel_force_stop,
              cpi->sf.mv_sf.subpel_iters_per_step,
              cond_cost_list(cpi, cost_list), x->nmv_vec_cost, x->mv_cost_stack,
              &dis, &x->pred_sse[ref], NULL, NULL, 0, 0, pw, ph,
              cpi->sf.mv_sf.use_accurate_subpel_search, 1);

          if (try_second) {
            const int minc =
                AOMMAX(x->mv_limits.col_min * 8, ref_mv.col - MV_MAX);
            const int maxc =
                AOMMIN(x->mv_limits.col_max * 8, ref_mv.col + MV_MAX);
            const int minr =
                AOMMAX(x->mv_limits.row_min * 8, ref_mv.row - MV_MAX);
            const int maxr =
                AOMMIN(x->mv_limits.row_max * 8, ref_mv.row + MV_MAX);
            MV best_mv = x->best_mv.as_mv;

            x->best_mv = x->second_best_mv;
            if (x->best_mv.as_mv.row * 8 <= maxr &&
                x->best_mv.as_mv.row * 8 >= minr &&
                x->best_mv.as_mv.col * 8 <= maxc &&
                x->best_mv.as_mv.col * 8 >= minc) {
              const int this_var = cpi->find_fractional_mv_step(
                  x, cm, mi_row, mi_col, &ref_mv, cm->allow_high_precision_mv,
                  x->errorperbit, &cpi->fn_ptr[bsize],
                  cpi->sf.mv_sf.subpel_force_stop,
                  cpi->sf.mv_sf.subpel_iters_per_step,
                  cond_cost_list(cpi, cost_list), x->nmv_vec_cost,
                  x->mv_cost_stack, &dis, &x->pred_sse[ref], NULL, NULL, 0, 0,
                  pw, ph, cpi->sf.mv_sf.use_accurate_subpel_search, 0);
              if (this_var < best_mv_var) best_mv = x->best_mv.as_mv;
            }
            x->best_mv.as_mv = best_mv;
          }
        } else {
          cpi->find_fractional_mv_step(
              x, cm, mi_row, mi_col, &ref_mv, cm->allow_high_precision_mv,
              x->errorperbit, &cpi->fn_ptr[bsize],
              cpi->sf.mv_sf.subpel_force_stop,
              cpi->sf.mv_sf.subpel_iters_per_step,
              cond_cost_list(cpi, cost_list), x->nmv_vec_cost, x->mv_cost_stack,
              &dis, &x->pred_sse[ref], NULL, NULL, 0, 0, 0, 0, 0, 1);
        }
        break;
      case OBMC_CAUSAL:
        av1_find_best_obmc_sub_pixel_tree_up(
            x, cm, mi_row, mi_col, &x->best_mv.as_mv, &ref_mv,
            cm->allow_high_precision_mv, x->errorperbit, &cpi->fn_ptr[bsize],
            cpi->sf.mv_sf.subpel_force_stop,
            cpi->sf.mv_sf.subpel_iters_per_step, x->nmv_vec_cost,
            x->mv_cost_stack, &dis, &x->pred_sse[ref], 0,
            cpi->sf.mv_sf.use_accurate_subpel_search);
        break;
      default: assert(0 && "Invalid motion mode!\n");
    }
  }
  *rate_mv = av1_mv_bit_cost(&x->best_mv.as_mv, &ref_mv, x->nmv_vec_cost,
                             x->mv_cost_stack, MV_COST_WEIGHT);

  if (cpi->sf.mv_sf.adaptive_motion_search &&
      mbmi->motion_mode == SIMPLE_TRANSLATION)
    x->pred_mv[ref] = x->best_mv.as_mv;
}

static INLINE void restore_dst_buf(MACROBLOCKD *xd, const BUFFER_SET dst,
                                   const int num_planes) {
  for (int i = 0; i < num_planes; i++) {
    xd->plane[i].dst.buf = dst.plane[i];
    xd->plane[i].dst.stride = dst.stride[i];
  }
}

static AOM_INLINE void build_second_inter_pred(const AV1_COMP *cpi,
                                               MACROBLOCK *x, BLOCK_SIZE bsize,
                                               const MV *other_mv, int ref_idx,
                                               uint8_t *second_pred) {
  const AV1_COMMON *const cm = &cpi->common;
  const int pw = block_size_wide[bsize];
  const int ph = block_size_high[bsize];
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  struct macroblockd_plane *const pd = &xd->plane[0];
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  const int p_col = ((mi_col * MI_SIZE) >> pd->subsampling_x);
  const int p_row = ((mi_row * MI_SIZE) >> pd->subsampling_y);

  // This function should only ever be called for compound modes
  assert(has_second_ref(mbmi));

  const int plane = 0;
  struct buf_2d ref_yv12 = xd->plane[plane].pre[!ref_idx];

  struct scale_factors sf;
  av1_setup_scale_factors_for_frame(&sf, ref_yv12.width, ref_yv12.height,
                                    cm->width, cm->height);

  InterPredParams inter_pred_params;

  av1_init_inter_params(&inter_pred_params, pw, ph, p_row, p_col,
                        pd->subsampling_x, pd->subsampling_y, xd->bd,
                        is_cur_buf_hbd(xd), 0, &sf, &ref_yv12,
                        mbmi->interp_filters);
  inter_pred_params.conv_params = get_conv_params(0, plane, xd->bd);

  // Get the prediction block from the 'other' reference frame.
  av1_build_inter_predictor(second_pred, pw, other_mv, &inter_pred_params);

  av1_dist_wtd_comp_weight_assign(cm, mbmi, 0, &xd->jcp_param.fwd_offset,
                                  &xd->jcp_param.bck_offset,
                                  &xd->jcp_param.use_dist_wtd_comp_avg, 1);
}

// Search for the best mv for one component of a compound,
// given that the other component is fixed.
static AOM_INLINE void compound_single_motion_search(
    const AV1_COMP *cpi, MACROBLOCK *x, BLOCK_SIZE bsize, MV *this_mv,
    const uint8_t *second_pred, const uint8_t *mask, int mask_stride,
    int *rate_mv, int ref_idx) {
  const AV1_COMMON *const cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  const int pw = block_size_wide[bsize];
  const int ph = block_size_high[bsize];
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  const int ref = mbmi->ref_frame[ref_idx];
  const int_mv ref_mv = av1_get_ref_mv(x, ref_idx);
  struct macroblockd_plane *const pd = &xd->plane[0];

  struct buf_2d backup_yv12[MAX_MB_PLANE];
  const YV12_BUFFER_CONFIG *const scaled_ref_frame =
      av1_get_scaled_ref_frame(cpi, ref);

  // Check that this is either an interinter or an interintra block
  assert(has_second_ref(mbmi) || (ref_idx == 0 && is_interintra_mode(mbmi)));

  // Store the first prediction buffer.
  struct buf_2d orig_yv12;
  struct buf_2d ref_yv12 = pd->pre[ref_idx];
  if (ref_idx) {
    orig_yv12 = pd->pre[0];
    pd->pre[0] = pd->pre[ref_idx];
  }

  if (scaled_ref_frame) {
    // Swap out the reference frame for a version that's been scaled to
    // match the resolution of the current frame, allowing the existing
    // full-pixel motion search code to be used without additional
    // modifications.
    for (int i = 0; i < num_planes; i++) {
      backup_yv12[i] = xd->plane[i].pre[ref_idx];
    }
    const int mi_row = xd->mi_row;
    const int mi_col = xd->mi_col;
    av1_setup_pre_planes(xd, ref_idx, scaled_ref_frame, mi_row, mi_col, NULL,
                         num_planes);
  }

  int bestsme = INT_MAX;
  int sadpb = x->sadperbit16;
  MV *const best_mv = &x->best_mv.as_mv;
  int search_range = SEARCH_RANGE_8P;

  MvLimits tmp_mv_limits = x->mv_limits;

  // Do compound motion search on the current reference frame.
  av1_set_mv_search_range(&x->mv_limits, &ref_mv.as_mv);

  // Use the mv result from the single mode as mv predictor.
  *best_mv = *this_mv;

  best_mv->col >>= 3;
  best_mv->row >>= 3;

  // Small-range full-pixel motion search.
  bestsme = av1_refining_search_8p_c(
      x, sadpb, search_range, &cpi->fn_ptr[bsize], mask, mask_stride, ref_idx,
      &ref_mv.as_mv, second_pred, &x->plane[0].src, &ref_yv12);
  if (bestsme < INT_MAX) {
    if (mask)
      bestsme = av1_get_mvpred_mask_var(
          x, best_mv, &ref_mv.as_mv, second_pred, mask, mask_stride, ref_idx,
          &cpi->fn_ptr[bsize], &x->plane[0].src, &ref_yv12, 1);
    else
      bestsme = av1_get_mvpred_av_var(x, best_mv, &ref_mv.as_mv, second_pred,
                                      &cpi->fn_ptr[bsize], &x->plane[0].src,
                                      &ref_yv12, 1);
  }

  x->mv_limits = tmp_mv_limits;

  if (scaled_ref_frame) {
    // Swap back the original buffers for subpel motion search.
    for (int i = 0; i < num_planes; i++) {
      xd->plane[i].pre[ref_idx] = backup_yv12[i];
    }
  }

  if (cpi->common.cur_frame_force_integer_mv) {
    x->best_mv.as_mv.row *= 8;
    x->best_mv.as_mv.col *= 8;
  }
  const int use_fractional_mv =
      bestsme < INT_MAX && cpi->common.cur_frame_force_integer_mv == 0;
  if (use_fractional_mv) {
    int dis; /* TODO: use dis in distortion calculation later. */
    unsigned int sse;
    const int mi_row = xd->mi_row;
    const int mi_col = xd->mi_col;
    bestsme = cpi->find_fractional_mv_step(
        x, cm, mi_row, mi_col, &ref_mv.as_mv,
        cpi->common.allow_high_precision_mv, x->errorperbit,
        &cpi->fn_ptr[bsize], 0, cpi->sf.mv_sf.subpel_iters_per_step, NULL,
        x->nmv_vec_cost, x->mv_cost_stack, &dis, &sse, second_pred, mask,
        mask_stride, ref_idx, pw, ph, cpi->sf.mv_sf.use_accurate_subpel_search,
        1);
  }

  // Restore the pointer to the first unscaled prediction buffer.
  if (ref_idx) pd->pre[0] = orig_yv12;

  if (bestsme < INT_MAX) *this_mv = *best_mv;

  *rate_mv = 0;

  *rate_mv += av1_mv_bit_cost(this_mv, &ref_mv.as_mv, x->nmv_vec_cost,
                              x->mv_cost_stack, MV_COST_WEIGHT);
}

// Wrapper for compound_single_motion_search, for the common case
// where the second prediction is also an inter mode.
static AOM_INLINE void compound_single_motion_search_interinter(
    const AV1_COMP *cpi, MACROBLOCK *x, BLOCK_SIZE bsize, int_mv *cur_mv,
    const uint8_t *mask, int mask_stride, int *rate_mv, int ref_idx) {
  MACROBLOCKD *xd = &x->e_mbd;
  // This function should only ever be called for compound modes
  assert(has_second_ref(xd->mi[0]));

  // Prediction buffer from second frame.
  DECLARE_ALIGNED(16, uint16_t, second_pred_alloc_16[MAX_SB_SQUARE]);
  uint8_t *second_pred;
  if (is_cur_buf_hbd(xd))
    second_pred = CONVERT_TO_BYTEPTR(second_pred_alloc_16);
  else
    second_pred = (uint8_t *)second_pred_alloc_16;

  MV *this_mv = &cur_mv[ref_idx].as_mv;
  const MV *other_mv = &cur_mv[!ref_idx].as_mv;
  build_second_inter_pred(cpi, x, bsize, other_mv, ref_idx, second_pred);
  compound_single_motion_search(cpi, x, bsize, this_mv, second_pred, mask,
                                mask_stride, rate_mv, ref_idx);
}

static AOM_INLINE void do_masked_motion_search_indexed(
    const AV1_COMP *const cpi, MACROBLOCK *x, const int_mv *const cur_mv,
    const INTERINTER_COMPOUND_DATA *const comp_data, BLOCK_SIZE bsize,
    int_mv *tmp_mv, int *rate_mv, int which) {
  // NOTE: which values: 0 - 0 only, 1 - 1 only, 2 - both
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  BLOCK_SIZE sb_type = mbmi->sb_type;
  const uint8_t *mask;
  const int mask_stride = block_size_wide[bsize];

  mask = av1_get_compound_type_mask(comp_data, sb_type);

  tmp_mv[0].as_int = cur_mv[0].as_int;
  tmp_mv[1].as_int = cur_mv[1].as_int;
  if (which == 0 || which == 1) {
    compound_single_motion_search_interinter(cpi, x, bsize, tmp_mv, mask,
                                             mask_stride, rate_mv, which);
  } else if (which == 2) {
    joint_motion_search(cpi, x, bsize, tmp_mv, mask, mask_stride, rate_mv);
  }
}

#define LEFT_TOP_MARGIN ((AOM_BORDER_IN_PIXELS - AOM_INTERP_EXTEND) << 3)
#define RIGHT_BOTTOM_MARGIN ((AOM_BORDER_IN_PIXELS - AOM_INTERP_EXTEND) << 3)

// TODO(jingning): this mv clamping function should be block size dependent.
static INLINE void clamp_mv2(MV *mv, const MACROBLOCKD *xd) {
  clamp_mv(mv, xd->mb_to_left_edge - LEFT_TOP_MARGIN,
           xd->mb_to_right_edge + RIGHT_BOTTOM_MARGIN,
           xd->mb_to_top_edge - LEFT_TOP_MARGIN,
           xd->mb_to_bottom_edge + RIGHT_BOTTOM_MARGIN);
}

static int8_t estimate_wedge_sign(const AV1_COMP *cpi, const MACROBLOCK *x,
                                  const BLOCK_SIZE bsize, const uint8_t *pred0,
                                  int stride0, const uint8_t *pred1,
                                  int stride1) {
  static const BLOCK_SIZE split_qtr[BLOCK_SIZES_ALL] = {
    //                            4X4
    BLOCK_INVALID,
    // 4X8,        8X4,           8X8
    BLOCK_INVALID, BLOCK_INVALID, BLOCK_4X4,
    // 8X16,       16X8,          16X16
    BLOCK_4X8, BLOCK_8X4, BLOCK_8X8,
    // 16X32,      32X16,         32X32
    BLOCK_8X16, BLOCK_16X8, BLOCK_16X16,
    // 32X64,      64X32,         64X64
    BLOCK_16X32, BLOCK_32X16, BLOCK_32X32,
    // 64x128,     128x64,        128x128
    BLOCK_32X64, BLOCK_64X32, BLOCK_64X64,
    // 4X16,       16X4,          8X32
    BLOCK_INVALID, BLOCK_INVALID, BLOCK_4X16,
    // 32X8,       16X64,         64X16
    BLOCK_16X4, BLOCK_8X32, BLOCK_32X8
  };
  const struct macroblock_plane *const p = &x->plane[0];
  const uint8_t *src = p->src.buf;
  int src_stride = p->src.stride;
  const int bw = block_size_wide[bsize];
  const int bh = block_size_high[bsize];
  const int bw_by2 = bw >> 1;
  const int bh_by2 = bh >> 1;
  uint32_t esq[2][2];
  int64_t tl, br;

  const BLOCK_SIZE f_index = split_qtr[bsize];
  assert(f_index != BLOCK_INVALID);

  if (is_cur_buf_hbd(&x->e_mbd)) {
    pred0 = CONVERT_TO_BYTEPTR(pred0);
    pred1 = CONVERT_TO_BYTEPTR(pred1);
  }

  // Residual variance computation over relevant quandrants in order to
  // find TL + BR, TL = sum(1st,2nd,3rd) quadrants of (pred0 - pred1),
  // BR = sum(2nd,3rd,4th) quadrants of (pred1 - pred0)
  // The 2nd and 3rd quadrants cancel out in TL + BR
  // Hence TL + BR = 1st quadrant of (pred0-pred1) + 4th of (pred1-pred0)
  // TODO(nithya): Sign estimation assumes 45 degrees (1st and 4th quadrants)
  // for all codebooks; experiment with other quadrant combinations for
  // 0, 90 and 135 degrees also.
  cpi->fn_ptr[f_index].vf(src, src_stride, pred0, stride0, &esq[0][0]);
  cpi->fn_ptr[f_index].vf(src + bh_by2 * src_stride + bw_by2, src_stride,
                          pred0 + bh_by2 * stride0 + bw_by2, stride0,
                          &esq[0][1]);
  cpi->fn_ptr[f_index].vf(src, src_stride, pred1, stride1, &esq[1][0]);
  cpi->fn_ptr[f_index].vf(src + bh_by2 * src_stride + bw_by2, src_stride,
                          pred1 + bh_by2 * stride1 + bw_by2, stride0,
                          &esq[1][1]);

  tl = ((int64_t)esq[0][0]) - ((int64_t)esq[1][0]);
  br = ((int64_t)esq[1][1]) - ((int64_t)esq[0][1]);
  return (tl + br > 0);
}

// Choose the best wedge index and sign
static int64_t pick_wedge(const AV1_COMP *const cpi, const MACROBLOCK *const x,
                          const BLOCK_SIZE bsize, const uint8_t *const p0,
                          const int16_t *const residual1,
                          const int16_t *const diff10,
                          int8_t *const best_wedge_sign,
                          int8_t *const best_wedge_index) {
  const MACROBLOCKD *const xd = &x->e_mbd;
  const struct buf_2d *const src = &x->plane[0].src;
  const int bw = block_size_wide[bsize];
  const int bh = block_size_high[bsize];
  const int N = bw * bh;
  assert(N >= 64);
  int rate;
  int64_t dist;
  int64_t rd, best_rd = INT64_MAX;
  int8_t wedge_index;
  int8_t wedge_sign;
  const int8_t wedge_types = get_wedge_types_lookup(bsize);
  const uint8_t *mask;
  uint64_t sse;
  const int hbd = is_cur_buf_hbd(xd);
  const int bd_round = hbd ? (xd->bd - 8) * 2 : 0;

  DECLARE_ALIGNED(32, int16_t, residual0[MAX_SB_SQUARE]);  // src - pred0
#if CONFIG_AV1_HIGHBITDEPTH
  if (hbd) {
    aom_highbd_subtract_block(bh, bw, residual0, bw, src->buf, src->stride,
                              CONVERT_TO_BYTEPTR(p0), bw, xd->bd);
  } else {
    aom_subtract_block(bh, bw, residual0, bw, src->buf, src->stride, p0, bw);
  }
#else
  (void)hbd;
  aom_subtract_block(bh, bw, residual0, bw, src->buf, src->stride, p0, bw);
#endif

  int64_t sign_limit = ((int64_t)aom_sum_squares_i16(residual0, N) -
                        (int64_t)aom_sum_squares_i16(residual1, N)) *
                       (1 << WEDGE_WEIGHT_BITS) / 2;
  int16_t *ds = residual0;

  av1_wedge_compute_delta_squares(ds, residual0, residual1, N);

  for (wedge_index = 0; wedge_index < wedge_types; ++wedge_index) {
    mask = av1_get_contiguous_soft_mask(wedge_index, 0, bsize);

    wedge_sign = av1_wedge_sign_from_residuals(ds, mask, N, sign_limit);

    mask = av1_get_contiguous_soft_mask(wedge_index, wedge_sign, bsize);
    sse = av1_wedge_sse_from_residuals(residual1, diff10, mask, N);
    sse = ROUND_POWER_OF_TWO(sse, bd_round);

    model_rd_sse_fn[MODELRD_TYPE_MASKED_COMPOUND](cpi, x, bsize, 0, sse, N,
                                                  &rate, &dist);
    // int rate2;
    // int64_t dist2;
    // model_rd_with_curvfit(cpi, x, bsize, 0, sse, N, &rate2, &dist2);
    // printf("sse %"PRId64": leagacy: %d %"PRId64", curvfit %d %"PRId64"\n",
    // sse, rate, dist, rate2, dist2); dist = dist2;
    // rate = rate2;

    rate += x->wedge_idx_cost[bsize][wedge_index];
    rd = RDCOST(x->rdmult, rate, dist);

    if (rd < best_rd) {
      *best_wedge_index = wedge_index;
      *best_wedge_sign = wedge_sign;
      best_rd = rd;
    }
  }

  return best_rd -
         RDCOST(x->rdmult, x->wedge_idx_cost[bsize][*best_wedge_index], 0);
}

// Choose the best wedge index the specified sign
static int64_t pick_wedge_fixed_sign(const AV1_COMP *const cpi,
                                     const MACROBLOCK *const x,
                                     const BLOCK_SIZE bsize,
                                     const int16_t *const residual1,
                                     const int16_t *const diff10,
                                     const int8_t wedge_sign,
                                     int8_t *const best_wedge_index) {
  const MACROBLOCKD *const xd = &x->e_mbd;

  const int bw = block_size_wide[bsize];
  const int bh = block_size_high[bsize];
  const int N = bw * bh;
  assert(N >= 64);
  int rate;
  int64_t dist;
  int64_t rd, best_rd = INT64_MAX;
  int8_t wedge_index;
  const int8_t wedge_types = get_wedge_types_lookup(bsize);
  const uint8_t *mask;
  uint64_t sse;
  const int hbd = is_cur_buf_hbd(xd);
  const int bd_round = hbd ? (xd->bd - 8) * 2 : 0;
  for (wedge_index = 0; wedge_index < wedge_types; ++wedge_index) {
    mask = av1_get_contiguous_soft_mask(wedge_index, wedge_sign, bsize);
    sse = av1_wedge_sse_from_residuals(residual1, diff10, mask, N);
    sse = ROUND_POWER_OF_TWO(sse, bd_round);

    model_rd_sse_fn[MODELRD_TYPE_MASKED_COMPOUND](cpi, x, bsize, 0, sse, N,
                                                  &rate, &dist);
    rate += x->wedge_idx_cost[bsize][wedge_index];
    rd = RDCOST(x->rdmult, rate, dist);

    if (rd < best_rd) {
      *best_wedge_index = wedge_index;
      best_rd = rd;
    }
  }
  return best_rd -
         RDCOST(x->rdmult, x->wedge_idx_cost[bsize][*best_wedge_index], 0);
}

static int64_t pick_interinter_wedge(
    const AV1_COMP *const cpi, MACROBLOCK *const x, const BLOCK_SIZE bsize,
    const uint8_t *const p0, const uint8_t *const p1,
    const int16_t *const residual1, const int16_t *const diff10) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const int bw = block_size_wide[bsize];

  int64_t rd;
  int8_t wedge_index = -1;
  int8_t wedge_sign = 0;

  assert(is_interinter_compound_used(COMPOUND_WEDGE, bsize));
  assert(cpi->common.seq_params.enable_masked_compound);

  if (cpi->sf.inter_sf.fast_wedge_sign_estimate) {
    wedge_sign = estimate_wedge_sign(cpi, x, bsize, p0, bw, p1, bw);
    rd = pick_wedge_fixed_sign(cpi, x, bsize, residual1, diff10, wedge_sign,
                               &wedge_index);
  } else {
    rd = pick_wedge(cpi, x, bsize, p0, residual1, diff10, &wedge_sign,
                    &wedge_index);
  }

  mbmi->interinter_comp.wedge_sign = wedge_sign;
  mbmi->interinter_comp.wedge_index = wedge_index;
  return rd;
}

static int64_t pick_interinter_seg(const AV1_COMP *const cpi,
                                   MACROBLOCK *const x, const BLOCK_SIZE bsize,
                                   const uint8_t *const p0,
                                   const uint8_t *const p1,
                                   const int16_t *const residual1,
                                   const int16_t *const diff10) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const int bw = block_size_wide[bsize];
  const int bh = block_size_high[bsize];
  const int N = 1 << num_pels_log2_lookup[bsize];
  int rate;
  int64_t dist;
  DIFFWTD_MASK_TYPE cur_mask_type;
  int64_t best_rd = INT64_MAX;
  DIFFWTD_MASK_TYPE best_mask_type = 0;
  const int hbd = is_cur_buf_hbd(xd);
  const int bd_round = hbd ? (xd->bd - 8) * 2 : 0;
  DECLARE_ALIGNED(16, uint8_t, seg_mask[2 * MAX_SB_SQUARE]);
  uint8_t *tmp_mask[2] = { xd->seg_mask, seg_mask };
  // try each mask type and its inverse
  for (cur_mask_type = 0; cur_mask_type < DIFFWTD_MASK_TYPES; cur_mask_type++) {
    // build mask and inverse
    if (hbd)
      av1_build_compound_diffwtd_mask_highbd(
          tmp_mask[cur_mask_type], cur_mask_type, CONVERT_TO_BYTEPTR(p0), bw,
          CONVERT_TO_BYTEPTR(p1), bw, bh, bw, xd->bd);
    else
      av1_build_compound_diffwtd_mask(tmp_mask[cur_mask_type], cur_mask_type,
                                      p0, bw, p1, bw, bh, bw);

    // compute rd for mask
    uint64_t sse = av1_wedge_sse_from_residuals(residual1, diff10,
                                                tmp_mask[cur_mask_type], N);
    sse = ROUND_POWER_OF_TWO(sse, bd_round);

    model_rd_sse_fn[MODELRD_TYPE_MASKED_COMPOUND](cpi, x, bsize, 0, sse, N,
                                                  &rate, &dist);
    const int64_t rd0 = RDCOST(x->rdmult, rate, dist);

    if (rd0 < best_rd) {
      best_mask_type = cur_mask_type;
      best_rd = rd0;
    }
  }
  mbmi->interinter_comp.mask_type = best_mask_type;
  if (best_mask_type == DIFFWTD_38_INV) {
    memcpy(xd->seg_mask, seg_mask, N * 2);
  }
  return best_rd;
}

static int64_t pick_interintra_wedge(const AV1_COMP *const cpi,
                                     const MACROBLOCK *const x,
                                     const BLOCK_SIZE bsize,
                                     const uint8_t *const p0,
                                     const uint8_t *const p1) {
  const MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  assert(av1_is_wedge_used(bsize));
  assert(cpi->common.seq_params.enable_interintra_compound);

  const struct buf_2d *const src = &x->plane[0].src;
  const int bw = block_size_wide[bsize];
  const int bh = block_size_high[bsize];
  DECLARE_ALIGNED(32, int16_t, residual1[MAX_SB_SQUARE]);  // src - pred1
  DECLARE_ALIGNED(32, int16_t, diff10[MAX_SB_SQUARE]);     // pred1 - pred0
#if CONFIG_AV1_HIGHBITDEPTH
  if (is_cur_buf_hbd(xd)) {
    aom_highbd_subtract_block(bh, bw, residual1, bw, src->buf, src->stride,
                              CONVERT_TO_BYTEPTR(p1), bw, xd->bd);
    aom_highbd_subtract_block(bh, bw, diff10, bw, CONVERT_TO_BYTEPTR(p1), bw,
                              CONVERT_TO_BYTEPTR(p0), bw, xd->bd);
  } else {
    aom_subtract_block(bh, bw, residual1, bw, src->buf, src->stride, p1, bw);
    aom_subtract_block(bh, bw, diff10, bw, p1, bw, p0, bw);
  }
#else
  aom_subtract_block(bh, bw, residual1, bw, src->buf, src->stride, p1, bw);
  aom_subtract_block(bh, bw, diff10, bw, p1, bw, p0, bw);
#endif
  int8_t wedge_index = -1;
  int64_t rd =
      pick_wedge_fixed_sign(cpi, x, bsize, residual1, diff10, 0, &wedge_index);

  mbmi->interintra_wedge_index = wedge_index;
  return rd;
}

static int interinter_compound_motion_search(const AV1_COMP *const cpi,
                                             MACROBLOCK *x,
                                             const int_mv *const cur_mv,
                                             const BLOCK_SIZE bsize,
                                             const PREDICTION_MODE this_mode) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  int_mv tmp_mv[2];
  int tmp_rate_mv = 0;
  mbmi->interinter_comp.seg_mask = xd->seg_mask;
  const INTERINTER_COMPOUND_DATA *compound_data = &mbmi->interinter_comp;

  if (this_mode == NEW_NEWMV) {
    do_masked_motion_search_indexed(cpi, x, cur_mv, compound_data, bsize,
                                    tmp_mv, &tmp_rate_mv, 2);
    mbmi->mv[0].as_int = tmp_mv[0].as_int;
    mbmi->mv[1].as_int = tmp_mv[1].as_int;
  } else if (this_mode >= NEAREST_NEWMV && this_mode <= NEW_NEARMV) {
    // which = 1 if this_mode == NEAREST_NEWMV || this_mode == NEAR_NEWMV
    // which = 0 if this_mode == NEW_NEARESTMV || this_mode == NEW_NEARMV
    int which = (NEWMV == compound_ref1_mode(this_mode));
    do_masked_motion_search_indexed(cpi, x, cur_mv, compound_data, bsize,
                                    tmp_mv, &tmp_rate_mv, which);
    mbmi->mv[which].as_int = tmp_mv[which].as_int;
  }
  return tmp_rate_mv;
}

static AOM_INLINE void get_inter_predictors_masked_compound(
    MACROBLOCK *x, const BLOCK_SIZE bsize, uint8_t **preds0, uint8_t **preds1,
    int16_t *residual1, int16_t *diff10, int *strides) {
  MACROBLOCKD *xd = &x->e_mbd;
  const int bw = block_size_wide[bsize];
  const int bh = block_size_high[bsize];
  // get inter predictors to use for masked compound modes
  av1_build_inter_predictors_for_planes_single_buf(xd, bsize, 0, 0, 0, preds0,
                                                   strides);
  av1_build_inter_predictors_for_planes_single_buf(xd, bsize, 0, 0, 1, preds1,
                                                   strides);
  const struct buf_2d *const src = &x->plane[0].src;
#if CONFIG_AV1_HIGHBITDEPTH
  if (is_cur_buf_hbd(xd)) {
    aom_highbd_subtract_block(bh, bw, residual1, bw, src->buf, src->stride,
                              CONVERT_TO_BYTEPTR(*preds1), bw, xd->bd);
    aom_highbd_subtract_block(bh, bw, diff10, bw, CONVERT_TO_BYTEPTR(*preds1),
                              bw, CONVERT_TO_BYTEPTR(*preds0), bw, xd->bd);
  } else {
    aom_subtract_block(bh, bw, residual1, bw, src->buf, src->stride, *preds1,
                       bw);
    aom_subtract_block(bh, bw, diff10, bw, *preds1, bw, *preds0, bw);
  }
#else
  aom_subtract_block(bh, bw, residual1, bw, src->buf, src->stride, *preds1, bw);
  aom_subtract_block(bh, bw, diff10, bw, *preds1, bw, *preds0, bw);
#endif
}

// Takes a backup of rate, distortion and model_rd for future reuse
static INLINE void backup_stats(COMPOUND_TYPE cur_type, int32_t *comp_rate,
                                int64_t *comp_dist, int32_t *comp_model_rate,
                                int64_t *comp_model_dist, int rate_sum,
                                int64_t dist_sum, RD_STATS *rd_stats,
                                int *comp_rs2, int rs2) {
  comp_rate[cur_type] = rd_stats->rate;
  comp_dist[cur_type] = rd_stats->dist;
  comp_model_rate[cur_type] = rate_sum;
  comp_model_dist[cur_type] = dist_sum;
  comp_rs2[cur_type] = rs2;
}

static int64_t masked_compound_type_rd(
    const AV1_COMP *const cpi, MACROBLOCK *x, const int_mv *const cur_mv,
    const BLOCK_SIZE bsize, const PREDICTION_MODE this_mode, int *rs2,
    int rate_mv, const BUFFER_SET *ctx, int *out_rate_mv, uint8_t **preds0,
    uint8_t **preds1, int16_t *residual1, int16_t *diff10, int *strides,
    int mode_rate, int64_t rd_thresh, int *calc_pred_masked_compound,
    int32_t *comp_rate, int64_t *comp_dist, int32_t *comp_model_rate,
    int64_t *comp_model_dist, const int64_t comp_best_model_rd,
    int64_t *const comp_model_rd_cur, int *comp_rs2) {
  const AV1_COMMON *const cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  int64_t best_rd_cur = INT64_MAX;
  int64_t rd = INT64_MAX;
  const COMPOUND_TYPE compound_type = mbmi->interinter_comp.type;
  // This function will be called only for COMPOUND_WEDGE and COMPOUND_DIFFWTD
  assert(compound_type == COMPOUND_WEDGE || compound_type == COMPOUND_DIFFWTD);
  int rate_sum, tmp_skip_txfm_sb;
  int64_t dist_sum, tmp_skip_sse_sb;
  pick_interinter_mask_type pick_interinter_mask[2] = { pick_interinter_wedge,
                                                        pick_interinter_seg };

  // TODO(any): Save pred and mask calculation as well into records. However
  // this may increase memory requirements as compound segment mask needs to be
  // stored in each record.
  if (*calc_pred_masked_compound) {
    get_inter_predictors_masked_compound(x, bsize, preds0, preds1, residual1,
                                         diff10, strides);
    *calc_pred_masked_compound = 0;
  }
  if (cpi->sf.inter_sf.prune_wedge_pred_diff_based &&
      compound_type == COMPOUND_WEDGE) {
    unsigned int sse;
    if (is_cur_buf_hbd(xd))
      (void)cpi->fn_ptr[bsize].vf(CONVERT_TO_BYTEPTR(*preds0), *strides,
                                  CONVERT_TO_BYTEPTR(*preds1), *strides, &sse);
    else
      (void)cpi->fn_ptr[bsize].vf(*preds0, *strides, *preds1, *strides, &sse);
    const unsigned int mse =
        ROUND_POWER_OF_TWO(sse, num_pels_log2_lookup[bsize]);
    // If two predictors are very similar, skip wedge compound mode search
    if (mse < 8 || (!have_newmv_in_inter_mode(this_mode) && mse < 64)) {
      *comp_model_rd_cur = INT64_MAX;
      return INT64_MAX;
    }
  }
  // Function pointer to pick the appropriate mask
  // compound_type == COMPOUND_WEDGE, calls pick_interinter_wedge()
  // compound_type == COMPOUND_DIFFWTD, calls pick_interinter_seg()
  best_rd_cur = pick_interinter_mask[compound_type - COMPOUND_WEDGE](
      cpi, x, bsize, *preds0, *preds1, residual1, diff10);
  *rs2 += get_interinter_compound_mask_rate(x, mbmi);
  best_rd_cur += RDCOST(x->rdmult, *rs2 + rate_mv, 0);

  // Although the true rate_mv might be different after motion search, but it
  // is unlikely to be the best mode considering the transform rd cost and other
  // mode overhead cost
  int64_t mode_rd = RDCOST(x->rdmult, *rs2 + mode_rate, 0);
  if (mode_rd > rd_thresh) {
    *comp_model_rd_cur = INT64_MAX;
    return INT64_MAX;
  }

  // Compute cost if matching record not found, else, reuse data
  if (comp_rate[compound_type] == INT_MAX) {
    // Check whether new MV search for wedge is to be done
    int wedge_newmv_search =
        have_newmv_in_inter_mode(this_mode) &&
        (compound_type == COMPOUND_WEDGE) &&
        (!cpi->sf.inter_sf.disable_interinter_wedge_newmv_search);
    int diffwtd_newmv_search =
        cpi->sf.inter_sf.enable_interinter_diffwtd_newmv_search &&
        compound_type == COMPOUND_DIFFWTD &&
        have_newmv_in_inter_mode(this_mode);

    // Search for new MV if needed and build predictor
    if (wedge_newmv_search) {
      *out_rate_mv =
          interinter_compound_motion_search(cpi, x, cur_mv, bsize, this_mode);
      const int mi_row = xd->mi_row;
      const int mi_col = xd->mi_col;
      av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, ctx, bsize,
                                    AOM_PLANE_Y, AOM_PLANE_Y);
    } else if (diffwtd_newmv_search) {
      *out_rate_mv =
          interinter_compound_motion_search(cpi, x, cur_mv, bsize, this_mode);
      // we need to update the mask according to the new motion vector
      CompoundTypeRdBuffers tmp_buf;
      int64_t tmp_rd = INT64_MAX;
      alloc_compound_type_rd_buffers_no_check(&tmp_buf);

      uint8_t *tmp_preds0[1] = { tmp_buf.pred0 };
      uint8_t *tmp_preds1[1] = { tmp_buf.pred1 };

      get_inter_predictors_masked_compound(x, bsize, tmp_preds0, tmp_preds1,
                                           tmp_buf.residual1, tmp_buf.diff10,
                                           strides);

      tmp_rd = pick_interinter_mask[compound_type - COMPOUND_WEDGE](
          cpi, x, bsize, *tmp_preds0, *tmp_preds1, tmp_buf.residual1,
          tmp_buf.diff10);
      // we can reuse rs2 here
      tmp_rd += RDCOST(x->rdmult, *rs2 + *out_rate_mv, 0);

      if (tmp_rd >= best_rd_cur) {
        // restore the motion vector
        mbmi->mv[0].as_int = cur_mv[0].as_int;
        mbmi->mv[1].as_int = cur_mv[1].as_int;
        *out_rate_mv = rate_mv;
        av1_build_wedge_inter_predictor_from_buf(xd, bsize, 0, 0, preds0,
                                                 strides, preds1, strides);
      } else {
        // build the final prediciton using the updated mv
        av1_build_wedge_inter_predictor_from_buf(xd, bsize, 0, 0, tmp_preds0,
                                                 strides, tmp_preds1, strides);
      }
      av1_release_compound_type_rd_buffers(&tmp_buf);
    } else {
      *out_rate_mv = rate_mv;
      av1_build_wedge_inter_predictor_from_buf(xd, bsize, 0, 0, preds0, strides,
                                               preds1, strides);
    }
    // Get the RD cost from model RD
    model_rd_sb_fn[MODELRD_TYPE_MASKED_COMPOUND](
        cpi, bsize, x, xd, 0, 0, &rate_sum, &dist_sum, &tmp_skip_txfm_sb,
        &tmp_skip_sse_sb, NULL, NULL, NULL);
    rd = RDCOST(x->rdmult, *rs2 + *out_rate_mv + rate_sum, dist_sum);
    *comp_model_rd_cur = rd;
    // Override with best if current is worse than best for new MV
    if (wedge_newmv_search) {
      if (rd >= best_rd_cur) {
        mbmi->mv[0].as_int = cur_mv[0].as_int;
        mbmi->mv[1].as_int = cur_mv[1].as_int;
        *out_rate_mv = rate_mv;
        av1_build_wedge_inter_predictor_from_buf(xd, bsize, 0, 0, preds0,
                                                 strides, preds1, strides);
        *comp_model_rd_cur = best_rd_cur;
      }
    }
    if (cpi->sf.inter_sf.prune_comp_type_by_model_rd &&
        (*comp_model_rd_cur > comp_best_model_rd) &&
        comp_best_model_rd != INT64_MAX) {
      *comp_model_rd_cur = INT64_MAX;
      return INT64_MAX;
    }
    // Compute RD cost for the current type
    RD_STATS rd_stats;
    const int64_t tmp_mode_rd = RDCOST(x->rdmult, *rs2 + *out_rate_mv, 0);
    const int64_t tmp_rd_thresh = rd_thresh - tmp_mode_rd;
    rd = estimate_yrd_for_sb(cpi, bsize, x, tmp_rd_thresh, &rd_stats);
    if (rd != INT64_MAX) {
      rd =
          RDCOST(x->rdmult, *rs2 + *out_rate_mv + rd_stats.rate, rd_stats.dist);
      // Backup rate and distortion for future reuse
      backup_stats(compound_type, comp_rate, comp_dist, comp_model_rate,
                   comp_model_dist, rate_sum, dist_sum, &rd_stats, comp_rs2,
                   *rs2);
    }
  } else {
    // Reuse data as matching record is found
    assert(comp_dist[compound_type] != INT64_MAX);
    // When disable_interinter_wedge_newmv_search is set, motion refinement is
    // disabled. Hence rate and distortion can be reused in this case as well
    assert(IMPLIES(have_newmv_in_inter_mode(this_mode),
                   cpi->sf.inter_sf.disable_interinter_wedge_newmv_search));
    assert(mbmi->mv[0].as_int == cur_mv[0].as_int);
    assert(mbmi->mv[1].as_int == cur_mv[1].as_int);
    *out_rate_mv = rate_mv;
    // Calculate RD cost based on stored stats
    rd = RDCOST(x->rdmult, *rs2 + *out_rate_mv + comp_rate[compound_type],
                comp_dist[compound_type]);
    // Recalculate model rdcost with the updated rate
    *comp_model_rd_cur =
        RDCOST(x->rdmult, *rs2 + *out_rate_mv + comp_model_rate[compound_type],
               comp_model_dist[compound_type]);
  }
  return rd;
}

#define MAX_INTERP_FILTER_STATS 128
typedef struct {
  int_interpfilters filters;
  int_mv mv[2];
  int8_t ref_frames[2];
  COMPOUND_TYPE comp_type;
  int compound_idx;
  int64_t rd;
  unsigned int pred_sse;
} INTERPOLATION_FILTER_STATS;

typedef struct {
  // OBMC secondary prediction buffers and respective strides
  uint8_t *above_pred_buf[MAX_MB_PLANE];
  int above_pred_stride[MAX_MB_PLANE];
  uint8_t *left_pred_buf[MAX_MB_PLANE];
  int left_pred_stride[MAX_MB_PLANE];
  int_mv (*single_newmv)[REF_FRAMES];
  // Pointer to array of motion vectors to use for each ref and their rates
  // Should point to first of 2 arrays in 2D array
  int (*single_newmv_rate)[REF_FRAMES];
  int (*single_newmv_valid)[REF_FRAMES];
  // Pointer to array of predicted rate-distortion
  // Should point to first of 2 arrays in 2D array
  int64_t (*modelled_rd)[MAX_REF_MV_SEARCH][REF_FRAMES];
  int ref_frame_cost;
  int single_comp_cost;
  int64_t (*simple_rd)[MAX_REF_MV_SEARCH][REF_FRAMES];
  int skip_motion_mode;
  INTERINTRA_MODE *inter_intra_mode;
  int single_ref_first_pass;
  SimpleRDState *simple_rd_state;
  // [comp_idx][saved stat_idx]
  INTERPOLATION_FILTER_STATS interp_filter_stats[MAX_INTERP_FILTER_STATS];
  int interp_filter_stats_idx;
} HandleInterModeArgs;

/* If the current mode shares the same mv with other modes with higher cost,
 * skip this mode. */
static int skip_repeated_mv(const AV1_COMMON *const cm,
                            const MACROBLOCK *const x,
                            PREDICTION_MODE this_mode,
                            const MV_REFERENCE_FRAME ref_frames[2],
                            InterModeSearchState *search_state) {
  const int is_comp_pred = ref_frames[1] > INTRA_FRAME;
  const uint8_t ref_frame_type = av1_ref_frame_type(ref_frames);
  const MB_MODE_INFO_EXT *const mbmi_ext = x->mbmi_ext;
  const int ref_mv_count = mbmi_ext->ref_mv_count[ref_frame_type];
  PREDICTION_MODE compare_mode = MB_MODE_COUNT;
  if (!is_comp_pred) {
    if (this_mode == NEARMV) {
      if (ref_mv_count == 0) {
        // NEARMV has the same motion vector as NEARESTMV
        compare_mode = NEARESTMV;
      }
      if (ref_mv_count == 1 &&
          cm->global_motion[ref_frames[0]].wmtype <= TRANSLATION) {
        // NEARMV has the same motion vector as GLOBALMV
        compare_mode = GLOBALMV;
      }
    }
    if (this_mode == GLOBALMV) {
      if (ref_mv_count == 0 &&
          cm->global_motion[ref_frames[0]].wmtype <= TRANSLATION) {
        // GLOBALMV has the same motion vector as NEARESTMV
        compare_mode = NEARESTMV;
      }
      if (ref_mv_count == 1) {
        // GLOBALMV has the same motion vector as NEARMV
        compare_mode = NEARMV;
      }
    }

    if (compare_mode != MB_MODE_COUNT) {
      // Use modelled_rd to check whether compare mode was searched
      if (search_state->modelled_rd[compare_mode][0][ref_frames[0]] !=
          INT64_MAX) {
        const int16_t mode_ctx =
            av1_mode_context_analyzer(mbmi_ext->mode_context, ref_frames);
        const int compare_cost = cost_mv_ref(x, compare_mode, mode_ctx);
        const int this_cost = cost_mv_ref(x, this_mode, mode_ctx);

        // Only skip if the mode cost is larger than compare mode cost
        if (this_cost > compare_cost) {
          search_state->modelled_rd[this_mode][0][ref_frames[0]] =
              search_state->modelled_rd[compare_mode][0][ref_frames[0]];
          return 1;
        }
      }
    }
  }
  return 0;
}

static INLINE int clamp_and_check_mv(int_mv *out_mv, int_mv in_mv,
                                     const AV1_COMMON *cm,
                                     const MACROBLOCK *x) {
  const MACROBLOCKD *const xd = &x->e_mbd;
  *out_mv = in_mv;
  lower_mv_precision(&out_mv->as_mv, cm->allow_high_precision_mv,
                     cm->cur_frame_force_integer_mv);
  clamp_mv2(&out_mv->as_mv, xd);
  return !mv_check_bounds(&x->mv_limits, &out_mv->as_mv);
}

// To use single newmv directly for compound modes, need to clamp the mv to the
// valid mv range. Without this, encoder would generate out of range mv, and
// this is seen in 8k encoding.
static INLINE void clamp_mv_in_range(MACROBLOCK *const x, int_mv *mv,
                                     int ref_idx) {
  const int_mv ref_mv = av1_get_ref_mv(x, ref_idx);
  int minc, maxc, minr, maxr;
  set_subpel_mv_search_range(&x->mv_limits, &minc, &maxc, &minr, &maxr,
                             &ref_mv.as_mv);
  clamp_mv(&mv->as_mv, minc, maxc, minr, maxr);
}

static int64_t handle_newmv(const AV1_COMP *const cpi, MACROBLOCK *const x,
                            const BLOCK_SIZE bsize, int_mv *cur_mv,
                            int *const rate_mv,
                            HandleInterModeArgs *const args) {
  const MACROBLOCKD *const xd = &x->e_mbd;
  const MB_MODE_INFO *const mbmi = xd->mi[0];
  const int is_comp_pred = has_second_ref(mbmi);
  const PREDICTION_MODE this_mode = mbmi->mode;
  const int refs[2] = { mbmi->ref_frame[0],
                        mbmi->ref_frame[1] < 0 ? 0 : mbmi->ref_frame[1] };
  const int ref_mv_idx = mbmi->ref_mv_idx;

  if (is_comp_pred) {
    const int valid_mv0 = args->single_newmv_valid[ref_mv_idx][refs[0]];
    const int valid_mv1 = args->single_newmv_valid[ref_mv_idx][refs[1]];

    if (this_mode == NEW_NEWMV) {
      if (valid_mv0) {
        cur_mv[0].as_int = args->single_newmv[ref_mv_idx][refs[0]].as_int;
        clamp_mv_in_range(x, &cur_mv[0], 0);
      }
      if (valid_mv1) {
        cur_mv[1].as_int = args->single_newmv[ref_mv_idx][refs[1]].as_int;
        clamp_mv_in_range(x, &cur_mv[1], 1);
      }

      // aomenc1
      if (cpi->sf.inter_sf.comp_inter_joint_search_thresh <= bsize ||
          !valid_mv0 || !valid_mv1) {
        joint_motion_search(cpi, x, bsize, cur_mv, NULL, 0, rate_mv);
      } else {
        *rate_mv = 0;
        for (int i = 0; i < 2; ++i) {
          const int_mv ref_mv = av1_get_ref_mv(x, i);
          *rate_mv +=
              av1_mv_bit_cost(&cur_mv[i].as_mv, &ref_mv.as_mv, x->nmv_vec_cost,
                              x->mv_cost_stack, MV_COST_WEIGHT);
        }
      }
    } else if (this_mode == NEAREST_NEWMV || this_mode == NEAR_NEWMV) {
      if (valid_mv1) {
        cur_mv[1].as_int = args->single_newmv[ref_mv_idx][refs[1]].as_int;
        clamp_mv_in_range(x, &cur_mv[1], 1);
      }

      // aomenc2
      if (cpi->sf.inter_sf.comp_inter_joint_search_thresh <= bsize ||
          !valid_mv1) {
        compound_single_motion_search_interinter(cpi, x, bsize, cur_mv, NULL, 0,
                                                 rate_mv, 1);
      } else {
        const int_mv ref_mv = av1_get_ref_mv(x, 1);
        *rate_mv =
            av1_mv_bit_cost(&cur_mv[1].as_mv, &ref_mv.as_mv, x->nmv_vec_cost,
                            x->mv_cost_stack, MV_COST_WEIGHT);
      }
    } else {
      assert(this_mode == NEW_NEARESTMV || this_mode == NEW_NEARMV);
      if (valid_mv0) {
        cur_mv[0].as_int = args->single_newmv[ref_mv_idx][refs[0]].as_int;
        clamp_mv_in_range(x, &cur_mv[0], 0);
      }

      // aomenc3
      if (cpi->sf.inter_sf.comp_inter_joint_search_thresh <= bsize ||
          !valid_mv0) {
        compound_single_motion_search_interinter(cpi, x, bsize, cur_mv, NULL, 0,
                                                 rate_mv, 0);
      } else {
        const int_mv ref_mv = av1_get_ref_mv(x, 0);
        *rate_mv =
            av1_mv_bit_cost(&cur_mv[0].as_mv, &ref_mv.as_mv, x->nmv_vec_cost,
                            x->mv_cost_stack, MV_COST_WEIGHT);
      }
    }
  } else {
    single_motion_search(cpi, x, bsize, 0, rate_mv);
    if (x->best_mv.as_int == INVALID_MV) return INT64_MAX;

    args->single_newmv[ref_mv_idx][refs[0]] = x->best_mv;
    args->single_newmv_rate[ref_mv_idx][refs[0]] = *rate_mv;
    args->single_newmv_valid[ref_mv_idx][refs[0]] = 1;

    cur_mv[0].as_int = x->best_mv.as_int;
  }

  return 0;
}

static INLINE void swap_dst_buf(MACROBLOCKD *xd, const BUFFER_SET *dst_bufs[2],
                                int num_planes) {
  const BUFFER_SET *buf0 = dst_bufs[0];
  dst_bufs[0] = dst_bufs[1];
  dst_bufs[1] = buf0;
  restore_dst_buf(xd, *dst_bufs[0], num_planes);
}

static INLINE int get_switchable_rate(MACROBLOCK *const x,
                                      const int_interpfilters filters,
                                      const int ctx[2]) {
  int inter_filter_cost;
  const InterpFilter filter0 = filters.as_filters.y_filter;
  const InterpFilter filter1 = filters.as_filters.x_filter;
  inter_filter_cost = x->switchable_interp_costs[ctx[0]][filter0];
  inter_filter_cost += x->switchable_interp_costs[ctx[1]][filter1];
  return SWITCHABLE_INTERP_RATE_FACTOR * inter_filter_cost;
}

// Build inter predictor and calculate model rd
// for a given plane.
static INLINE void interp_model_rd_eval(
    MACROBLOCK *const x, const AV1_COMP *const cpi, BLOCK_SIZE bsize,
    const BUFFER_SET *const orig_dst, int plane_from, int plane_to,
    RD_STATS *rd_stats, int is_skip_build_pred) {
  const AV1_COMMON *cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  RD_STATS tmp_rd_stats;
  av1_init_rd_stats(&tmp_rd_stats);

  // Skip inter predictor if the predictor is already avilable.
  if (!is_skip_build_pred) {
    const int mi_row = xd->mi_row;
    const int mi_col = xd->mi_col;
    av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, orig_dst, bsize,
                                  plane_from, plane_to);
  }

  model_rd_sb_fn[cpi->sf.rt_sf.use_simple_rd_model
                     ? MODELRD_LEGACY
                     : MODELRD_TYPE_INTERP_FILTER](
      cpi, bsize, x, xd, plane_from, plane_to, &tmp_rd_stats.rate,
      &tmp_rd_stats.dist, &tmp_rd_stats.skip, &tmp_rd_stats.sse, NULL, NULL,
      NULL);

  av1_merge_rd_stats(rd_stats, &tmp_rd_stats);
}

// calculate the rdcost of given interpolation_filter
static INLINE int64_t interpolation_filter_rd(
    MACROBLOCK *const x, const AV1_COMP *const cpi,
    const TileDataEnc *tile_data, BLOCK_SIZE bsize,
    const BUFFER_SET *const orig_dst, int64_t *const rd,
    RD_STATS *rd_stats_luma, RD_STATS *rd_stats, int *const switchable_rate,
    const BUFFER_SET *dst_bufs[2], int filter_idx, const int switchable_ctx[2],
    const int skip_pred) {
  const AV1_COMMON *cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  RD_STATS this_rd_stats_luma, this_rd_stats;

  // Initialize rd_stats structures to default values.
  av1_init_rd_stats(&this_rd_stats_luma);
  this_rd_stats = *rd_stats_luma;
  const int_interpfilters last_best = mbmi->interp_filters;
  mbmi->interp_filters = filter_sets[filter_idx];
  const int tmp_rs =
      get_switchable_rate(x, mbmi->interp_filters, switchable_ctx);

  int64_t min_rd = RDCOST(x->rdmult, tmp_rs, 0);
  if (min_rd > *rd) {
    mbmi->interp_filters = last_best;
    return 0;
  }

  (void)tile_data;

  assert(skip_pred != 2);
  assert((rd_stats_luma->rate >= 0) && (rd_stats->rate >= 0));
  assert((rd_stats_luma->dist >= 0) && (rd_stats->dist >= 0));
  assert((rd_stats_luma->sse >= 0) && (rd_stats->sse >= 0));
  assert((rd_stats_luma->skip == 0) || (rd_stats_luma->skip == 1));
  assert((rd_stats->skip == 0) || (rd_stats->skip == 1));
  assert((skip_pred >= 0) && (skip_pred <= cpi->default_interp_skip_flags));

  // When skip pred is equal to default_interp_skip_flags,
  // skip both luma and chroma MC.
  // For mono-chrome images:
  // num_planes = 1 and cpi->default_interp_skip_flags = 1,
  // skip_pred = 1: skip both luma and chroma
  // skip_pred = 0: Evaluate luma and as num_planes=1,
  // skip chroma evaluation
  int tmp_skip_pred = (skip_pred == cpi->default_interp_skip_flags)
                          ? INTERP_SKIP_LUMA_SKIP_CHROMA
                          : skip_pred;

  switch (tmp_skip_pred) {
    case INTERP_EVAL_LUMA_EVAL_CHROMA:
      // skip_pred = 0: Evaluate both luma and chroma.
      // Luma MC
      interp_model_rd_eval(x, cpi, bsize, orig_dst, AOM_PLANE_Y, AOM_PLANE_Y,
                           &this_rd_stats_luma, 0);
      this_rd_stats = this_rd_stats_luma;
#if CONFIG_COLLECT_RD_STATS == 3
      RD_STATS rd_stats_y;
      pick_tx_size_type_yrd(cpi, x, &rd_stats_y, bsize, INT64_MAX);
      PrintPredictionUnitStats(cpi, tile_data, x, &rd_stats_y, bsize);
#endif  // CONFIG_COLLECT_RD_STATS == 3
      AOM_FALLTHROUGH_INTENDED;
    case INTERP_SKIP_LUMA_EVAL_CHROMA:
      // skip_pred = 1: skip luma evaluation (retain previous best luma stats)
      // and do chroma evaluation.
      for (int plane = 1; plane < num_planes; ++plane) {
        int64_t tmp_rd =
            RDCOST(x->rdmult, tmp_rs + this_rd_stats.rate, this_rd_stats.dist);
        if (tmp_rd >= *rd) {
          mbmi->interp_filters = last_best;
          return 0;
        }
        interp_model_rd_eval(x, cpi, bsize, orig_dst, plane, plane,
                             &this_rd_stats, 0);
      }
      break;
    case INTERP_SKIP_LUMA_SKIP_CHROMA:
      // both luma and chroma evaluation is skipped
      this_rd_stats = *rd_stats;
      break;
    case INTERP_EVAL_INVALID:
    default: assert(0); return 0;
  }
  int64_t tmp_rd =
      RDCOST(x->rdmult, tmp_rs + this_rd_stats.rate, this_rd_stats.dist);

  if (tmp_rd < *rd) {
    *rd = tmp_rd;
    *switchable_rate = tmp_rs;
    if (skip_pred != cpi->default_interp_skip_flags) {
      if (skip_pred == INTERP_EVAL_LUMA_EVAL_CHROMA) {
        // Overwrite the data as current filter is the best one
        *rd_stats_luma = this_rd_stats_luma;
        *rd_stats = this_rd_stats;
        // As luma MC data is computed, no need to recompute after the search
        x->recalc_luma_mc_data = 0;
      } else if (skip_pred == INTERP_SKIP_LUMA_EVAL_CHROMA) {
        // As luma MC data is not computed, update of luma data can be skipped
        *rd_stats = this_rd_stats;
        // As luma MC data is not recomputed and current filter is the best,
        // indicate the possibility of recomputing MC data
        // If current buffer contains valid MC data, toggle to indicate that
        // luma MC data needs to be recomputed
        x->recalc_luma_mc_data ^= 1;
      }
      swap_dst_buf(xd, dst_bufs, num_planes);
    }
    return 1;
  }
  mbmi->interp_filters = last_best;
  return 0;
}

static INLINE INTERP_PRED_TYPE is_pred_filter_search_allowed(
    const AV1_COMP *const cpi, MACROBLOCKD *xd, BLOCK_SIZE bsize,
    int_interpfilters *af, int_interpfilters *lf) {
  const AV1_COMMON *cm = &cpi->common;
  const MB_MODE_INFO *const above_mbmi = xd->above_mbmi;
  const MB_MODE_INFO *const left_mbmi = xd->left_mbmi;
  const int bsl = mi_size_wide_log2[bsize];
  int is_horiz_eq = 0, is_vert_eq = 0;

  if (above_mbmi && is_inter_block(above_mbmi))
    *af = above_mbmi->interp_filters;

  if (left_mbmi && is_inter_block(left_mbmi)) *lf = left_mbmi->interp_filters;

  if (af->as_filters.x_filter != INTERP_INVALID)
    is_horiz_eq = af->as_filters.x_filter == lf->as_filters.x_filter;
  if (af->as_filters.y_filter != INTERP_INVALID)
    is_vert_eq = af->as_filters.y_filter == lf->as_filters.y_filter;

  INTERP_PRED_TYPE pred_filter_type = (is_vert_eq << 1) + is_horiz_eq;
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  int pred_filter_enable =
      cpi->sf.interp_sf.cb_pred_filter_search
          ? (((mi_row + mi_col) >> bsl) +
             get_chessboard_index(cm->current_frame.frame_number)) &
                0x1
          : 0;
  pred_filter_enable &= is_horiz_eq || is_vert_eq;
  // pred_filter_search = 0: pred_filter is disabled
  // pred_filter_search = 1: pred_filter is enabled and only horz pred matching
  // pred_filter_search = 2: pred_filter is enabled and only vert pred matching
  // pred_filter_search = 3: pred_filter is enabled and
  //                         both vert, horz pred matching
  return pred_filter_enable * pred_filter_type;
}

static DUAL_FILTER_TYPE find_best_interp_rd_facade(
    MACROBLOCK *const x, const AV1_COMP *const cpi,
    const TileDataEnc *tile_data, BLOCK_SIZE bsize,
    const BUFFER_SET *const orig_dst, int64_t *const rd, RD_STATS *rd_stats_y,
    RD_STATS *rd_stats, int *const switchable_rate,
    const BUFFER_SET *dst_bufs[2], const int switchable_ctx[2],
    const int skip_pred, uint16_t allow_interp_mask, int is_w4_or_h4) {
  int tmp_skip_pred = skip_pred;
  DUAL_FILTER_TYPE best_filt_type = REG_REG;

  // If no filter are set to be evaluated, return from function
  if (allow_interp_mask == 0x0) return best_filt_type;
  // For block width or height is 4, skip the pred evaluation of SHARP_SHARP
  tmp_skip_pred = is_w4_or_h4 ? cpi->default_interp_skip_flags : skip_pred;

  // Loop over the all filter types and evaluate for only allowed filter types
  for (int filt_type = SHARP_SHARP; filt_type >= REG_REG; --filt_type) {
    const int is_filter_allowed =
        get_interp_filter_allowed_mask(allow_interp_mask, filt_type);
    if (is_filter_allowed)
      if (interpolation_filter_rd(x, cpi, tile_data, bsize, orig_dst, rd,
                                  rd_stats_y, rd_stats, switchable_rate,
                                  dst_bufs, filt_type, switchable_ctx,
                                  tmp_skip_pred))
        best_filt_type = filt_type;
    tmp_skip_pred = skip_pred;
  }
  return best_filt_type;
}

static INLINE void pred_dual_interp_filter_rd(
    MACROBLOCK *const x, const AV1_COMP *const cpi,
    const TileDataEnc *tile_data, BLOCK_SIZE bsize,
    const BUFFER_SET *const orig_dst, int64_t *const rd, RD_STATS *rd_stats_y,
    RD_STATS *rd_stats, int *const switchable_rate,
    const BUFFER_SET *dst_bufs[2], const int switchable_ctx[2],
    const int skip_pred, INTERP_PRED_TYPE pred_filt_type, int_interpfilters *af,
    int_interpfilters *lf) {
  (void)lf;
  assert(pred_filt_type > INTERP_HORZ_NEQ_VERT_NEQ);
  assert(pred_filt_type < INTERP_PRED_TYPE_ALL);
  uint16_t allowed_interp_mask = 0;

  if (pred_filt_type == INTERP_HORZ_EQ_VERT_NEQ) {
    // pred_filter_search = 1: Only horizontal filter is matching
    allowed_interp_mask =
        av1_interp_dual_filt_mask[pred_filt_type - 1][af->as_filters.x_filter];
  } else if (pred_filt_type == INTERP_HORZ_NEQ_VERT_EQ) {
    // pred_filter_search = 2: Only vertical filter is matching
    allowed_interp_mask =
        av1_interp_dual_filt_mask[pred_filt_type - 1][af->as_filters.y_filter];
  } else {
    // pred_filter_search = 3: Both horizontal and vertical filter are matching
    int filt_type =
        af->as_filters.x_filter + af->as_filters.y_filter * SWITCHABLE_FILTERS;
    set_interp_filter_allowed_mask(&allowed_interp_mask, filt_type);
  }
  // REG_REG is already been evaluated in the beginning
  reset_interp_filter_allowed_mask(&allowed_interp_mask, REG_REG);
  find_best_interp_rd_facade(x, cpi, tile_data, bsize, orig_dst, rd, rd_stats_y,
                             rd_stats, switchable_rate, dst_bufs,
                             switchable_ctx, skip_pred, allowed_interp_mask, 0);
}
// Evaluate dual filter type
// a) Using above, left block interp filter
// b) Find the best horizontal filter and
//    then evaluate corresponding vertical filters.
static INLINE void fast_dual_interp_filter_rd(
    MACROBLOCK *const x, const AV1_COMP *const cpi,
    const TileDataEnc *tile_data, BLOCK_SIZE bsize,
    const BUFFER_SET *const orig_dst, int64_t *const rd, RD_STATS *rd_stats_y,
    RD_STATS *rd_stats, int *const switchable_rate,
    const BUFFER_SET *dst_bufs[2], const int switchable_ctx[2],
    const int skip_hor, const int skip_ver) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  INTERP_PRED_TYPE pred_filter_type = INTERP_HORZ_NEQ_VERT_NEQ;
  int_interpfilters af = av1_broadcast_interp_filter(INTERP_INVALID);
  int_interpfilters lf = af;

  if (!have_newmv_in_inter_mode(mbmi->mode)) {
    pred_filter_type = is_pred_filter_search_allowed(cpi, xd, bsize, &af, &lf);
  }

  if (pred_filter_type) {
    pred_dual_interp_filter_rd(x, cpi, tile_data, bsize, orig_dst, rd,
                               rd_stats_y, rd_stats, switchable_rate, dst_bufs,
                               switchable_ctx, (skip_hor & skip_ver),
                               pred_filter_type, &af, &lf);
  } else {
    const int bw = block_size_wide[bsize];
    const int bh = block_size_high[bsize];
    int best_dual_mode = 0;
    int skip_pred = bw <= 4 ? cpi->default_interp_skip_flags : skip_hor;
    // TODO(any): Make use of find_best_interp_rd_facade()
    // if speed impact is negligible
    for (int i = (SWITCHABLE_FILTERS - 1); i >= 1; --i) {
      if (interpolation_filter_rd(x, cpi, tile_data, bsize, orig_dst, rd,
                                  rd_stats_y, rd_stats, switchable_rate,
                                  dst_bufs, i, switchable_ctx, skip_pred)) {
        best_dual_mode = i;
      }
      skip_pred = skip_hor;
    }
    // From best of horizontal EIGHTTAP_REGULAR modes, check vertical modes
    skip_pred = bh <= 4 ? cpi->default_interp_skip_flags : skip_ver;
    for (int i = (best_dual_mode + (SWITCHABLE_FILTERS * 2));
         i >= (best_dual_mode + SWITCHABLE_FILTERS); i -= SWITCHABLE_FILTERS) {
      interpolation_filter_rd(x, cpi, tile_data, bsize, orig_dst, rd,
                              rd_stats_y, rd_stats, switchable_rate, dst_bufs,
                              i, switchable_ctx, skip_pred);
      skip_pred = skip_ver;
    }
  }
}

// Find the best interp filter if dual_interp_filter = 0
static INLINE void find_best_non_dual_interp_filter(
    MACROBLOCK *const x, const AV1_COMP *const cpi,
    const TileDataEnc *tile_data, BLOCK_SIZE bsize,
    const BUFFER_SET *const orig_dst, int64_t *const rd, RD_STATS *rd_stats_y,
    RD_STATS *rd_stats, int *const switchable_rate,
    const BUFFER_SET *dst_bufs[2], const int switchable_ctx[2],
    const int skip_ver, const int skip_hor) {
  int8_t i;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];

  // Regular filter evaluation should have been done and hence the same should
  // be the winner
  assert(x->e_mbd.mi[0]->interp_filters.as_int == filter_sets[0].as_int);
  if ((skip_hor & skip_ver) != cpi->default_interp_skip_flags) {
    INTERP_PRED_TYPE pred_filter_type = INTERP_HORZ_NEQ_VERT_NEQ;
    int_interpfilters af = av1_broadcast_interp_filter(INTERP_INVALID);
    int_interpfilters lf = af;

    pred_filter_type = is_pred_filter_search_allowed(cpi, xd, bsize, &af, &lf);
    if (pred_filter_type) {
      assert(af.as_filters.x_filter != INTERP_INVALID);
      int filter_idx = SWITCHABLE * af.as_filters.x_filter;
      // This assert tells that (filter_x == filter_y) for non-dual filter case
      assert(filter_sets[filter_idx].as_filters.x_filter ==
             filter_sets[filter_idx].as_filters.y_filter);
      if (cpi->sf.interp_sf.adaptive_interp_filter_search &&
          !(get_interp_filter_allowed_mask(cpi->interp_filter_search_mask,
                                           filter_idx))) {
        return;
      }
      if (filter_idx) {
        interpolation_filter_rd(x, cpi, tile_data, bsize, orig_dst, rd,
                                rd_stats_y, rd_stats, switchable_rate, dst_bufs,
                                filter_idx, switchable_ctx,
                                (skip_hor & skip_ver));
      }
      return;
    }
  }
  // Reuse regular filter's modeled rd data for sharp filter for following
  // cases
  // 1) When bsize is 4x4
  // 2) When block width is 4 (i.e. 4x8/4x16 blocks) and MV in vertical
  // direction is full-pel
  // 3) When block height is 4 (i.e. 8x4/16x4 blocks) and MV in horizontal
  // direction is full-pel
  // TODO(any): Optimize cases 2 and 3 further if luma MV in relavant direction
  // alone is full-pel

  if ((bsize == BLOCK_4X4) ||
      (block_size_wide[bsize] == 4 &&
       skip_ver == cpi->default_interp_skip_flags) ||
      (block_size_high[bsize] == 4 &&
       skip_hor == cpi->default_interp_skip_flags)) {
    int skip_pred = skip_hor & skip_ver;
    uint16_t allowed_interp_mask = 0;

    // REG_REG filter type is evaluated beforehand, hence skip it
    set_interp_filter_allowed_mask(&allowed_interp_mask, SHARP_SHARP);
    set_interp_filter_allowed_mask(&allowed_interp_mask, SMOOTH_SMOOTH);
    if (cpi->sf.interp_sf.adaptive_interp_filter_search)
      allowed_interp_mask &= cpi->interp_filter_search_mask;

    find_best_interp_rd_facade(x, cpi, tile_data, bsize, orig_dst, rd,
                               rd_stats_y, rd_stats, switchable_rate, dst_bufs,
                               switchable_ctx, skip_pred, allowed_interp_mask,
                               1);
  } else {
    int skip_pred = (skip_hor & skip_ver);
    for (i = (SWITCHABLE_FILTERS + 1); i < DUAL_FILTER_SET_SIZE;
         i += (SWITCHABLE_FILTERS + 1)) {
      // This assert tells that (filter_x == filter_y) for non-dual filter case
      assert(filter_sets[i].as_filters.x_filter ==
             filter_sets[i].as_filters.y_filter);
      if (cpi->sf.interp_sf.adaptive_interp_filter_search &&
          !(get_interp_filter_allowed_mask(cpi->interp_filter_search_mask,
                                           i))) {
        continue;
      }
      interpolation_filter_rd(x, cpi, tile_data, bsize, orig_dst, rd,
                              rd_stats_y, rd_stats, switchable_rate, dst_bufs,
                              i, switchable_ctx, skip_pred);
      // In first iteration, smooth filter is evaluated. If smooth filter
      // (which is less sharper) is the winner among regular and smooth filters,
      // sharp filter evaluation is skipped
      // TODO(any): Refine this gating based on modelled rd only (i.e., by not
      // accounting switchable filter rate)
      if (cpi->sf.interp_sf.skip_sharp_interp_filter_search &&
          skip_pred != cpi->default_interp_skip_flags) {
        if (mbmi->interp_filters.as_int == filter_sets[SMOOTH_SMOOTH].as_int)
          break;
      }
    }
  }
}

// return mv_diff
static INLINE int is_interp_filter_good_match(
    const INTERPOLATION_FILTER_STATS *st, MB_MODE_INFO *const mi,
    int skip_level) {
  const int is_comp = has_second_ref(mi);
  int i;

  for (i = 0; i < 1 + is_comp; ++i) {
    if (st->ref_frames[i] != mi->ref_frame[i]) return INT_MAX;
  }

  if (skip_level == 1 && is_comp) {
    if (st->comp_type != mi->interinter_comp.type) return INT_MAX;
    if (st->compound_idx != mi->compound_idx) return INT_MAX;
  }

  int mv_diff = 0;
  for (i = 0; i < 1 + is_comp; ++i) {
    mv_diff += abs(st->mv[i].as_mv.row - mi->mv[i].as_mv.row) +
               abs(st->mv[i].as_mv.col - mi->mv[i].as_mv.col);
  }
  return mv_diff;
}

// Checks if characteristics of search match
static INLINE int is_comp_rd_match(const AV1_COMP *const cpi,
                                   const MACROBLOCK *const x,
                                   const COMP_RD_STATS *st,
                                   const MB_MODE_INFO *const mi,
                                   int32_t *comp_rate, int64_t *comp_dist,
                                   int32_t *comp_model_rate,
                                   int64_t *comp_model_dist, int *comp_rs2) {
  // TODO(ranjit): Ensure that compound type search use regular filter always
  // and check if following check can be removed
  // Check if interp filter matches with previous case
  if (st->filter.as_int != mi->interp_filters.as_int) return 0;

  const MACROBLOCKD *const xd = &x->e_mbd;
  // Match MV and reference indices
  for (int i = 0; i < 2; ++i) {
    if ((st->ref_frames[i] != mi->ref_frame[i]) ||
        (st->mv[i].as_int != mi->mv[i].as_int)) {
      return 0;
    }
    const WarpedMotionParams *const wm = &xd->global_motion[mi->ref_frame[i]];
    if (is_global_mv_block(mi, wm->wmtype) != st->is_global[i]) return 0;
  }

  // Store the stats for COMPOUND_AVERAGE and COMPOUND_DISTWTD
  for (int comp_type = COMPOUND_AVERAGE; comp_type <= COMPOUND_DISTWTD;
       comp_type++) {
    comp_rate[comp_type] = st->rate[comp_type];
    comp_dist[comp_type] = st->dist[comp_type];
    comp_model_rate[comp_type] = st->model_rate[comp_type];
    comp_model_dist[comp_type] = st->model_dist[comp_type];
    comp_rs2[comp_type] = st->comp_rs2[comp_type];
  }

  // For compound wedge/segment, reuse data only if NEWMV is not present in
  // either of the directions
  if ((!have_newmv_in_inter_mode(mi->mode) &&
       !have_newmv_in_inter_mode(st->mode)) ||
      (cpi->sf.inter_sf.disable_interinter_wedge_newmv_search)) {
    memcpy(&comp_rate[COMPOUND_WEDGE], &st->rate[COMPOUND_WEDGE],
           sizeof(comp_rate[COMPOUND_WEDGE]) * 2);
    memcpy(&comp_dist[COMPOUND_WEDGE], &st->dist[COMPOUND_WEDGE],
           sizeof(comp_dist[COMPOUND_WEDGE]) * 2);
    memcpy(&comp_model_rate[COMPOUND_WEDGE], &st->model_rate[COMPOUND_WEDGE],
           sizeof(comp_model_rate[COMPOUND_WEDGE]) * 2);
    memcpy(&comp_model_dist[COMPOUND_WEDGE], &st->model_dist[COMPOUND_WEDGE],
           sizeof(comp_model_dist[COMPOUND_WEDGE]) * 2);
    memcpy(&comp_rs2[COMPOUND_WEDGE], &st->comp_rs2[COMPOUND_WEDGE],
           sizeof(comp_rs2[COMPOUND_WEDGE]) * 2);
  }
  return 1;
}

static INLINE int find_interp_filter_in_stats(
    MB_MODE_INFO *const mbmi, INTERPOLATION_FILTER_STATS *interp_filter_stats,
    int interp_filter_stats_idx, int skip_level) {
  // [skip_levels][single or comp]
  const int thr[2][2] = { { 0, 0 }, { 3, 7 } };
  const int is_comp = has_second_ref(mbmi);

  // Find good enough match.
  // TODO(yunqing): Separate single-ref mode and comp mode stats for fast
  // search.
  int best = INT_MAX;
  int match = -1;
  for (int j = 0; j < interp_filter_stats_idx; ++j) {
    const INTERPOLATION_FILTER_STATS *st = &interp_filter_stats[j];
    const int mv_diff = is_interp_filter_good_match(st, mbmi, skip_level);
    // Exact match is found.
    if (mv_diff == 0) {
      match = j;
      break;
    } else if (mv_diff < best && mv_diff <= thr[skip_level - 1][is_comp]) {
      best = mv_diff;
      match = j;
    }
  }

  if (match != -1) {
    mbmi->interp_filters = interp_filter_stats[match].filters;
    return match;
  }
  return -1;  // no match result found
}
// Checks if similar compound type search case is accounted earlier
// If found, returns relevant rd data
static INLINE int find_comp_rd_in_stats(const AV1_COMP *const cpi,
                                        const MACROBLOCK *x,
                                        const MB_MODE_INFO *const mbmi,
                                        int32_t *comp_rate, int64_t *comp_dist,
                                        int32_t *comp_model_rate,
                                        int64_t *comp_model_dist, int *comp_rs2,
                                        int *match_index) {
  for (int j = 0; j < x->comp_rd_stats_idx; ++j) {
    if (is_comp_rd_match(cpi, x, &x->comp_rd_stats[j], mbmi, comp_rate,
                         comp_dist, comp_model_rate, comp_model_dist,
                         comp_rs2)) {
      *match_index = j;
      return 1;
    }
  }
  return 0;  // no match result found
}

static INLINE int save_interp_filter_search_stat(
    MB_MODE_INFO *const mbmi, int64_t rd, unsigned int pred_sse,
    INTERPOLATION_FILTER_STATS *interp_filter_stats,
    int interp_filter_stats_idx) {
  if (interp_filter_stats_idx < MAX_INTERP_FILTER_STATS) {
    INTERPOLATION_FILTER_STATS stat = { mbmi->interp_filters,
                                        { mbmi->mv[0], mbmi->mv[1] },
                                        { mbmi->ref_frame[0],
                                          mbmi->ref_frame[1] },
                                        mbmi->interinter_comp.type,
                                        mbmi->compound_idx,
                                        rd,
                                        pred_sse };
    interp_filter_stats[interp_filter_stats_idx] = stat;
    interp_filter_stats_idx++;
  }
  return interp_filter_stats_idx;
}

static INLINE void save_comp_rd_search_stat(
    MACROBLOCK *x, const MB_MODE_INFO *const mbmi, const int32_t *comp_rate,
    const int64_t *comp_dist, const int32_t *comp_model_rate,
    const int64_t *comp_model_dist, const int_mv *cur_mv, const int *comp_rs2) {
  const int offset = x->comp_rd_stats_idx;
  if (offset < MAX_COMP_RD_STATS) {
    COMP_RD_STATS *const rd_stats = x->comp_rd_stats + offset;
    memcpy(rd_stats->rate, comp_rate, sizeof(rd_stats->rate));
    memcpy(rd_stats->dist, comp_dist, sizeof(rd_stats->dist));
    memcpy(rd_stats->model_rate, comp_model_rate, sizeof(rd_stats->model_rate));
    memcpy(rd_stats->model_dist, comp_model_dist, sizeof(rd_stats->model_dist));
    memcpy(rd_stats->comp_rs2, comp_rs2, sizeof(rd_stats->comp_rs2));
    memcpy(rd_stats->mv, cur_mv, sizeof(rd_stats->mv));
    memcpy(rd_stats->ref_frames, mbmi->ref_frame, sizeof(rd_stats->ref_frames));
    rd_stats->mode = mbmi->mode;
    rd_stats->filter = mbmi->interp_filters;
    rd_stats->ref_mv_idx = mbmi->ref_mv_idx;
    const MACROBLOCKD *const xd = &x->e_mbd;
    for (int i = 0; i < 2; ++i) {
      const WarpedMotionParams *const wm =
          &xd->global_motion[mbmi->ref_frame[i]];
      rd_stats->is_global[i] = is_global_mv_block(mbmi, wm->wmtype);
    }
    memcpy(&rd_stats->interinter_comp, &mbmi->interinter_comp,
           sizeof(rd_stats->interinter_comp));
    ++x->comp_rd_stats_idx;
  }
}

static INLINE int find_interp_filter_match(
    MB_MODE_INFO *const mbmi, const AV1_COMP *const cpi,
    const InterpFilter assign_filter, const int need_search,
    INTERPOLATION_FILTER_STATS *interp_filter_stats,
    int interp_filter_stats_idx) {
  int match_found_idx = -1;
  if (cpi->sf.interp_sf.use_interp_filter && need_search)
    match_found_idx = find_interp_filter_in_stats(
        mbmi, interp_filter_stats, interp_filter_stats_idx,
        cpi->sf.interp_sf.use_interp_filter);

  if (!need_search || match_found_idx == -1)
    set_default_interp_filters(mbmi, assign_filter);
  return match_found_idx;
}

static INLINE void calc_interp_skip_pred_flag(MACROBLOCK *const x,
                                              const AV1_COMP *const cpi,
                                              int *skip_hor, int *skip_ver) {
  const AV1_COMMON *cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const int num_planes = av1_num_planes(cm);
  const int is_compound = has_second_ref(mbmi);
  assert(is_intrabc_block(mbmi) == 0);
  for (int ref = 0; ref < 1 + is_compound; ++ref) {
    const struct scale_factors *const sf =
        get_ref_scale_factors_const(cm, mbmi->ref_frame[ref]);
    // TODO(any): Refine skip flag calculation considering scaling
    if (av1_is_scaled(sf)) {
      *skip_hor = 0;
      *skip_ver = 0;
      break;
    }
    const MV mv = mbmi->mv[ref].as_mv;
    int skip_hor_plane = 0;
    int skip_ver_plane = 0;
    for (int plane_idx = 0; plane_idx < AOMMAX(1, (num_planes - 1));
         ++plane_idx) {
      struct macroblockd_plane *const pd = &xd->plane[plane_idx];
      const int bw = pd->width;
      const int bh = pd->height;
      const MV mv_q4 = clamp_mv_to_umv_border_sb(
          xd, &mv, bw, bh, pd->subsampling_x, pd->subsampling_y);
      const int sub_x = (mv_q4.col & SUBPEL_MASK) << SCALE_EXTRA_BITS;
      const int sub_y = (mv_q4.row & SUBPEL_MASK) << SCALE_EXTRA_BITS;
      skip_hor_plane |= ((sub_x == 0) << plane_idx);
      skip_ver_plane |= ((sub_y == 0) << plane_idx);
    }
    *skip_hor &= skip_hor_plane;
    *skip_ver &= skip_ver_plane;
    // It is not valid that "luma MV is sub-pel, whereas chroma MV is not"
    assert(*skip_hor != 2);
    assert(*skip_ver != 2);
  }
  // When compond prediction type is compound segment wedge, luma MC and chroma
  // MC need to go hand in hand as mask generated during luma MC is reuired for
  // chroma MC. If skip_hor = 0 and skip_ver = 1, mask used for chroma MC during
  // vertical filter decision may be incorrect as temporary MC evaluation
  // overwrites the mask. Make skip_ver as 0 for this case so that mask is
  // populated during luma MC
  if (is_compound && mbmi->compound_idx == 1 &&
      mbmi->interinter_comp.type == COMPOUND_DIFFWTD) {
    assert(mbmi->comp_group_idx == 1);
    if (*skip_hor == 0 && *skip_ver == 1) *skip_ver = 0;
  }
}

static int64_t interpolation_filter_search(
    MACROBLOCK *const x, const AV1_COMP *const cpi,
    const TileDataEnc *tile_data, BLOCK_SIZE bsize,
    const BUFFER_SET *const tmp_dst, const BUFFER_SET *const orig_dst,
    int64_t *const rd, int *const switchable_rate, int *skip_build_pred,
    HandleInterModeArgs *args, int64_t ref_best_rd) {
  const AV1_COMMON *cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const int need_search =
      av1_is_interp_needed(xd) && !cpi->sf.rt_sf.skip_interp_filter_search;
  const int ref_frame = xd->mi[0]->ref_frame[0];
  RD_STATS rd_stats_luma, rd_stats;

  // Initialization of rd_stats structures with default values
  av1_init_rd_stats(&rd_stats_luma);
  av1_init_rd_stats(&rd_stats);

  int match_found_idx = -1;
  const InterpFilter assign_filter = cm->interp_filter;

  match_found_idx = find_interp_filter_match(
      mbmi, cpi, assign_filter, need_search, args->interp_filter_stats,
      args->interp_filter_stats_idx);

  if (match_found_idx != -1) {
    *rd = args->interp_filter_stats[match_found_idx].rd;
    x->pred_sse[ref_frame] =
        args->interp_filter_stats[match_found_idx].pred_sse;
    return 0;
  }

  int switchable_ctx[2];
  switchable_ctx[0] = av1_get_pred_context_switchable_interp(xd, 0);
  switchable_ctx[1] = av1_get_pred_context_switchable_interp(xd, 1);
  *switchable_rate =
      get_switchable_rate(x, mbmi->interp_filters, switchable_ctx);

  // Do MC evaluation for default filter_type.
  // Luma MC
  interp_model_rd_eval(x, cpi, bsize, orig_dst, AOM_PLANE_Y, AOM_PLANE_Y,
                       &rd_stats_luma, *skip_build_pred);

#if CONFIG_COLLECT_RD_STATS == 3
  RD_STATS rd_stats_y;
  pick_tx_size_type_yrd(cpi, x, &rd_stats_y, bsize, INT64_MAX);
  PrintPredictionUnitStats(cpi, tile_data, x, &rd_stats_y, bsize);
#endif  // CONFIG_COLLECT_RD_STATS == 3
  // Chroma MC
  if (num_planes > 1) {
    interp_model_rd_eval(x, cpi, bsize, orig_dst, AOM_PLANE_U, AOM_PLANE_V,
                         &rd_stats, *skip_build_pred);
  }
  *skip_build_pred = 1;

  av1_merge_rd_stats(&rd_stats, &rd_stats_luma);

  assert(rd_stats.rate >= 0);

  *rd = RDCOST(x->rdmult, *switchable_rate + rd_stats.rate, rd_stats.dist);
  x->pred_sse[ref_frame] = (unsigned int)(rd_stats_luma.sse >> 4);

  if (assign_filter != SWITCHABLE || match_found_idx != -1) {
    return 0;
  }
  if (!need_search) {
    int_interpfilters filters = av1_broadcast_interp_filter(EIGHTTAP_REGULAR);
    assert(mbmi->interp_filters.as_int == filters.as_int);
    (void)filters;
    return 0;
  }
  if (args->modelled_rd != NULL) {
    if (has_second_ref(mbmi)) {
      const int ref_mv_idx = mbmi->ref_mv_idx;
      MV_REFERENCE_FRAME *refs = mbmi->ref_frame;
      const int mode0 = compound_ref0_mode(mbmi->mode);
      const int mode1 = compound_ref1_mode(mbmi->mode);
      const int64_t mrd = AOMMIN(args->modelled_rd[mode0][ref_mv_idx][refs[0]],
                                 args->modelled_rd[mode1][ref_mv_idx][refs[1]]);
      if ((*rd >> 1) > mrd && ref_best_rd < INT64_MAX) {
        return INT64_MAX;
      }
    }
  }

  x->recalc_luma_mc_data = 0;
  // skip_flag=xx (in binary form)
  // Setting 0th flag corresonds to skipping luma MC and setting 1st bt
  // corresponds to skipping chroma MC  skip_flag=0 corresponds to "Don't skip
  // luma and chroma MC"  Skip flag=1 corresponds to "Skip Luma MC only"
  // Skip_flag=2 is not a valid case
  // skip_flag=3 corresponds to "Skip both luma and chroma MC"
  int skip_hor = cpi->default_interp_skip_flags;
  int skip_ver = cpi->default_interp_skip_flags;
  calc_interp_skip_pred_flag(x, cpi, &skip_hor, &skip_ver);

  // do interp_filter search
  restore_dst_buf(xd, *tmp_dst, num_planes);
  const BUFFER_SET *dst_bufs[2] = { tmp_dst, orig_dst };
  // Evaluate dual interp filters
  if (cm->seq_params.enable_dual_filter) {
    if (cpi->sf.interp_sf.use_fast_interpolation_filter_search) {
      fast_dual_interp_filter_rd(x, cpi, tile_data, bsize, orig_dst, rd,
                                 &rd_stats_luma, &rd_stats, switchable_rate,
                                 dst_bufs, switchable_ctx, skip_hor, skip_ver);
    } else {
      // Use full interpolation filter search
      uint16_t allowed_interp_mask = ALLOW_ALL_INTERP_FILT_MASK;
      // REG_REG filter type is evaluated beforehand, so loop is repeated over
      // REG_SMOOTH to SHARP_SHARP for full interpolation filter search
      reset_interp_filter_allowed_mask(&allowed_interp_mask, REG_REG);
      find_best_interp_rd_facade(x, cpi, tile_data, bsize, orig_dst, rd,
                                 &rd_stats_luma, &rd_stats, switchable_rate,
                                 dst_bufs, switchable_ctx,
                                 (skip_hor & skip_ver), allowed_interp_mask, 0);
    }
  } else {
    // Evaluate non-dual interp filters
    find_best_non_dual_interp_filter(
        x, cpi, tile_data, bsize, orig_dst, rd, &rd_stats_luma, &rd_stats,
        switchable_rate, dst_bufs, switchable_ctx, skip_ver, skip_hor);
  }
  swap_dst_buf(xd, dst_bufs, num_planes);
  // Recompute final MC data if required
  if (x->recalc_luma_mc_data == 1) {
    // Recomputing final luma MC data is required only if the same was skipped
    // in either of the directions  Condition below is necessary, but not
    // sufficient
    assert((skip_hor == 1) || (skip_ver == 1));
    const int mi_row = xd->mi_row;
    const int mi_col = xd->mi_col;
    av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, orig_dst, bsize,
                                  AOM_PLANE_Y, AOM_PLANE_Y);
  }
  x->pred_sse[ref_frame] = (unsigned int)(rd_stats_luma.sse >> 4);

  // save search results
  if (cpi->sf.interp_sf.use_interp_filter) {
    assert(match_found_idx == -1);
    args->interp_filter_stats_idx = save_interp_filter_search_stat(
        mbmi, *rd, x->pred_sse[ref_frame], args->interp_filter_stats,
        args->interp_filter_stats_idx);
  }
  return 0;
}

static int txfm_search(const AV1_COMP *cpi, const TileDataEnc *tile_data,
                       MACROBLOCK *x, BLOCK_SIZE bsize, RD_STATS *rd_stats,
                       RD_STATS *rd_stats_y, RD_STATS *rd_stats_uv,
                       int mode_rate, int64_t ref_best_rd) {
  /*
   * This function combines y and uv planes' transform search processes
   * together, when the prediction is generated. It first does subtraction to
   * obtain the prediction error. Then it calls
   * pick_tx_size_type_yrd/super_block_yrd and super_block_uvrd sequentially and
   * handles the early terminations happening in those functions. At the end, it
   * computes the rd_stats/_y/_uv accordingly.
   */
  const AV1_COMMON *cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const int ref_frame_1 = mbmi->ref_frame[1];
  const int64_t mode_rd = RDCOST(x->rdmult, mode_rate, 0);
  const int64_t rd_thresh =
      ref_best_rd == INT64_MAX ? INT64_MAX : ref_best_rd - mode_rd;
  const int skip_ctx = av1_get_skip_context(xd);
  const int skip_flag_cost[2] = { x->skip_cost[skip_ctx][0],
                                  x->skip_cost[skip_ctx][1] };
  const int64_t min_header_rate =
      mode_rate + AOMMIN(skip_flag_cost[0], skip_flag_cost[1]);
  // Account for minimum skip and non_skip rd.
  // Eventually either one of them will be added to mode_rate
  const int64_t min_header_rd_possible = RDCOST(x->rdmult, min_header_rate, 0);
  (void)tile_data;

  if (min_header_rd_possible > ref_best_rd) {
    av1_invalid_rd_stats(rd_stats_y);
    return 0;
  }

  av1_init_rd_stats(rd_stats);
  av1_init_rd_stats(rd_stats_y);
  rd_stats->rate = mode_rate;

  // cost and distortion
  av1_subtract_plane(x, bsize, 0);
  if (x->tx_mode_search_type == TX_MODE_SELECT &&
      !xd->lossless[mbmi->segment_id]) {
    pick_tx_size_type_yrd(cpi, x, rd_stats_y, bsize, rd_thresh);
#if CONFIG_COLLECT_RD_STATS == 2
    PrintPredictionUnitStats(cpi, tile_data, x, rd_stats_y, bsize);
#endif  // CONFIG_COLLECT_RD_STATS == 2
  } else {
    super_block_yrd(cpi, x, rd_stats_y, bsize, rd_thresh);
    memset(mbmi->inter_tx_size, mbmi->tx_size, sizeof(mbmi->inter_tx_size));
    for (int i = 0; i < xd->n4_h * xd->n4_w; ++i)
      set_blk_skip(x, 0, i, rd_stats_y->skip);
  }

  if (rd_stats_y->rate == INT_MAX) {
    // TODO(angiebird): check if we need this
    // restore_dst_buf(xd, *orig_dst, num_planes);
    mbmi->ref_frame[1] = ref_frame_1;
    return 0;
  }

  av1_merge_rd_stats(rd_stats, rd_stats_y);

  const int64_t non_skip_rdcosty =
      RDCOST(x->rdmult, rd_stats->rate + skip_flag_cost[0], rd_stats->dist);
  const int64_t skip_rdcosty =
      RDCOST(x->rdmult, mode_rate + skip_flag_cost[1], rd_stats->sse);
  const int64_t min_rdcosty = AOMMIN(non_skip_rdcosty, skip_rdcosty);
  if (min_rdcosty > ref_best_rd) {
    const int64_t tokenonly_rdy =
        AOMMIN(RDCOST(x->rdmult, rd_stats_y->rate, rd_stats_y->dist),
               RDCOST(x->rdmult, 0, rd_stats_y->sse));
    // Invalidate rd_stats_y to skip the rest of the motion modes search
    if (tokenonly_rdy -
            (tokenonly_rdy >> cpi->sf.inter_sf.prune_motion_mode_level) >
        rd_thresh)
      av1_invalid_rd_stats(rd_stats_y);
    mbmi->ref_frame[1] = ref_frame_1;
    return 0;
  }

  av1_init_rd_stats(rd_stats_uv);
  const int num_planes = av1_num_planes(cm);
  if (num_planes > 1) {
    int64_t ref_best_chroma_rd = ref_best_rd;
    // Calculate best rd cost possible for chroma
    if (cpi->sf.inter_sf.perform_best_rd_based_gating_for_chroma &&
        (ref_best_chroma_rd != INT64_MAX)) {
      ref_best_chroma_rd =
          (ref_best_chroma_rd - AOMMIN(non_skip_rdcosty, skip_rdcosty));
    }
    const int is_cost_valid_uv =
        super_block_uvrd(cpi, x, rd_stats_uv, bsize, ref_best_chroma_rd);
    if (!is_cost_valid_uv) {
      mbmi->ref_frame[1] = ref_frame_1;
      return 0;
    }
    av1_merge_rd_stats(rd_stats, rd_stats_uv);
  }

  if (rd_stats->skip) {
    rd_stats->rate -= rd_stats_uv->rate + rd_stats_y->rate;
    rd_stats_y->rate = 0;
    rd_stats_uv->rate = 0;
    rd_stats->dist = rd_stats->sse;
    rd_stats_y->dist = rd_stats_y->sse;
    rd_stats_uv->dist = rd_stats_uv->sse;
    rd_stats->rate += skip_flag_cost[1];
    mbmi->skip = 1;
    // here mbmi->skip temporarily plays a role as what this_skip2 does

    const int64_t tmprd = RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist);
    if (tmprd > ref_best_rd) {
      mbmi->ref_frame[1] = ref_frame_1;
      return 0;
    }
  } else if (!xd->lossless[mbmi->segment_id] &&
             (RDCOST(x->rdmult,
                     rd_stats_y->rate + rd_stats_uv->rate + skip_flag_cost[0],
                     rd_stats->dist) >=
              RDCOST(x->rdmult, skip_flag_cost[1], rd_stats->sse))) {
    rd_stats->rate -= rd_stats_uv->rate + rd_stats_y->rate;
    rd_stats->rate += skip_flag_cost[1];
    rd_stats->dist = rd_stats->sse;
    rd_stats_y->dist = rd_stats_y->sse;
    rd_stats_uv->dist = rd_stats_uv->sse;
    rd_stats_y->rate = 0;
    rd_stats_uv->rate = 0;
    mbmi->skip = 1;
  } else {
    rd_stats->rate += skip_flag_cost[0];
    mbmi->skip = 0;
  }

  return 1;
}

static INLINE bool enable_wedge_search(MACROBLOCK *const x,
                                       const AV1_COMP *const cpi) {
  // Enable wedge search if source variance and edge strength are above
  // the thresholds.
  return x->source_variance >
             cpi->sf.inter_sf.disable_wedge_search_var_thresh &&
         x->edge_strength > cpi->sf.inter_sf.disable_wedge_search_edge_thresh;
}

static INLINE bool enable_wedge_interinter_search(MACROBLOCK *const x,
                                                  const AV1_COMP *const cpi) {
  return enable_wedge_search(x, cpi) && cpi->oxcf.enable_interinter_wedge &&
         !cpi->sf.inter_sf.disable_interinter_wedge;
}

static INLINE bool enable_wedge_interintra_search(MACROBLOCK *const x,
                                                  const AV1_COMP *const cpi) {
  return enable_wedge_search(x, cpi) && cpi->oxcf.enable_interintra_wedge &&
         !cpi->sf.inter_sf.disable_wedge_interintra_search;
}

static int handle_inter_intra_mode(const AV1_COMP *const cpi,
                                   MACROBLOCK *const x, BLOCK_SIZE bsize,
                                   MB_MODE_INFO *mbmi,
                                   HandleInterModeArgs *args,
                                   int64_t ref_best_rd, int *rate_mv,
                                   int *tmp_rate2, const BUFFER_SET *orig_dst) {
  const int try_smooth_interintra = cpi->oxcf.enable_smooth_interintra &&
                                    !cpi->sf.inter_sf.disable_smooth_interintra;
  const int is_wedge_used = av1_is_wedge_used(bsize);
  const int try_wedge_interintra =
      is_wedge_used && enable_wedge_interintra_search(x, cpi);
  if (!try_smooth_interintra && !try_wedge_interintra) return -1;

  const AV1_COMMON *const cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;
  int64_t rd = INT64_MAX;
  int rmode, rate_sum;
  int64_t dist_sum;
  int tmp_rate_mv = 0;
  int tmp_skip_txfm_sb;
  const int bw = block_size_wide[bsize];
  int64_t tmp_skip_sse_sb;
  DECLARE_ALIGNED(16, uint8_t, tmp_buf_[2 * MAX_INTERINTRA_SB_SQUARE]);
  DECLARE_ALIGNED(16, uint8_t, intrapred_[2 * MAX_INTERINTRA_SB_SQUARE]);
  uint8_t *tmp_buf = get_buf_by_bd(xd, tmp_buf_);
  uint8_t *intrapred = get_buf_by_bd(xd, intrapred_);
  const int *const interintra_mode_cost =
      x->interintra_mode_cost[size_group_lookup[bsize]];
  const int_mv mv0 = mbmi->mv[0];
  mbmi->ref_frame[1] = NONE_FRAME;
  xd->plane[0].dst.buf = tmp_buf;
  xd->plane[0].dst.stride = bw;
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, NULL, bsize,
                                AOM_PLANE_Y, AOM_PLANE_Y);
  const int num_planes = av1_num_planes(cm);
  restore_dst_buf(xd, *orig_dst, num_planes);
  mbmi->ref_frame[1] = INTRA_FRAME;
  INTERINTRA_MODE best_interintra_mode =
      args->inter_intra_mode[mbmi->ref_frame[0]];

  int64_t best_interintra_rd_nowedge = INT64_MAX;
  if (try_smooth_interintra) {
    mbmi->use_wedge_interintra = 0;
    int j = 0;
    if (cpi->sf.inter_sf.reuse_inter_intra_mode == 0 ||
        best_interintra_mode == INTERINTRA_MODES) {
      for (j = 0; j < INTERINTRA_MODES; ++j) {
        if ((!cpi->oxcf.enable_smooth_intra ||
             cpi->sf.intra_sf.disable_smooth_intra) &&
            (INTERINTRA_MODE)j == II_SMOOTH_PRED)
          continue;
        mbmi->interintra_mode = (INTERINTRA_MODE)j;
        rmode = interintra_mode_cost[mbmi->interintra_mode];
        av1_build_intra_predictors_for_interintra(cm, xd, bsize, 0, orig_dst,
                                                  intrapred, bw);
        av1_combine_interintra(xd, bsize, 0, tmp_buf, bw, intrapred, bw);
        model_rd_sb_fn[MODELRD_TYPE_INTERINTRA](
            cpi, bsize, x, xd, 0, 0, &rate_sum, &dist_sum, &tmp_skip_txfm_sb,
            &tmp_skip_sse_sb, NULL, NULL, NULL);
        rd = RDCOST(x->rdmult, tmp_rate_mv + rate_sum + rmode, dist_sum);
        if (rd < best_interintra_rd_nowedge) {
          best_interintra_rd_nowedge = rd;
          best_interintra_mode = mbmi->interintra_mode;
        }
      }
      args->inter_intra_mode[mbmi->ref_frame[0]] = best_interintra_mode;
    }
    assert(IMPLIES(!cpi->oxcf.enable_smooth_interintra ||
                       cpi->sf.inter_sf.disable_smooth_interintra,
                   best_interintra_mode != II_SMOOTH_PRED));
    rmode = interintra_mode_cost[best_interintra_mode];
    if (j == 0 || best_interintra_mode != INTERINTRA_MODES - 1) {
      mbmi->interintra_mode = best_interintra_mode;
      av1_build_intra_predictors_for_interintra(cm, xd, bsize, 0, orig_dst,
                                                intrapred, bw);
      av1_combine_interintra(xd, bsize, 0, tmp_buf, bw, intrapred, bw);
    }

    RD_STATS rd_stats;
    const int64_t rd_thresh = get_rd_thresh_from_best_rd(
        ref_best_rd, (1 << INTER_INTRA_RD_THRESH_SHIFT),
        INTER_INTRA_RD_THRESH_SCALE);
    const int rwedge = is_wedge_used ? x->wedge_interintra_cost[bsize][0] : 0;
    const int total_mode_rate = *rate_mv + rmode + rwedge;
    const int64_t mode_rd = RDCOST(x->rdmult, total_mode_rate, 0);
    const int64_t tmp_rd_thresh = rd_thresh - mode_rd;
    rd = estimate_yrd_for_sb(cpi, bsize, x, tmp_rd_thresh, &rd_stats);
    if (rd != INT64_MAX) {
      rd = RDCOST(x->rdmult, total_mode_rate + rd_stats.rate, rd_stats.dist);
    } else {
      return -1;
    }
    best_interintra_rd_nowedge = rd;
    if (ref_best_rd < INT64_MAX &&
        (best_interintra_rd_nowedge >> INTER_INTRA_RD_THRESH_SHIFT) *
                INTER_INTRA_RD_THRESH_SCALE >
            ref_best_rd) {
      return -1;
    }
  }

  int64_t best_interintra_rd_wedge = INT64_MAX;
  if (try_wedge_interintra) {
    mbmi->use_wedge_interintra = 1;
    if (!cpi->sf.inter_sf.fast_interintra_wedge_search) {
      // Exhaustive search of all wedge and mode combinations.
      int best_mode = 0;
      int best_wedge_index = 0;
      int64_t best_total_rd = INT64_MAX;
      for (int j = 0; j < INTERINTRA_MODES; ++j) {
        mbmi->interintra_mode = (INTERINTRA_MODE)j;
        av1_build_intra_predictors_for_interintra(cm, xd, bsize, 0, orig_dst,
                                                  intrapred, bw);
        rd = pick_interintra_wedge(cpi, x, bsize, intrapred_, tmp_buf_);
        const int rate_overhead =
            interintra_mode_cost[mbmi->interintra_mode] +
            x->wedge_idx_cost[bsize][mbmi->interintra_wedge_index];
        const int64_t total_rd = rd + RDCOST(x->rdmult, rate_overhead, 0);
        if (total_rd < best_total_rd) {
          best_total_rd = total_rd;
          best_interintra_rd_wedge = rd;
          best_mode = mbmi->interintra_mode;
          best_wedge_index = mbmi->interintra_wedge_index;
        }
      }
      mbmi->interintra_mode = best_mode;
      mbmi->interintra_wedge_index = best_wedge_index;
      if (best_mode != INTERINTRA_MODES - 1) {
        av1_build_intra_predictors_for_interintra(cm, xd, bsize, 0, orig_dst,
                                                  intrapred, bw);
      }
    } else if (!try_smooth_interintra) {
      if (best_interintra_mode == INTERINTRA_MODES) {
        mbmi->interintra_mode = INTERINTRA_MODES - 1;
        best_interintra_mode = INTERINTRA_MODES - 1;
        av1_build_intra_predictors_for_interintra(cm, xd, bsize, 0, orig_dst,
                                                  intrapred, bw);
        best_interintra_rd_wedge =
            pick_interintra_wedge(cpi, x, bsize, intrapred_, tmp_buf_);

        for (int j = 0; j < INTERINTRA_MODES; ++j) {
          mbmi->interintra_mode = (INTERINTRA_MODE)j;
          rmode = interintra_mode_cost[mbmi->interintra_mode];
          av1_build_intra_predictors_for_interintra(cm, xd, bsize, 0, orig_dst,
                                                    intrapred, bw);
          av1_combine_interintra(xd, bsize, 0, tmp_buf, bw, intrapred, bw);
          model_rd_sb_fn[MODELRD_TYPE_INTERINTRA](
              cpi, bsize, x, xd, 0, 0, &rate_sum, &dist_sum, &tmp_skip_txfm_sb,
              &tmp_skip_sse_sb, NULL, NULL, NULL);
          rd = RDCOST(x->rdmult, tmp_rate_mv + rate_sum + rmode, dist_sum);
          if (rd < best_interintra_rd_wedge) {
            best_interintra_rd_wedge = rd;
            best_interintra_mode = mbmi->interintra_mode;
          }
        }
        args->inter_intra_mode[mbmi->ref_frame[0]] = best_interintra_mode;
        mbmi->interintra_mode = best_interintra_mode;

        if (best_interintra_mode != INTERINTRA_MODES - 1) {
          av1_build_intra_predictors_for_interintra(cm, xd, bsize, 0, orig_dst,
                                                    intrapred, bw);
        }
      } else {
        mbmi->interintra_mode = best_interintra_mode;
        av1_build_intra_predictors_for_interintra(cm, xd, bsize, 0, orig_dst,
                                                  intrapred, bw);
        best_interintra_rd_wedge =
            pick_interintra_wedge(cpi, x, bsize, intrapred_, tmp_buf_);
      }
    } else {
      best_interintra_rd_wedge =
          pick_interintra_wedge(cpi, x, bsize, intrapred_, tmp_buf_);
    }

    const int rate_overhead =
        interintra_mode_cost[mbmi->interintra_mode] +
        x->wedge_idx_cost[bsize][mbmi->interintra_wedge_index] +
        x->wedge_interintra_cost[bsize][1];
    best_interintra_rd_wedge += RDCOST(x->rdmult, rate_overhead + *rate_mv, 0);

    int_mv tmp_mv;
    rd = INT64_MAX;
    // Refine motion vector.
    if (have_newmv_in_inter_mode(mbmi->mode)) {
      // get negative of mask
      const uint8_t *mask =
          av1_get_contiguous_soft_mask(mbmi->interintra_wedge_index, 1, bsize);
      tmp_mv = mbmi->mv[0];
      compound_single_motion_search(cpi, x, bsize, &tmp_mv.as_mv, intrapred,
                                    mask, bw, &tmp_rate_mv, 0);
      if (mbmi->mv[0].as_int != tmp_mv.as_int) {
        mbmi->mv[0].as_int = tmp_mv.as_int;
        av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, orig_dst, bsize,
                                      AOM_PLANE_Y, AOM_PLANE_Y);
        model_rd_sb_fn[MODELRD_TYPE_MASKED_COMPOUND](
            cpi, bsize, x, xd, 0, 0, &rate_sum, &dist_sum, &tmp_skip_txfm_sb,
            &tmp_skip_sse_sb, NULL, NULL, NULL);
        rd =
            RDCOST(x->rdmult, tmp_rate_mv + rate_overhead + rate_sum, dist_sum);
      }
    }
    if (rd >= best_interintra_rd_wedge) {
      tmp_mv.as_int = mv0.as_int;
      tmp_rate_mv = *rate_mv;
      av1_combine_interintra(xd, bsize, 0, tmp_buf, bw, intrapred, bw);
    }
    // Evaluate closer to true rd
    RD_STATS rd_stats;
    const int64_t mode_rd = RDCOST(x->rdmult, rate_overhead + tmp_rate_mv, 0);
    const int64_t tmp_rd_thresh = best_interintra_rd_nowedge - mode_rd;
    rd = estimate_yrd_for_sb(cpi, bsize, x, tmp_rd_thresh, &rd_stats);
    if (rd != INT64_MAX) {
      rd = RDCOST(x->rdmult, rate_overhead + tmp_rate_mv + rd_stats.rate,
                  rd_stats.dist);
    } else {
      if (best_interintra_rd_nowedge == INT64_MAX) return -1;
    }
    best_interintra_rd_wedge = rd;
    if (best_interintra_rd_wedge < best_interintra_rd_nowedge) {
      mbmi->mv[0].as_int = tmp_mv.as_int;
      *tmp_rate2 += tmp_rate_mv - *rate_mv;
      *rate_mv = tmp_rate_mv;
    } else {
      mbmi->use_wedge_interintra = 0;
      mbmi->interintra_mode = best_interintra_mode;
      mbmi->mv[0].as_int = mv0.as_int;
      av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, orig_dst, bsize,
                                    AOM_PLANE_Y, AOM_PLANE_Y);
    }
  }

  if (best_interintra_rd_nowedge == INT64_MAX &&
      best_interintra_rd_wedge == INT64_MAX) {
    return -1;
  }

  if (num_planes > 1) {
    av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, orig_dst, bsize,
                                  AOM_PLANE_U, num_planes - 1);
  }
  return 0;
}

// If number of valid neighbours is 1,
// 1) ROTZOOM parameters can be obtained reliably (2 parameters from
// one neighbouring MV)
// 2) For IDENTITY/TRANSLATION cases, warp can perform better due to
// a different interpolation filter being used. However the quality
// gains (due to the same) may not be much
// For above 2 cases warp evaluation is skipped

static int check_if_optimal_warp(const AV1_COMP *cpi,
                                 WarpedMotionParams *wm_params,
                                 int num_proj_ref) {
  int is_valid_warp = 1;
  if (cpi->sf.inter_sf.prune_warp_using_wmtype) {
    TransformationType wmtype = get_wmtype(wm_params);
    if (num_proj_ref == 1) {
      if (wmtype != ROTZOOM) is_valid_warp = 0;
    } else {
      if (wmtype < ROTZOOM) is_valid_warp = 0;
    }
  }
  return is_valid_warp;
}

static INLINE void update_mode_start_end_index(const AV1_COMP *const cpi,
                                               int *mode_index_start,
                                               int *mode_index_end,
                                               int last_motion_mode_allowed,
                                               int interintra_allowed,
                                               int eval_motion_mode) {
  *mode_index_start = (int)SIMPLE_TRANSLATION;
  *mode_index_end = (int)last_motion_mode_allowed + interintra_allowed;
  if (cpi->sf.winner_mode_sf.motion_mode_for_winner_cand) {
    if (!eval_motion_mode) {
      *mode_index_end = (int)SIMPLE_TRANSLATION;
    } else {
      // Set the start index appropriately to process motion modes other than
      // simple translation
      *mode_index_start = 1;
    }
  }
}

// TODO(afergs): Refactor the MBMI references in here - there's four
// TODO(afergs): Refactor optional args - add them to a struct or remove
static int64_t motion_mode_rd(
    const AV1_COMP *const cpi, TileDataEnc *tile_data, MACROBLOCK *const x,
    BLOCK_SIZE bsize, RD_STATS *rd_stats, RD_STATS *rd_stats_y,
    RD_STATS *rd_stats_uv, int *disable_skip, HandleInterModeArgs *const args,
    int64_t ref_best_rd, int *rate_mv, const BUFFER_SET *orig_dst,
    int64_t *best_est_rd, int do_tx_search, InterModesInfo *inter_modes_info,
    int eval_motion_mode) {
  const AV1_COMMON *const cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  const int is_comp_pred = has_second_ref(mbmi);
  const PREDICTION_MODE this_mode = mbmi->mode;
  const int rate2_nocoeff = rd_stats->rate;
  int best_xskip = 0, best_disable_skip = 0;
  RD_STATS best_rd_stats, best_rd_stats_y, best_rd_stats_uv;
  uint8_t best_blk_skip[MAX_MIB_SIZE * MAX_MIB_SIZE];
  uint8_t best_tx_type_map[MAX_MIB_SIZE * MAX_MIB_SIZE];
  const int rate_mv0 = *rate_mv;
  const int interintra_allowed = cm->seq_params.enable_interintra_compound &&
                                 is_interintra_allowed(mbmi) &&
                                 mbmi->compound_idx;
  int pts0[SAMPLES_ARRAY_SIZE], pts_inref0[SAMPLES_ARRAY_SIZE];

  assert(mbmi->ref_frame[1] != INTRA_FRAME);
  const MV_REFERENCE_FRAME ref_frame_1 = mbmi->ref_frame[1];
  (void)tile_data;
  av1_invalid_rd_stats(&best_rd_stats);
  aom_clear_system_state();
  mbmi->num_proj_ref = 1;  // assume num_proj_ref >=1
  MOTION_MODE last_motion_mode_allowed = SIMPLE_TRANSLATION;
  if (cm->switchable_motion_mode) {
    last_motion_mode_allowed = motion_mode_allowed(xd->global_motion, xd, mbmi,
                                                   cm->allow_warped_motion);
  }

  if (last_motion_mode_allowed == WARPED_CAUSAL) {
    mbmi->num_proj_ref = av1_findSamples(cm, xd, pts0, pts_inref0);
  }
  const int total_samples = mbmi->num_proj_ref;
  if (total_samples == 0) {
    last_motion_mode_allowed = OBMC_CAUSAL;
  }

  const MB_MODE_INFO base_mbmi = *mbmi;
  MB_MODE_INFO best_mbmi;
  SimpleRDState *const simple_states = &args->simple_rd_state[mbmi->ref_mv_idx];
  const int switchable_rate =
      av1_is_interp_needed(xd) ? av1_get_switchable_rate(cm, x, xd) : 0;
  int64_t best_rd = INT64_MAX;
  int best_rate_mv = rate_mv0;
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  int mode_index_start, mode_index_end;
  update_mode_start_end_index(cpi, &mode_index_start, &mode_index_end,
                              last_motion_mode_allowed, interintra_allowed,
                              eval_motion_mode);
  for (int mode_index = mode_index_start; mode_index <= mode_index_end;
       mode_index++) {
    if (args->skip_motion_mode && mode_index) continue;
    if (cpi->sf.inter_sf.prune_single_motion_modes_by_simple_trans &&
        args->single_ref_first_pass && mode_index)
      break;
    int tmp_rate2 = rate2_nocoeff;
    const int is_interintra_mode = mode_index > (int)last_motion_mode_allowed;
    int tmp_rate_mv = rate_mv0;

    *mbmi = base_mbmi;
    if (is_interintra_mode) {
      mbmi->motion_mode = SIMPLE_TRANSLATION;
    } else {
      mbmi->motion_mode = (MOTION_MODE)mode_index;
      assert(mbmi->ref_frame[1] != INTRA_FRAME);
    }

    const FRAME_UPDATE_TYPE update_type = get_frame_update_type(&cpi->gf_group);
    const int prune_obmc = cpi->obmc_probs[update_type][bsize] <
                           cpi->sf.inter_sf.prune_obmc_prob_thresh;
    if ((cpi->oxcf.enable_obmc == 0 || cpi->sf.inter_sf.disable_obmc ||
         cpi->sf.rt_sf.use_nonrd_pick_mode || prune_obmc) &&
        mbmi->motion_mode == OBMC_CAUSAL)
      continue;

    if (mbmi->motion_mode == SIMPLE_TRANSLATION && !is_interintra_mode) {
      // SIMPLE_TRANSLATION mode: no need to recalculate.
      // The prediction is calculated before motion_mode_rd() is called in
      // handle_inter_mode()
      if (cpi->sf.inter_sf.prune_single_motion_modes_by_simple_trans &&
          !is_comp_pred) {
        if (args->single_ref_first_pass == 0) {
          if (simple_states->early_skipped) {
            assert(simple_states->rd_stats.rdcost == INT64_MAX);
            return INT64_MAX;
          }
          if (simple_states->rd_stats.rdcost != INT64_MAX) {
            best_rd = simple_states->rd_stats.rdcost;
            best_rd_stats = simple_states->rd_stats;
            best_rd_stats_y = simple_states->rd_stats_y;
            best_rd_stats_uv = simple_states->rd_stats_uv;
            memcpy(best_blk_skip, simple_states->blk_skip,
                   sizeof(x->blk_skip[0]) * xd->n4_h * xd->n4_w);
            av1_copy_array(best_tx_type_map, simple_states->tx_type_map,
                           xd->n4_h * xd->n4_w);
            best_xskip = simple_states->skip;
            best_disable_skip = simple_states->disable_skip;
            best_mbmi = *mbmi;
          }
          continue;
        }
        simple_states->early_skipped = 0;
      }
    } else if (mbmi->motion_mode == OBMC_CAUSAL) {
      const uint32_t cur_mv = mbmi->mv[0].as_int;
      assert(!is_comp_pred);
      if (have_newmv_in_inter_mode(this_mode)) {
        single_motion_search(cpi, x, bsize, 0, &tmp_rate_mv);
        mbmi->mv[0].as_int = x->best_mv.as_int;
        tmp_rate2 = rate2_nocoeff - rate_mv0 + tmp_rate_mv;
      }
      if ((mbmi->mv[0].as_int != cur_mv) || eval_motion_mode) {
        av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, orig_dst, bsize,
                                      0, av1_num_planes(cm) - 1);
      }
      av1_build_obmc_inter_prediction(
          cm, xd, args->above_pred_buf, args->above_pred_stride,
          args->left_pred_buf, args->left_pred_stride);
    } else if (mbmi->motion_mode == WARPED_CAUSAL) {
      int pts[SAMPLES_ARRAY_SIZE], pts_inref[SAMPLES_ARRAY_SIZE];
      mbmi->motion_mode = WARPED_CAUSAL;
      mbmi->wm_params.wmtype = DEFAULT_WMTYPE;
      mbmi->interp_filters = av1_broadcast_interp_filter(
          av1_unswitchable_filter(cm->interp_filter));

      memcpy(pts, pts0, total_samples * 2 * sizeof(*pts0));
      memcpy(pts_inref, pts_inref0, total_samples * 2 * sizeof(*pts_inref0));
      // Select the samples according to motion vector difference
      if (mbmi->num_proj_ref > 1) {
        mbmi->num_proj_ref = av1_selectSamples(
            &mbmi->mv[0].as_mv, pts, pts_inref, mbmi->num_proj_ref, bsize);
      }

      if (!av1_find_projection(mbmi->num_proj_ref, pts, pts_inref, bsize,
                               mbmi->mv[0].as_mv.row, mbmi->mv[0].as_mv.col,
                               &mbmi->wm_params, mi_row, mi_col)) {
        // Refine MV for NEWMV mode
        assert(!is_comp_pred);
        if (have_newmv_in_inter_mode(this_mode)) {
          const int_mv mv0 = mbmi->mv[0];
          const WarpedMotionParams wm_params0 = mbmi->wm_params;
          const int num_proj_ref0 = mbmi->num_proj_ref;

          if (cpi->sf.inter_sf.prune_warp_using_wmtype) {
            TransformationType wmtype = get_wmtype(&mbmi->wm_params);
            if (wmtype < ROTZOOM) continue;
          }

          // Refine MV in a small range.
          av1_refine_warped_mv(cpi, x, bsize, pts0, pts_inref0, total_samples);

          // Keep the refined MV and WM parameters.
          if (mv0.as_int != mbmi->mv[0].as_int) {
            const int_mv ref_mv = av1_get_ref_mv(x, 0);
            tmp_rate_mv = av1_mv_bit_cost(&mbmi->mv[0].as_mv, &ref_mv.as_mv,
                                          x->nmv_vec_cost, x->mv_cost_stack,
                                          MV_COST_WEIGHT);
            if (cpi->sf.mv_sf.adaptive_motion_search) {
              x->pred_mv[mbmi->ref_frame[0]] = mbmi->mv[0].as_mv;
            }
            tmp_rate2 = rate2_nocoeff - rate_mv0 + tmp_rate_mv;
          } else {
            // Restore the old MV and WM parameters.
            mbmi->mv[0] = mv0;
            mbmi->wm_params = wm_params0;
            mbmi->num_proj_ref = num_proj_ref0;
          }
        } else {
          if (!check_if_optimal_warp(cpi, &mbmi->wm_params, mbmi->num_proj_ref))
            continue;
        }

        av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, NULL, bsize, 0,
                                      av1_num_planes(cm) - 1);
      } else {
        continue;
      }
    } else if (is_interintra_mode) {
      const int ret =
          handle_inter_intra_mode(cpi, x, bsize, mbmi, args, ref_best_rd,
                                  &tmp_rate_mv, &tmp_rate2, orig_dst);
      if (ret < 0) continue;
    }

    // If we are searching newmv and the mv is the same as refmv, skip the
    // current mode
    if (this_mode == NEW_NEWMV) {
      const int_mv ref_mv_0 = av1_get_ref_mv(x, 0);
      const int_mv ref_mv_1 = av1_get_ref_mv(x, 1);
      if (mbmi->mv[0].as_int == ref_mv_0.as_int ||
          mbmi->mv[1].as_int == ref_mv_1.as_int) {
        continue;
      }
    } else if (this_mode == NEAREST_NEWMV || this_mode == NEAR_NEWMV) {
      const int_mv ref_mv_1 = av1_get_ref_mv(x, 1);
      if (mbmi->mv[1].as_int == ref_mv_1.as_int) {
        continue;
      }
    } else if (this_mode == NEW_NEARESTMV || this_mode == NEW_NEARMV) {
      const int_mv ref_mv_0 = av1_get_ref_mv(x, 0);
      if (mbmi->mv[0].as_int == ref_mv_0.as_int) {
        continue;
      }
    } else if (this_mode == NEWMV) {
      const int_mv ref_mv_0 = av1_get_ref_mv(x, 0);
      if (mbmi->mv[0].as_int == ref_mv_0.as_int) {
        continue;
      }
    }

    x->force_skip = 0;
    rd_stats->dist = 0;
    rd_stats->sse = 0;
    rd_stats->skip = 1;
    rd_stats->rate = tmp_rate2;
    if (mbmi->motion_mode != WARPED_CAUSAL) rd_stats->rate += switchable_rate;
    if (interintra_allowed) {
      rd_stats->rate += x->interintra_cost[size_group_lookup[bsize]]
                                          [mbmi->ref_frame[1] == INTRA_FRAME];
      if (mbmi->ref_frame[1] == INTRA_FRAME) {
        rd_stats->rate += x->interintra_mode_cost[size_group_lookup[bsize]]
                                                 [mbmi->interintra_mode];
        if (av1_is_wedge_used(bsize)) {
          rd_stats->rate +=
              x->wedge_interintra_cost[bsize][mbmi->use_wedge_interintra];
          if (mbmi->use_wedge_interintra) {
            rd_stats->rate +=
                x->wedge_idx_cost[bsize][mbmi->interintra_wedge_index];
          }
        }
      }
    }
    if ((last_motion_mode_allowed > SIMPLE_TRANSLATION) &&
        (mbmi->ref_frame[1] != INTRA_FRAME)) {
      if (last_motion_mode_allowed == WARPED_CAUSAL) {
        rd_stats->rate += x->motion_mode_cost[bsize][mbmi->motion_mode];
      } else {
        rd_stats->rate += x->motion_mode_cost1[bsize][mbmi->motion_mode];
      }
    }

    if (!do_tx_search) {
      int64_t curr_sse = -1;
      int est_residue_cost = 0;
      int64_t est_dist = 0;
      int64_t est_rd = 0;
      if (cpi->sf.inter_sf.inter_mode_rd_model_estimation == 1) {
        curr_sse = get_sse(cpi, x);
        const int has_est_rd = get_est_rate_dist(tile_data, bsize, curr_sse,
                                                 &est_residue_cost, &est_dist);
        (void)has_est_rd;
        assert(has_est_rd);
      } else if (cpi->sf.inter_sf.inter_mode_rd_model_estimation == 2 ||
                 cpi->sf.rt_sf.use_nonrd_pick_mode) {
        model_rd_sb_fn[MODELRD_TYPE_MOTION_MODE_RD](
            cpi, bsize, x, xd, 0, num_planes - 1, &est_residue_cost, &est_dist,
            NULL, &curr_sse, NULL, NULL, NULL);
      }
      est_rd = RDCOST(x->rdmult, rd_stats->rate + est_residue_cost, est_dist);
      if (est_rd * 0.80 > *best_est_rd) {
        mbmi->ref_frame[1] = ref_frame_1;
        continue;
      }
      const int mode_rate = rd_stats->rate;
      rd_stats->rate += est_residue_cost;
      rd_stats->dist = est_dist;
      rd_stats->rdcost = est_rd;
      *best_est_rd = AOMMIN(*best_est_rd, rd_stats->rdcost);
      if (cm->current_frame.reference_mode == SINGLE_REFERENCE) {
        if (!is_comp_pred) {
          assert(curr_sse >= 0);
          inter_modes_info_push(inter_modes_info, mode_rate, curr_sse,
                                rd_stats->rdcost, rd_stats, rd_stats_y,
                                rd_stats_uv, mbmi);
        }
      } else {
        assert(curr_sse >= 0);
        inter_modes_info_push(inter_modes_info, mode_rate, curr_sse,
                              rd_stats->rdcost, rd_stats, rd_stats_y,
                              rd_stats_uv, mbmi);
      }
      mbmi->skip = 0;
    } else {
      if (!txfm_search(cpi, tile_data, x, bsize, rd_stats, rd_stats_y,
                       rd_stats_uv, rd_stats->rate, ref_best_rd)) {
        if (rd_stats_y->rate == INT_MAX && mode_index == 0) {
          if (cpi->sf.inter_sf.prune_single_motion_modes_by_simple_trans &&
              !is_comp_pred) {
            simple_states->early_skipped = 1;
          }
          return INT64_MAX;
        }
        continue;
      }

      const int64_t curr_rd = RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist);
      ref_best_rd = AOMMIN(ref_best_rd, curr_rd);
      *disable_skip = 0;
      if (cpi->sf.inter_sf.inter_mode_rd_model_estimation == 1) {
        const int skip_ctx = av1_get_skip_context(xd);
        inter_mode_data_push(tile_data, mbmi->sb_type, rd_stats->sse,
                             rd_stats->dist,
                             rd_stats_y->rate + rd_stats_uv->rate +
                                 x->skip_cost[skip_ctx][mbmi->skip]);
      }
    }

    if (this_mode == GLOBALMV || this_mode == GLOBAL_GLOBALMV) {
      if (is_nontrans_global_motion(xd, xd->mi[0])) {
        mbmi->interp_filters = av1_broadcast_interp_filter(
            av1_unswitchable_filter(cm->interp_filter));
      }
    }

    const int64_t tmp_rd = RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist);
    if (mode_index == 0) {
      args->simple_rd[this_mode][mbmi->ref_mv_idx][mbmi->ref_frame[0]] = tmp_rd;
      if (!is_comp_pred) {
        simple_states->rd_stats = *rd_stats;
        simple_states->rd_stats.rdcost = tmp_rd;
        simple_states->rd_stats_y = *rd_stats_y;
        simple_states->rd_stats_uv = *rd_stats_uv;
        memcpy(simple_states->blk_skip, x->blk_skip,
               sizeof(x->blk_skip[0]) * xd->n4_h * xd->n4_w);
        av1_copy_array(simple_states->tx_type_map, xd->tx_type_map,
                       xd->n4_h * xd->n4_w);
        simple_states->skip = mbmi->skip;
        simple_states->disable_skip = *disable_skip;
      }
    }
    if (mode_index == 0 || tmp_rd < best_rd) {
      best_mbmi = *mbmi;
      best_rd = tmp_rd;
      best_rd_stats = *rd_stats;
      best_rd_stats_y = *rd_stats_y;
      best_rate_mv = tmp_rate_mv;
      if (num_planes > 1) best_rd_stats_uv = *rd_stats_uv;
      memcpy(best_blk_skip, x->blk_skip,
             sizeof(x->blk_skip[0]) * xd->n4_h * xd->n4_w);
      av1_copy_array(best_tx_type_map, xd->tx_type_map, xd->n4_h * xd->n4_w);
      best_xskip = mbmi->skip;
      best_disable_skip = *disable_skip;
      // TODO(anyone): evaluate the quality and speed trade-off of the early
      // termination logic below.
      // if (best_xskip) break;
    }
  }
  mbmi->ref_frame[1] = ref_frame_1;
  *rate_mv = best_rate_mv;
  if (best_rd == INT64_MAX) {
    av1_invalid_rd_stats(rd_stats);
    restore_dst_buf(xd, *orig_dst, num_planes);
    return INT64_MAX;
  }
  *mbmi = best_mbmi;
  *rd_stats = best_rd_stats;
  *rd_stats_y = best_rd_stats_y;
  if (num_planes > 1) *rd_stats_uv = best_rd_stats_uv;
  memcpy(x->blk_skip, best_blk_skip,
         sizeof(x->blk_skip[0]) * xd->n4_h * xd->n4_w);
  av1_copy_array(xd->tx_type_map, best_tx_type_map, xd->n4_h * xd->n4_w);
  x->force_skip = best_xskip;
  *disable_skip = best_disable_skip;

  restore_dst_buf(xd, *orig_dst, num_planes);
  return 0;
}

static int64_t skip_mode_rd(RD_STATS *rd_stats, const AV1_COMP *const cpi,
                            MACROBLOCK *const x, BLOCK_SIZE bsize,
                            const BUFFER_SET *const orig_dst) {
  assert(bsize < BLOCK_SIZES_ALL);
  const AV1_COMMON *cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  MACROBLOCKD *const xd = &x->e_mbd;
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, orig_dst, bsize, 0,
                                av1_num_planes(cm) - 1);

  int64_t total_sse = 0;
  for (int plane = 0; plane < num_planes; ++plane) {
    const struct macroblock_plane *const p = &x->plane[plane];
    const struct macroblockd_plane *const pd = &xd->plane[plane];
    const BLOCK_SIZE plane_bsize =
        get_plane_block_size(bsize, pd->subsampling_x, pd->subsampling_y);
    const int bw = block_size_wide[plane_bsize];
    const int bh = block_size_high[plane_bsize];

    av1_subtract_plane(x, plane_bsize, plane);
    int64_t sse = aom_sum_squares_2d_i16(p->src_diff, bw, bw, bh) << 4;
    total_sse += sse;
  }
  const int skip_mode_ctx = av1_get_skip_mode_context(xd);
  rd_stats->dist = rd_stats->sse = total_sse;
  rd_stats->rate = x->skip_mode_cost[skip_mode_ctx][1];
  rd_stats->rdcost = RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist);

  restore_dst_buf(xd, *orig_dst, num_planes);
  return 0;
}

static INLINE void get_this_mv(int_mv *this_mv, PREDICTION_MODE this_mode,
                               int ref_idx, int ref_mv_idx,
                               const MV_REFERENCE_FRAME *ref_frame,
                               const MB_MODE_INFO_EXT *mbmi_ext) {
  const int is_comp_pred = ref_frame[1] > INTRA_FRAME;
  const PREDICTION_MODE single_mode =
      get_single_mode(this_mode, ref_idx, is_comp_pred);
  assert(is_inter_singleref_mode(single_mode));
  if (single_mode == NEWMV) {
    this_mv->as_int = INVALID_MV;
  } else if (single_mode == GLOBALMV) {
    *this_mv = mbmi_ext->global_mvs[ref_frame[ref_idx]];
  } else {
    assert(single_mode == NEARMV || single_mode == NEARESTMV);
    const uint8_t ref_frame_type = av1_ref_frame_type(ref_frame);
    const int ref_mv_offset = single_mode == NEARESTMV ? 0 : ref_mv_idx + 1;
    if (ref_mv_offset < mbmi_ext->ref_mv_count[ref_frame_type]) {
      assert(ref_mv_offset >= 0);
      if (ref_idx == 0) {
        *this_mv =
            mbmi_ext->ref_mv_stack[ref_frame_type][ref_mv_offset].this_mv;
      } else {
        *this_mv =
            mbmi_ext->ref_mv_stack[ref_frame_type][ref_mv_offset].comp_mv;
      }
    } else {
      *this_mv = mbmi_ext->global_mvs[ref_frame[ref_idx]];
    }
  }
}

// This function update the non-new mv for the current prediction mode
static INLINE int build_cur_mv(int_mv *cur_mv, PREDICTION_MODE this_mode,
                               const AV1_COMMON *cm, const MACROBLOCK *x) {
  const MACROBLOCKD *xd = &x->e_mbd;
  const MB_MODE_INFO *mbmi = xd->mi[0];
  const int is_comp_pred = has_second_ref(mbmi);
  int ret = 1;
  for (int i = 0; i < is_comp_pred + 1; ++i) {
    int_mv this_mv;
    get_this_mv(&this_mv, this_mode, i, mbmi->ref_mv_idx, mbmi->ref_frame,
                x->mbmi_ext);
    const PREDICTION_MODE single_mode =
        get_single_mode(this_mode, i, is_comp_pred);
    if (single_mode == NEWMV) {
      const uint8_t ref_frame_type = av1_ref_frame_type(mbmi->ref_frame);
      cur_mv[i] =
          (i == 0) ? x->mbmi_ext->ref_mv_stack[ref_frame_type][mbmi->ref_mv_idx]
                         .this_mv
                   : x->mbmi_ext->ref_mv_stack[ref_frame_type][mbmi->ref_mv_idx]
                         .comp_mv;
    } else {
      ret &= clamp_and_check_mv(cur_mv + i, this_mv, cm, x);
    }
  }
  return ret;
}

static INLINE int get_drl_cost(const MB_MODE_INFO *mbmi,
                               const MB_MODE_INFO_EXT *mbmi_ext,
                               const int (*const drl_mode_cost0)[2],
                               int8_t ref_frame_type) {
  int cost = 0;
  if (mbmi->mode == NEWMV || mbmi->mode == NEW_NEWMV) {
    for (int idx = 0; idx < 2; ++idx) {
      if (mbmi_ext->ref_mv_count[ref_frame_type] > idx + 1) {
        uint8_t drl_ctx = av1_drl_ctx(mbmi_ext->weight[ref_frame_type], idx);
        cost += drl_mode_cost0[drl_ctx][mbmi->ref_mv_idx != idx];
        if (mbmi->ref_mv_idx == idx) return cost;
      }
    }
    return cost;
  }

  if (have_nearmv_in_inter_mode(mbmi->mode)) {
    for (int idx = 1; idx < 3; ++idx) {
      if (mbmi_ext->ref_mv_count[ref_frame_type] > idx + 1) {
        uint8_t drl_ctx = av1_drl_ctx(mbmi_ext->weight[ref_frame_type], idx);
        cost += drl_mode_cost0[drl_ctx][mbmi->ref_mv_idx != (idx - 1)];
        if (mbmi->ref_mv_idx == (idx - 1)) return cost;
      }
    }
    return cost;
  }
  return cost;
}

// Calculates the cost for compound type mask
static INLINE void calc_masked_type_cost(MACROBLOCK *x, BLOCK_SIZE bsize,
                                         int comp_group_idx_ctx,
                                         int comp_index_ctx,
                                         int masked_compound_used,
                                         int *masked_type_cost) {
  av1_zero_array(masked_type_cost, COMPOUND_TYPES);
  // Account for group index cost when wedge and/or diffwtd prediction are
  // enabled
  if (masked_compound_used) {
    // Compound group index of average and distwtd is 0
    // Compound group index of wedge and diffwtd is 1
    masked_type_cost[COMPOUND_AVERAGE] +=
        x->comp_group_idx_cost[comp_group_idx_ctx][0];
    masked_type_cost[COMPOUND_DISTWTD] += masked_type_cost[COMPOUND_AVERAGE];
    masked_type_cost[COMPOUND_WEDGE] +=
        x->comp_group_idx_cost[comp_group_idx_ctx][1];
    masked_type_cost[COMPOUND_DIFFWTD] += masked_type_cost[COMPOUND_WEDGE];
  }

  // Compute the cost to signal compound index/type
  masked_type_cost[COMPOUND_AVERAGE] += x->comp_idx_cost[comp_index_ctx][1];
  masked_type_cost[COMPOUND_DISTWTD] += x->comp_idx_cost[comp_index_ctx][0];
  masked_type_cost[COMPOUND_WEDGE] += x->compound_type_cost[bsize][0];
  masked_type_cost[COMPOUND_DIFFWTD] += x->compound_type_cost[bsize][1];
}

// Updates mbmi structure with the relevant compound type info
static INLINE void update_mbmi_for_compound_type(MB_MODE_INFO *mbmi,
                                                 COMPOUND_TYPE cur_type) {
  mbmi->interinter_comp.type = cur_type;
  mbmi->comp_group_idx = (cur_type >= COMPOUND_WEDGE);
  mbmi->compound_idx = (cur_type != COMPOUND_DISTWTD);
}

// When match is found, populate the compound type data
// and calculate the rd cost using the stored stats and
// update the mbmi appropriately.
static INLINE int populate_reuse_comp_type_data(
    const MACROBLOCK *x, MB_MODE_INFO *mbmi,
    BEST_COMP_TYPE_STATS *best_type_stats, int_mv *cur_mv, int32_t *comp_rate,
    int64_t *comp_dist, int *comp_rs2, int *rate_mv, int64_t *rd,
    int match_index) {
  const int winner_comp_type =
      x->comp_rd_stats[match_index].interinter_comp.type;
  if (comp_rate[winner_comp_type] == INT_MAX)
    return best_type_stats->best_compmode_interinter_cost;
  update_mbmi_for_compound_type(mbmi, winner_comp_type);
  mbmi->interinter_comp = x->comp_rd_stats[match_index].interinter_comp;
  *rd = RDCOST(
      x->rdmult,
      comp_rs2[winner_comp_type] + *rate_mv + comp_rate[winner_comp_type],
      comp_dist[winner_comp_type]);
  mbmi->mv[0].as_int = cur_mv[0].as_int;
  mbmi->mv[1].as_int = cur_mv[1].as_int;
  return comp_rs2[winner_comp_type];
}

// Updates rd cost and relevant compound type data for the best compound type
static INLINE void update_best_info(const MB_MODE_INFO *const mbmi, int64_t *rd,
                                    BEST_COMP_TYPE_STATS *best_type_stats,
                                    int64_t best_rd_cur,
                                    int64_t comp_model_rd_cur, int rs2) {
  *rd = best_rd_cur;
  best_type_stats->comp_best_model_rd = comp_model_rd_cur;
  best_type_stats->best_compound_data = mbmi->interinter_comp;
  best_type_stats->best_compmode_interinter_cost = rs2;
}

// Updates best_mv for masked compound types
static INLINE void update_mask_best_mv(const MB_MODE_INFO *const mbmi,
                                       int_mv *best_mv, int_mv *cur_mv,
                                       const COMPOUND_TYPE cur_type,
                                       int *best_tmp_rate_mv, int tmp_rate_mv,
                                       const SPEED_FEATURES *const sf) {
  if (cur_type == COMPOUND_WEDGE ||
      (sf->inter_sf.enable_interinter_diffwtd_newmv_search &&
       cur_type == COMPOUND_DIFFWTD)) {
    *best_tmp_rate_mv = tmp_rate_mv;
    best_mv[0].as_int = mbmi->mv[0].as_int;
    best_mv[1].as_int = mbmi->mv[1].as_int;
  } else {
    best_mv[0].as_int = cur_mv[0].as_int;
    best_mv[1].as_int = cur_mv[1].as_int;
  }
}

// Computes the valid compound_types to be evaluated
static INLINE int compute_valid_comp_types(
    MACROBLOCK *x, const AV1_COMP *const cpi, int *try_average_and_distwtd_comp,
    BLOCK_SIZE bsize, int masked_compound_used, int mode_search_mask,
    COMPOUND_TYPE *valid_comp_types) {
  const AV1_COMMON *cm = &cpi->common;
  int valid_type_count = 0;
  int comp_type, valid_check;
  int8_t enable_masked_type[MASKED_COMPOUND_TYPES] = { 0, 0 };

  const int try_average_comp = (mode_search_mask & (1 << COMPOUND_AVERAGE));
  const int try_distwtd_comp =
      ((mode_search_mask & (1 << COMPOUND_DISTWTD)) &&
       cm->seq_params.order_hint_info.enable_dist_wtd_comp == 1 &&
       cpi->sf.inter_sf.use_dist_wtd_comp_flag != DIST_WTD_COMP_DISABLED);
  *try_average_and_distwtd_comp = try_average_comp && try_distwtd_comp;

  // Check if COMPOUND_AVERAGE and COMPOUND_DISTWTD are valid cases
  for (comp_type = COMPOUND_AVERAGE; comp_type <= COMPOUND_DISTWTD;
       comp_type++) {
    valid_check =
        (comp_type == COMPOUND_AVERAGE) ? try_average_comp : try_distwtd_comp;
    if (!*try_average_and_distwtd_comp && valid_check &&
        is_interinter_compound_used(comp_type, bsize))
      valid_comp_types[valid_type_count++] = comp_type;
  }
  // Check if COMPOUND_WEDGE and COMPOUND_DIFFWTD are valid cases
  if (masked_compound_used) {
    // enable_masked_type[0] corresponds to COMPOUND_WEDGE
    // enable_masked_type[1] corresponds to COMPOUND_DIFFWTD
    enable_masked_type[0] = enable_wedge_interinter_search(x, cpi);
    enable_masked_type[1] = cpi->oxcf.enable_diff_wtd_comp;
    for (comp_type = COMPOUND_WEDGE; comp_type <= COMPOUND_DIFFWTD;
         comp_type++) {
      if ((mode_search_mask & (1 << comp_type)) &&
          is_interinter_compound_used(comp_type, bsize) &&
          enable_masked_type[comp_type - COMPOUND_WEDGE])
        valid_comp_types[valid_type_count++] = comp_type;
    }
  }
  return valid_type_count;
}

// Choose the better of the two COMPOUND_AVERAGE,
// COMPOUND_DISTWTD based on modeled cost
static int find_best_avg_distwtd_comp_type(MACROBLOCK *x, int *comp_model_rate,
                                           int64_t *comp_model_dist,
                                           int rate_mv, int64_t *best_rd) {
  int64_t est_rd[2];
  est_rd[COMPOUND_AVERAGE] =
      RDCOST(x->rdmult, comp_model_rate[COMPOUND_AVERAGE] + rate_mv,
             comp_model_dist[COMPOUND_AVERAGE]);
  est_rd[COMPOUND_DISTWTD] =
      RDCOST(x->rdmult, comp_model_rate[COMPOUND_DISTWTD] + rate_mv,
             comp_model_dist[COMPOUND_DISTWTD]);
  int best_type = (est_rd[COMPOUND_AVERAGE] <= est_rd[COMPOUND_DISTWTD])
                      ? COMPOUND_AVERAGE
                      : COMPOUND_DISTWTD;
  *best_rd = est_rd[best_type];
  return best_type;
}

static int compound_type_rd(
    const AV1_COMP *const cpi, MACROBLOCK *x, BLOCK_SIZE bsize, int_mv *cur_mv,
    int mode_search_mask, int masked_compound_used, const BUFFER_SET *orig_dst,
    const BUFFER_SET *tmp_dst, const CompoundTypeRdBuffers *buffers,
    int *rate_mv, int64_t *rd, RD_STATS *rd_stats, int64_t ref_best_rd,
    int *is_luma_interp_done, int64_t rd_thresh) {
  const AV1_COMMON *cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  const PREDICTION_MODE this_mode = mbmi->mode;
  const int bw = block_size_wide[bsize];
  int rs2;
  int_mv best_mv[2];
  int best_tmp_rate_mv = *rate_mv;
  BEST_COMP_TYPE_STATS best_type_stats;
  // Initializing BEST_COMP_TYPE_STATS
  best_type_stats.best_compound_data.type = COMPOUND_AVERAGE;
  best_type_stats.best_compmode_interinter_cost = 0;
  best_type_stats.comp_best_model_rd = INT64_MAX;

  uint8_t *preds0[1] = { buffers->pred0 };
  uint8_t *preds1[1] = { buffers->pred1 };
  int strides[1] = { bw };
  int tmp_rate_mv;
  const int num_pix = 1 << num_pels_log2_lookup[bsize];
  const int mask_len = 2 * num_pix * sizeof(uint8_t);
  COMPOUND_TYPE cur_type;
  // Local array to store the mask cost for different compound types
  int masked_type_cost[COMPOUND_TYPES];

  int calc_pred_masked_compound = 1;
  int64_t comp_dist[COMPOUND_TYPES] = { INT64_MAX, INT64_MAX, INT64_MAX,
                                        INT64_MAX };
  int32_t comp_rate[COMPOUND_TYPES] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX };
  int comp_rs2[COMPOUND_TYPES] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX };
  int32_t comp_model_rate[COMPOUND_TYPES] = { INT_MAX, INT_MAX, INT_MAX,
                                              INT_MAX };
  int64_t comp_model_dist[COMPOUND_TYPES] = { INT64_MAX, INT64_MAX, INT64_MAX,
                                              INT64_MAX };
  int match_index = 0;
  const int match_found =
      find_comp_rd_in_stats(cpi, x, mbmi, comp_rate, comp_dist, comp_model_rate,
                            comp_model_dist, comp_rs2, &match_index);
  best_mv[0].as_int = cur_mv[0].as_int;
  best_mv[1].as_int = cur_mv[1].as_int;
  *rd = INT64_MAX;
  int rate_sum, tmp_skip_txfm_sb;
  int64_t dist_sum, tmp_skip_sse_sb;

  // Local array to store the valid compound types to be evaluated in the core
  // loop
  COMPOUND_TYPE valid_comp_types[COMPOUND_TYPES] = {
    COMPOUND_AVERAGE, COMPOUND_DISTWTD, COMPOUND_WEDGE, COMPOUND_DIFFWTD
  };
  int valid_type_count = 0;
  int try_average_and_distwtd_comp = 0;
  // compute_valid_comp_types() returns the number of valid compound types to be
  // evaluated and populates the same in the local array valid_comp_types[].
  // It also sets the flag 'try_average_and_distwtd_comp'
  valid_type_count = compute_valid_comp_types(
      x, cpi, &try_average_and_distwtd_comp, bsize, masked_compound_used,
      mode_search_mask, valid_comp_types);

  // The following context indices are independent of compound type
  const int comp_group_idx_ctx = get_comp_group_idx_context(xd);
  const int comp_index_ctx = get_comp_index_context(cm, xd);

  // Populates masked_type_cost local array for the 4 compound types
  calc_masked_type_cost(x, bsize, comp_group_idx_ctx, comp_index_ctx,
                        masked_compound_used, masked_type_cost);

  int64_t comp_model_rd_cur = INT64_MAX;
  int64_t best_rd_cur = INT64_MAX;
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;

  // If the match is found, calculate the rd cost using the
  // stored stats and update the mbmi appropriately.
  if (match_found && cpi->sf.inter_sf.reuse_compound_type_decision) {
    return populate_reuse_comp_type_data(x, mbmi, &best_type_stats, cur_mv,
                                         comp_rate, comp_dist, comp_rs2,
                                         rate_mv, rd, match_index);
  }
  // Special handling if both compound_average and compound_distwtd
  // are to be searched. In this case, first estimate between the two
  // modes and then call estimate_yrd_for_sb() only for the better of
  // the two.
  if (try_average_and_distwtd_comp) {
    int est_rate[2];
    int64_t est_dist[2], est_rd;
    COMPOUND_TYPE best_type;
    // Since modelled rate and dist are separately stored,
    // compute better of COMPOUND_AVERAGE and COMPOUND_DISTWTD
    // using the stored stats.
    if ((comp_model_rate[COMPOUND_AVERAGE] != INT_MAX) &&
        comp_model_rate[COMPOUND_DISTWTD] != INT_MAX) {
      // Choose the better of the COMPOUND_AVERAGE,
      // COMPOUND_DISTWTD on modeled cost.
      best_type = find_best_avg_distwtd_comp_type(
          x, comp_model_rate, comp_model_dist, *rate_mv, &est_rd);
      update_mbmi_for_compound_type(mbmi, best_type);
      if (comp_rate[best_type] != INT_MAX)
        best_rd_cur = RDCOST(
            x->rdmult,
            masked_type_cost[best_type] + *rate_mv + comp_rate[best_type],
            comp_dist[best_type]);
      comp_model_rd_cur = est_rd;
      // Update stats for best compound type
      if (best_rd_cur < *rd) {
        update_best_info(mbmi, rd, &best_type_stats, best_rd_cur,
                         comp_model_rd_cur, masked_type_cost[best_type]);
      }
      restore_dst_buf(xd, *tmp_dst, 1);
    } else {
      // Calculate model_rd for COMPOUND_AVERAGE and COMPOUND_DISTWTD
      for (int comp_type = COMPOUND_AVERAGE; comp_type <= COMPOUND_DISTWTD;
           comp_type++) {
        update_mbmi_for_compound_type(mbmi, comp_type);
        av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, orig_dst, bsize,
                                      AOM_PLANE_Y, AOM_PLANE_Y);
        model_rd_sb_fn[MODELRD_CURVFIT](
            cpi, bsize, x, xd, 0, 0, &est_rate[comp_type], &est_dist[comp_type],
            NULL, NULL, NULL, NULL, NULL);
        est_rate[comp_type] += masked_type_cost[comp_type];
        comp_model_rate[comp_type] = est_rate[comp_type];
        comp_model_dist[comp_type] = est_dist[comp_type];
        if (comp_type == COMPOUND_AVERAGE) {
          *is_luma_interp_done = 1;
          restore_dst_buf(xd, *tmp_dst, 1);
        }
      }
      // Choose the better of the two based on modeled cost and call
      // estimate_yrd_for_sb() for that one.
      best_type = find_best_avg_distwtd_comp_type(
          x, comp_model_rate, comp_model_dist, *rate_mv, &est_rd);
      update_mbmi_for_compound_type(mbmi, best_type);
      if (best_type == COMPOUND_AVERAGE) restore_dst_buf(xd, *orig_dst, 1);
      rs2 = masked_type_cost[best_type];
      RD_STATS est_rd_stats;
      const int64_t mode_rd = RDCOST(x->rdmult, rs2 + *rate_mv, 0);
      const int64_t tmp_rd_thresh = AOMMIN(*rd, rd_thresh) - mode_rd;
      const int64_t est_rd_ =
          estimate_yrd_for_sb(cpi, bsize, x, tmp_rd_thresh, &est_rd_stats);

      if (est_rd_ != INT64_MAX) {
        best_rd_cur = RDCOST(x->rdmult, rs2 + *rate_mv + est_rd_stats.rate,
                             est_rd_stats.dist);
        // Backup rate and distortion for future reuse
        backup_stats(best_type, comp_rate, comp_dist, comp_model_rate,
                     comp_model_dist, est_rate[best_type], est_dist[best_type],
                     &est_rd_stats, comp_rs2, rs2);
        comp_model_rd_cur = est_rd;
      }
      if (best_type == COMPOUND_AVERAGE) restore_dst_buf(xd, *tmp_dst, 1);
      // Update stats for best compound type
      if (best_rd_cur < *rd) {
        update_best_info(mbmi, rd, &best_type_stats, best_rd_cur,
                         comp_model_rd_cur, rs2);
      }
    }
  }

  // If COMPOUND_AVERAGE is not valid, use the spare buffer
  if (valid_comp_types[0] != COMPOUND_AVERAGE) restore_dst_buf(xd, *tmp_dst, 1);

  // Loop over valid compound types
  for (int i = 0; i < valid_type_count; i++) {
    cur_type = valid_comp_types[i];
    comp_model_rd_cur = INT64_MAX;
    tmp_rate_mv = *rate_mv;
    best_rd_cur = INT64_MAX;

    // Case COMPOUND_AVERAGE and COMPOUND_DISTWTD
    if (cur_type < COMPOUND_WEDGE) {
      update_mbmi_for_compound_type(mbmi, cur_type);
      rs2 = masked_type_cost[cur_type];
      const int64_t mode_rd = RDCOST(x->rdmult, rs2 + rd_stats->rate, 0);
      if (mode_rd < ref_best_rd) {
        // Reuse data if matching record is found
        if (comp_rate[cur_type] == INT_MAX) {
          av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, orig_dst, bsize,
                                        AOM_PLANE_Y, AOM_PLANE_Y);
          if (cur_type == COMPOUND_AVERAGE) *is_luma_interp_done = 1;

          // Compute RD cost for the current type
          RD_STATS est_rd_stats;
          const int64_t tmp_rd_thresh = AOMMIN(*rd, rd_thresh) - mode_rd;
          const int64_t est_rd =
              estimate_yrd_for_sb(cpi, bsize, x, tmp_rd_thresh, &est_rd_stats);
          if (est_rd != INT64_MAX) {
            best_rd_cur = RDCOST(x->rdmult, rs2 + *rate_mv + est_rd_stats.rate,
                                 est_rd_stats.dist);
            model_rd_sb_fn[MODELRD_TYPE_MASKED_COMPOUND](
                cpi, bsize, x, xd, 0, 0, &rate_sum, &dist_sum,
                &tmp_skip_txfm_sb, &tmp_skip_sse_sb, NULL, NULL, NULL);
            comp_model_rd_cur =
                RDCOST(x->rdmult, rs2 + *rate_mv + rate_sum, dist_sum);

            // Backup rate and distortion for future reuse
            backup_stats(cur_type, comp_rate, comp_dist, comp_model_rate,
                         comp_model_dist, rate_sum, dist_sum, &est_rd_stats,
                         comp_rs2, rs2);
          }
        } else {
          // Calculate RD cost based on stored stats
          assert(comp_dist[cur_type] != INT64_MAX);
          best_rd_cur = RDCOST(x->rdmult, rs2 + *rate_mv + comp_rate[cur_type],
                               comp_dist[cur_type]);
          // Recalculate model rdcost with the updated rate
          comp_model_rd_cur =
              RDCOST(x->rdmult, rs2 + *rate_mv + comp_model_rate[cur_type],
                     comp_model_dist[cur_type]);
        }
      }
      // use spare buffer for following compound type try
      if (cur_type == COMPOUND_AVERAGE) restore_dst_buf(xd, *tmp_dst, 1);
    } else {
      // Handle masked compound types
      update_mbmi_for_compound_type(mbmi, cur_type);
      rs2 = masked_type_cost[cur_type];
      // Evaluate COMPOUND_WEDGE / COMPOUND_DIFFWTD if approximated cost is
      // within threshold
      int64_t approx_rd = ((*rd / cpi->max_comp_type_rd_threshold_div) *
                           cpi->max_comp_type_rd_threshold_mul);

      if (approx_rd < ref_best_rd) {
        const int64_t tmp_rd_thresh = AOMMIN(*rd, rd_thresh);
        best_rd_cur = masked_compound_type_rd(
            cpi, x, cur_mv, bsize, this_mode, &rs2, *rate_mv, orig_dst,
            &tmp_rate_mv, preds0, preds1, buffers->residual1, buffers->diff10,
            strides, rd_stats->rate, tmp_rd_thresh, &calc_pred_masked_compound,
            comp_rate, comp_dist, comp_model_rate, comp_model_dist,
            best_type_stats.comp_best_model_rd, &comp_model_rd_cur, comp_rs2);
      }
    }
    // Update stats for best compound type
    if (best_rd_cur < *rd) {
      update_best_info(mbmi, rd, &best_type_stats, best_rd_cur,
                       comp_model_rd_cur, rs2);
      if (masked_compound_used && cur_type >= COMPOUND_WEDGE) {
        memcpy(buffers->tmp_best_mask_buf, xd->seg_mask, mask_len);
        if (have_newmv_in_inter_mode(this_mode))
          update_mask_best_mv(mbmi, best_mv, cur_mv, cur_type,
                              &best_tmp_rate_mv, tmp_rate_mv, &cpi->sf);
      }
    }
    // reset to original mvs for next iteration
    mbmi->mv[0].as_int = cur_mv[0].as_int;
    mbmi->mv[1].as_int = cur_mv[1].as_int;
  }
  if (mbmi->interinter_comp.type != best_type_stats.best_compound_data.type) {
    mbmi->comp_group_idx =
        (best_type_stats.best_compound_data.type < COMPOUND_WEDGE) ? 0 : 1;
    mbmi->compound_idx =
        !(best_type_stats.best_compound_data.type == COMPOUND_DISTWTD);
    mbmi->interinter_comp = best_type_stats.best_compound_data;
    memcpy(xd->seg_mask, buffers->tmp_best_mask_buf, mask_len);
  }
  if (have_newmv_in_inter_mode(this_mode)) {
    mbmi->mv[0].as_int = best_mv[0].as_int;
    mbmi->mv[1].as_int = best_mv[1].as_int;
    if (mbmi->interinter_comp.type == COMPOUND_WEDGE) {
      rd_stats->rate += best_tmp_rate_mv - *rate_mv;
      *rate_mv = best_tmp_rate_mv;
    }
  }
  restore_dst_buf(xd, *orig_dst, 1);
  if (!match_found)
    save_comp_rd_search_stat(x, mbmi, comp_rate, comp_dist, comp_model_rate,
                             comp_model_dist, cur_mv, comp_rs2);
  return best_type_stats.best_compmode_interinter_cost;
}

static INLINE int is_single_newmv_valid(const HandleInterModeArgs *const args,
                                        const MB_MODE_INFO *const mbmi,
                                        PREDICTION_MODE this_mode) {
  for (int ref_idx = 0; ref_idx < 2; ++ref_idx) {
    const PREDICTION_MODE single_mode = get_single_mode(this_mode, ref_idx, 1);
    const MV_REFERENCE_FRAME ref = mbmi->ref_frame[ref_idx];
    if (single_mode == NEWMV &&
        args->single_newmv_valid[mbmi->ref_mv_idx][ref] == 0) {
      return 0;
    }
  }
  return 1;
}

static int get_drl_refmv_count(const MACROBLOCK *const x,
                               const MV_REFERENCE_FRAME *ref_frame,
                               PREDICTION_MODE mode) {
  MB_MODE_INFO_EXT *const mbmi_ext = x->mbmi_ext;
  const int8_t ref_frame_type = av1_ref_frame_type(ref_frame);
  const int has_nearmv = have_nearmv_in_inter_mode(mode) ? 1 : 0;
  const int ref_mv_count = mbmi_ext->ref_mv_count[ref_frame_type];
  const int only_newmv = (mode == NEWMV || mode == NEW_NEWMV);
  const int has_drl =
      (has_nearmv && ref_mv_count > 2) || (only_newmv && ref_mv_count > 1);
  const int ref_set =
      has_drl ? AOMMIN(MAX_REF_MV_SEARCH, ref_mv_count - has_nearmv) : 1;

  return ref_set;
}

// Whether this reference motion vector can be skipped, based on initial
// heuristics.
static bool ref_mv_idx_early_breakout(const AV1_COMP *const cpi, MACROBLOCK *x,
                                      const HandleInterModeArgs *const args,
                                      int64_t ref_best_rd, int ref_mv_idx) {
  const SPEED_FEATURES *const sf = &cpi->sf;
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  const MB_MODE_INFO_EXT *const mbmi_ext = x->mbmi_ext;
  const int8_t ref_frame_type = av1_ref_frame_type(mbmi->ref_frame);
  const int is_comp_pred = has_second_ref(mbmi);
  if (sf->inter_sf.reduce_inter_modes && ref_mv_idx > 0) {
    if (mbmi->ref_frame[0] == LAST2_FRAME ||
        mbmi->ref_frame[0] == LAST3_FRAME ||
        mbmi->ref_frame[1] == LAST2_FRAME ||
        mbmi->ref_frame[1] == LAST3_FRAME) {
      const int has_nearmv = have_nearmv_in_inter_mode(mbmi->mode) ? 1 : 0;
      if (mbmi_ext->weight[ref_frame_type][ref_mv_idx + has_nearmv] <
          REF_CAT_LEVEL) {
        return true;
      }
    }
    // TODO(any): Experiment with reduce_inter_modes for compound prediction
    if (sf->inter_sf.reduce_inter_modes >= 2 && !is_comp_pred &&
        have_newmv_in_inter_mode(mbmi->mode)) {
      if (mbmi->ref_frame[0] != cpi->nearest_past_ref &&
          mbmi->ref_frame[0] != cpi->nearest_future_ref) {
        const int has_nearmv = have_nearmv_in_inter_mode(mbmi->mode) ? 1 : 0;
        if (mbmi_ext->weight[ref_frame_type][ref_mv_idx + has_nearmv] <
            REF_CAT_LEVEL) {
          return true;
        }
      }
    }
  }
  if (sf->inter_sf.prune_single_motion_modes_by_simple_trans && !is_comp_pred &&
      args->single_ref_first_pass == 0) {
    if (args->simple_rd_state[ref_mv_idx].early_skipped) {
      return true;
    }
  }
  mbmi->ref_mv_idx = ref_mv_idx;
  if (is_comp_pred && (!is_single_newmv_valid(args, mbmi, mbmi->mode))) {
    return true;
  }
  size_t est_rd_rate = args->ref_frame_cost + args->single_comp_cost;
  const int drl_cost =
      get_drl_cost(mbmi, mbmi_ext, x->drl_mode_cost0, ref_frame_type);
  est_rd_rate += drl_cost;
  if (RDCOST(x->rdmult, est_rd_rate, 0) > ref_best_rd &&
      mbmi->mode != NEARESTMV && mbmi->mode != NEAREST_NEARESTMV) {
    return true;
  }
  return false;
}

typedef struct {
  int64_t rd;
  int drl_cost;
  int rate_mv;
  int_mv mv;
} inter_mode_info;

// Compute the estimated RD cost for the motion vector with simple translation.
static int64_t simple_translation_pred_rd(
    AV1_COMP *const cpi, MACROBLOCK *x, RD_STATS *rd_stats,
    HandleInterModeArgs *args, int ref_mv_idx, inter_mode_info *mode_info,
    int64_t ref_best_rd, BLOCK_SIZE bsize) {
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  MB_MODE_INFO_EXT *const mbmi_ext = x->mbmi_ext;
  const int8_t ref_frame_type = av1_ref_frame_type(mbmi->ref_frame);
  const AV1_COMMON *cm = &cpi->common;
  const int is_comp_pred = has_second_ref(mbmi);

  struct macroblockd_plane *p = xd->plane;
  const BUFFER_SET orig_dst = {
    { p[0].dst.buf, p[1].dst.buf, p[2].dst.buf },
    { p[0].dst.stride, p[1].dst.stride, p[2].dst.stride },
  };
  av1_init_rd_stats(rd_stats);

  mbmi->interinter_comp.type = COMPOUND_AVERAGE;
  mbmi->comp_group_idx = 0;
  mbmi->compound_idx = 1;
  if (mbmi->ref_frame[1] == INTRA_FRAME) {
    mbmi->ref_frame[1] = NONE_FRAME;
  }
  int16_t mode_ctx =
      av1_mode_context_analyzer(mbmi_ext->mode_context, mbmi->ref_frame);

  mbmi->num_proj_ref = 0;
  mbmi->motion_mode = SIMPLE_TRANSLATION;
  mbmi->ref_mv_idx = ref_mv_idx;

  rd_stats->rate += args->ref_frame_cost + args->single_comp_cost;
  const int drl_cost =
      get_drl_cost(mbmi, mbmi_ext, x->drl_mode_cost0, ref_frame_type);
  rd_stats->rate += drl_cost;
  mode_info[ref_mv_idx].drl_cost = drl_cost;

  int_mv cur_mv[2];
  if (!build_cur_mv(cur_mv, mbmi->mode, cm, x)) {
    return INT64_MAX;
  }
  assert(have_nearmv_in_inter_mode(mbmi->mode));
  for (int i = 0; i < is_comp_pred + 1; ++i) {
    mbmi->mv[i].as_int = cur_mv[i].as_int;
  }
  const int ref_mv_cost = cost_mv_ref(x, mbmi->mode, mode_ctx);
  rd_stats->rate += ref_mv_cost;

  if (RDCOST(x->rdmult, rd_stats->rate, 0) > ref_best_rd) {
    return INT64_MAX;
  }

  mbmi->motion_mode = SIMPLE_TRANSLATION;
  mbmi->num_proj_ref = 0;
  if (is_comp_pred) {
    // Only compound_average
    mbmi->interinter_comp.type = COMPOUND_AVERAGE;
    mbmi->comp_group_idx = 0;
    mbmi->compound_idx = 1;
  }
  set_default_interp_filters(mbmi, cm->interp_filter);

  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, &orig_dst, bsize,
                                AOM_PLANE_Y, AOM_PLANE_Y);
  int est_rate;
  int64_t est_dist;
  model_rd_sb_fn[MODELRD_CURVFIT](cpi, bsize, x, xd, 0, 0, &est_rate, &est_dist,
                                  NULL, NULL, NULL, NULL, NULL);
  return RDCOST(x->rdmult, rd_stats->rate + est_rate, est_dist);
}

// Represents a set of integers, from 0 to sizeof(int) * 8, as bits in
// an integer. 0 for the i-th bit means that integer is excluded, 1 means
// it is included.
static INLINE void mask_set_bit(int *mask, int index) { *mask |= (1 << index); }

static INLINE bool mask_check_bit(int mask, int index) {
  return (mask >> index) & 0x1;
}

// Before performing the full MV search in handle_inter_mode, do a simple
// translation search and see if we can eliminate any motion vectors.
// Returns an integer where, if the i-th bit is set, it means that the i-th
// motion vector should be searched. This is only set for NEAR_MV.
static int ref_mv_idx_to_search(AV1_COMP *const cpi, MACROBLOCK *x,
                                RD_STATS *rd_stats,
                                HandleInterModeArgs *const args,
                                int64_t ref_best_rd, inter_mode_info *mode_info,
                                BLOCK_SIZE bsize, const int ref_set) {
  AV1_COMMON *const cm = &cpi->common;
  const MACROBLOCKD *const xd = &x->e_mbd;
  const MB_MODE_INFO *const mbmi = xd->mi[0];
  const PREDICTION_MODE this_mode = mbmi->mode;

  // Only search indices if they have some chance of being good.
  int good_indices = 0;
  for (int i = 0; i < ref_set; ++i) {
    if (ref_mv_idx_early_breakout(cpi, x, args, ref_best_rd, i)) {
      continue;
    }
    mask_set_bit(&good_indices, i);
  }

  // Only prune in NEARMV mode, if the speed feature is set, and the block size
  // is large enough. If these conditions are not met, return all good indices
  // found so far.
  if (!cpi->sf.inter_sf.prune_mode_search_simple_translation)
    return good_indices;
  if (!have_nearmv_in_inter_mode(this_mode)) return good_indices;
  if (num_pels_log2_lookup[bsize] <= 6) return good_indices;
  // Do not prune when there is internal resizing. TODO(elliottk) fix this
  // so b/2384 can be resolved.
  if (av1_is_scaled(get_ref_scale_factors(cm, mbmi->ref_frame[0])) ||
      (mbmi->ref_frame[1] > 0 &&
       av1_is_scaled(get_ref_scale_factors(cm, mbmi->ref_frame[1])))) {
    return good_indices;
  }

  // Calculate the RD cost for the motion vectors using simple translation.
  int64_t idx_rdcost[] = { INT64_MAX, INT64_MAX, INT64_MAX };
  for (int ref_mv_idx = 0; ref_mv_idx < ref_set; ++ref_mv_idx) {
    // If this index is bad, ignore it.
    if (!mask_check_bit(good_indices, ref_mv_idx)) {
      continue;
    }
    idx_rdcost[ref_mv_idx] = simple_translation_pred_rd(
        cpi, x, rd_stats, args, ref_mv_idx, mode_info, ref_best_rd, bsize);
  }
  // Find the index with the best RD cost.
  int best_idx = 0;
  for (int i = 1; i < MAX_REF_MV_SEARCH; ++i) {
    if (idx_rdcost[i] < idx_rdcost[best_idx]) {
      best_idx = i;
    }
  }
  // Only include indices that are good and within a % of the best.
  const double dth = has_second_ref(mbmi) ? 1.05 : 1.001;
  // If the simple translation cost is not within this multiple of the
  // best RD, skip it. Note that the cutoff is derived experimentally.
  const double ref_dth = 5;
  int result = 0;
  for (int i = 0; i < ref_set; ++i) {
    if (mask_check_bit(good_indices, i) &&
        (1.0 * idx_rdcost[i]) / idx_rdcost[best_idx] < dth &&
        (1.0 * idx_rdcost[i]) / ref_best_rd < ref_dth) {
      mask_set_bit(&result, i);
    }
  }
  return result;
}

typedef struct motion_mode_candidate {
  MB_MODE_INFO mbmi;
  int rate_mv;
  int rate2_nocoeff;
  int skip_motion_mode;
  int64_t rd_cost;
} motion_mode_candidate;

typedef struct motion_mode_best_st_candidate {
  motion_mode_candidate motion_mode_cand[MAX_WINNER_MOTION_MODES];
  int num_motion_mode_cand;
} motion_mode_best_st_candidate;

static int64_t handle_inter_mode(AV1_COMP *const cpi, TileDataEnc *tile_data,
                                 MACROBLOCK *x, BLOCK_SIZE bsize,
                                 RD_STATS *rd_stats, RD_STATS *rd_stats_y,
                                 RD_STATS *rd_stats_uv, int *disable_skip,
                                 HandleInterModeArgs *args, int64_t ref_best_rd,
                                 uint8_t *const tmp_buf,
                                 const CompoundTypeRdBuffers *rd_buffers,
                                 int64_t *best_est_rd, const int do_tx_search,
                                 InterModesInfo *inter_modes_info,
                                 motion_mode_candidate *motion_mode_cand) {
  const AV1_COMMON *cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = xd->mi[0];
  MB_MODE_INFO_EXT *const mbmi_ext = x->mbmi_ext;
  const int is_comp_pred = has_second_ref(mbmi);
  const PREDICTION_MODE this_mode = mbmi->mode;
  int i;
  const int refs[2] = { mbmi->ref_frame[0],
                        (mbmi->ref_frame[1] < 0 ? 0 : mbmi->ref_frame[1]) };
  int rate_mv = 0;
  int64_t rd = INT64_MAX;
  // do first prediction into the destination buffer. Do the next
  // prediction into a temporary buffer. Then keep track of which one
  // of these currently holds the best predictor, and use the other
  // one for future predictions. In the end, copy from tmp_buf to
  // dst if necessary.
  struct macroblockd_plane *p = xd->plane;
  const BUFFER_SET orig_dst = {
    { p[0].dst.buf, p[1].dst.buf, p[2].dst.buf },
    { p[0].dst.stride, p[1].dst.stride, p[2].dst.stride },
  };
  const BUFFER_SET tmp_dst = { { tmp_buf, tmp_buf + 1 * MAX_SB_SQUARE,
                                 tmp_buf + 2 * MAX_SB_SQUARE },
                               { MAX_SB_SIZE, MAX_SB_SIZE, MAX_SB_SIZE } };

  const int masked_compound_used = is_any_masked_compound_used(bsize) &&
                                   cm->seq_params.enable_masked_compound;
  int64_t ret_val = INT64_MAX;
  const int8_t ref_frame_type = av1_ref_frame_type(mbmi->ref_frame);
  RD_STATS best_rd_stats, best_rd_stats_y, best_rd_stats_uv;
  int64_t best_rd = INT64_MAX;
  uint8_t best_blk_skip[MAX_MIB_SIZE * MAX_MIB_SIZE];
  uint8_t best_tx_type_map[MAX_MIB_SIZE * MAX_MIB_SIZE];
  MB_MODE_INFO best_mbmi = *mbmi;
  int best_disable_skip = 0;
  int best_xskip = 0;
  int64_t newmv_ret_val = INT64_MAX;
  inter_mode_info mode_info[MAX_REF_MV_SEARCH];

  int mode_search_mask = (1 << COMPOUND_AVERAGE) | (1 << COMPOUND_DISTWTD) |
                         (1 << COMPOUND_WEDGE) | (1 << COMPOUND_DIFFWTD);

  // First, perform a simple translation search for each of the indices. If
  // an index performs well, it will be fully searched here.
  const int ref_set = get_drl_refmv_count(x, mbmi->ref_frame, this_mode);
  // Save MV results from first 2 ref_mv_idx.
  int_mv save_mv[MAX_REF_MV_SEARCH - 1][2] = { { { 0 } } };
  int best_ref_mv_idx = -1;
  const int idx_mask = ref_mv_idx_to_search(cpi, x, rd_stats, args, ref_best_rd,
                                            mode_info, bsize, ref_set);
  const int16_t mode_ctx =
      av1_mode_context_analyzer(mbmi_ext->mode_context, mbmi->ref_frame);
  const int ref_mv_cost = cost_mv_ref(x, this_mode, mode_ctx);
  const int base_rate =
      args->ref_frame_cost + args->single_comp_cost + ref_mv_cost;
  for (int ref_mv_idx = 0; ref_mv_idx < ref_set; ++ref_mv_idx) {
    mode_info[ref_mv_idx].mv.as_int = INVALID_MV;
    mode_info[ref_mv_idx].rd = INT64_MAX;
    if (!mask_check_bit(idx_mask, ref_mv_idx)) {
      // MV did not perform well in simple translation search. Skip it.
      continue;
    }
    av1_init_rd_stats(rd_stats);

    mbmi->interinter_comp.type = COMPOUND_AVERAGE;
    mbmi->comp_group_idx = 0;
    mbmi->compound_idx = 1;
    if (mbmi->ref_frame[1] == INTRA_FRAME) mbmi->ref_frame[1] = NONE_FRAME;

    mbmi->num_proj_ref = 0;
    mbmi->motion_mode = SIMPLE_TRANSLATION;
    mbmi->ref_mv_idx = ref_mv_idx;

    rd_stats->rate = base_rate;
    const int drl_cost =
        get_drl_cost(mbmi, mbmi_ext, x->drl_mode_cost0, ref_frame_type);
    rd_stats->rate += drl_cost;
    mode_info[ref_mv_idx].drl_cost = drl_cost;

    int rs = 0;
    int compmode_interinter_cost = 0;

    int_mv cur_mv[2];
    if (!build_cur_mv(cur_mv, this_mode, cm, x)) {
      continue;
    }

    if (have_newmv_in_inter_mode(this_mode)) {
#if CONFIG_COLLECT_COMPONENT_TIMING
      start_timing(cpi, handle_newmv_time);
#endif
      if (cpi->sf.inter_sf.prune_single_motion_modes_by_simple_trans &&
          args->single_ref_first_pass == 0 && !is_comp_pred) {
        const int ref0 = mbmi->ref_frame[0];
        newmv_ret_val = args->single_newmv_valid[ref_mv_idx][ref0] ? 0 : 1;
        cur_mv[0] = args->single_newmv[ref_mv_idx][ref0];
        rate_mv = args->single_newmv_rate[ref_mv_idx][ref0];
      } else {
        newmv_ret_val = handle_newmv(cpi, x, bsize, cur_mv, &rate_mv, args);
      }
#if CONFIG_COLLECT_COMPONENT_TIMING
      end_timing(cpi, handle_newmv_time);
#endif

      if (newmv_ret_val != 0) continue;

      rd_stats->rate += rate_mv;

      if (cpi->sf.inter_sf.skip_repeated_newmv) {
        if (!is_comp_pred && this_mode == NEWMV && ref_mv_idx > 0) {
          int skip = 0;
          int this_rate_mv = 0;
          for (i = 0; i < ref_mv_idx; ++i) {
            // Check if the motion search result same as previous results
            if (cur_mv[0].as_int == args->single_newmv[i][refs[0]].as_int &&
                args->single_newmv_valid[i][refs[0]]) {
              // If the compared mode has no valid rd, it is unlikely this
              // mode will be the best mode
              if (mode_info[i].rd == INT64_MAX) {
                skip = 1;
                break;
              }
              // Compare the cost difference including drl cost and mv cost
              if (mode_info[i].mv.as_int != INVALID_MV) {
                const int compare_cost =
                    mode_info[i].rate_mv + mode_info[i].drl_cost;
                const int_mv ref_mv = av1_get_ref_mv(x, 0);
                this_rate_mv = av1_mv_bit_cost(
                    &mode_info[i].mv.as_mv, &ref_mv.as_mv, x->nmv_vec_cost,
                    x->mv_cost_stack, MV_COST_WEIGHT);
                const int this_cost = this_rate_mv + drl_cost;

                if (compare_cost <= this_cost) {
                  skip = 1;
                  break;
                } else {
                  // If the cost is less than current best result, make this
                  // the best and update corresponding variables unless the
                  // best_mv is the same as ref_mv. In this case we skip and
                  // rely on NEAR(EST)MV instead
                  if (best_mbmi.ref_mv_idx == i &&
                      mode_info[i].mv.as_int != ref_mv.as_int) {
                    assert(best_rd != INT64_MAX);
                    best_mbmi.ref_mv_idx = ref_mv_idx;
                    motion_mode_cand->rate_mv = this_rate_mv;
                    best_rd_stats.rate += this_cost - compare_cost;
                    best_rd = RDCOST(x->rdmult, best_rd_stats.rate,
                                     best_rd_stats.dist);
                    if (best_rd < ref_best_rd) ref_best_rd = best_rd;
                    skip = 1;
                    break;
                  }
                }
              }
            }
          }
          if (skip) {
            const THR_MODES mode_enum = get_prediction_mode_idx(
                best_mbmi.mode, best_mbmi.ref_frame[0], best_mbmi.ref_frame[1]);
            // Collect mode stats for multiwinner mode processing
            store_winner_mode_stats(
                &cpi->common, x, &best_mbmi, &best_rd_stats, &best_rd_stats_y,
                &best_rd_stats_uv, mode_enum, NULL, bsize, best_rd,
                cpi->sf.winner_mode_sf.enable_multiwinner_mode_process,
                do_tx_search);
            args->modelled_rd[this_mode][ref_mv_idx][refs[0]] =
                args->modelled_rd[this_mode][i][refs[0]];
            args->simple_rd[this_mode][ref_mv_idx][refs[0]] =
                args->simple_rd[this_mode][i][refs[0]];
            mode_info[ref_mv_idx].rd = mode_info[i].rd;
            mode_info[ref_mv_idx].rate_mv = this_rate_mv;
            mode_info[ref_mv_idx].mv.as_int = mode_info[i].mv.as_int;

            restore_dst_buf(xd, orig_dst, num_planes);
            continue;
          }
        }
      }
    }
    for (i = 0; i < is_comp_pred + 1; ++i) {
      mbmi->mv[i].as_int = cur_mv[i].as_int;
    }

    if (RDCOST(x->rdmult, rd_stats->rate, 0) > ref_best_rd &&
        mbmi->mode != NEARESTMV && mbmi->mode != NEAREST_NEARESTMV) {
      continue;
    }

    if (cpi->sf.inter_sf.prune_ref_mv_idx_search && is_comp_pred) {
      // TODO(yunqing): Move this part to a separate function when it is done.
      // Store MV result.
      if (ref_mv_idx < MAX_REF_MV_SEARCH - 1) {
        for (i = 0; i < is_comp_pred + 1; ++i)
          save_mv[ref_mv_idx][i].as_int = mbmi->mv[i].as_int;
      }
      // Skip the evaluation if an MV match is found.
      if (ref_mv_idx > 0) {
        int match = 0;
        for (int idx = 0; idx < ref_mv_idx; ++idx) {
          int mv_diff = 0;
          for (i = 0; i < 1 + is_comp_pred; ++i) {
            mv_diff += abs(save_mv[idx][i].as_mv.row - mbmi->mv[i].as_mv.row) +
                       abs(save_mv[idx][i].as_mv.col - mbmi->mv[i].as_mv.col);
          }

          // If this mode is not the best one, and current MV is similar to
          // previous stored MV, terminate this ref_mv_idx evaluation.
          if (best_ref_mv_idx == -1 && mv_diff < 1) {
            match = 1;
            break;
          }
        }
        if (match == 1) continue;
      }
    }

#if CONFIG_COLLECT_COMPONENT_TIMING
    start_timing(cpi, compound_type_rd_time);
#endif
    int skip_build_pred = 0;
    const int mi_row = xd->mi_row;
    const int mi_col = xd->mi_col;
    if (is_comp_pred) {
      // Find matching interp filter or set to default interp filter
      const int need_search = av1_is_interp_needed(xd);
      const InterpFilter assign_filter = cm->interp_filter;
      int is_luma_interp_done = 0;
      find_interp_filter_match(mbmi, cpi, assign_filter, need_search,
                               args->interp_filter_stats,
                               args->interp_filter_stats_idx);

      int64_t best_rd_compound;
      int64_t rd_thresh;
      const int comp_type_rd_shift = COMP_TYPE_RD_THRESH_SHIFT;
      const int comp_type_rd_scale = COMP_TYPE_RD_THRESH_SCALE;
      rd_thresh = get_rd_thresh_from_best_rd(
          ref_best_rd, (1 << comp_type_rd_shift), comp_type_rd_scale);
      compmode_interinter_cost = compound_type_rd(
          cpi, x, bsize, cur_mv, mode_search_mask, masked_compound_used,
          &orig_dst, &tmp_dst, rd_buffers, &rate_mv, &best_rd_compound,
          rd_stats, ref_best_rd, &is_luma_interp_done, rd_thresh);
      if (ref_best_rd < INT64_MAX &&
          (best_rd_compound >> comp_type_rd_shift) * comp_type_rd_scale >
              ref_best_rd) {
        restore_dst_buf(xd, orig_dst, num_planes);
        continue;
      }
      // No need to call av1_enc_build_inter_predictor for luma if
      // COMPOUND_AVERAGE is selected because it is the first
      // candidate in compound_type_rd, and the following
      // compound types searching uses tmp_dst buffer

      if (mbmi->interinter_comp.type == COMPOUND_AVERAGE &&
          is_luma_interp_done) {
        if (num_planes > 1) {
          av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, &orig_dst,
                                        bsize, AOM_PLANE_U, num_planes - 1);
        }
        skip_build_pred = 1;
      }
    }

#if CONFIG_COLLECT_COMPONENT_TIMING
    end_timing(cpi, compound_type_rd_time);
#endif

#if CONFIG_COLLECT_COMPONENT_TIMING
    start_timing(cpi, interpolation_filter_search_time);
#endif
    ret_val = interpolation_filter_search(x, cpi, tile_data, bsize, &tmp_dst,
                                          &orig_dst, &rd, &rs, &skip_build_pred,
                                          args, ref_best_rd);
#if CONFIG_COLLECT_COMPONENT_TIMING
    end_timing(cpi, interpolation_filter_search_time);
#endif
    if (args->modelled_rd != NULL && !is_comp_pred) {
      args->modelled_rd[this_mode][ref_mv_idx][refs[0]] = rd;
    }
    if (ret_val != 0) {
      restore_dst_buf(xd, orig_dst, num_planes);
      continue;
    } else if (cpi->sf.inter_sf.model_based_post_interp_filter_breakout &&
               ref_best_rd != INT64_MAX && (rd >> 3) * 3 > ref_best_rd) {
      restore_dst_buf(xd, orig_dst, num_planes);
      continue;
    }

    if (args->modelled_rd != NULL) {
      if (is_comp_pred) {
        const int mode0 = compound_ref0_mode(this_mode);
        const int mode1 = compound_ref1_mode(this_mode);
        const int64_t mrd =
            AOMMIN(args->modelled_rd[mode0][ref_mv_idx][refs[0]],
                   args->modelled_rd[mode1][ref_mv_idx][refs[1]]);
        if ((rd >> 3) * 6 > mrd && ref_best_rd < INT64_MAX) {
          restore_dst_buf(xd, orig_dst, num_planes);
          continue;
        }
      }
    }
    rd_stats->rate += compmode_interinter_cost;
    if (skip_build_pred != 1) {
      av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, &orig_dst, bsize, 0,
                                    av1_num_planes(cm) - 1);
    }

#if CONFIG_COLLECT_COMPONENT_TIMING
    start_timing(cpi, motion_mode_rd_time);
#endif
    int rate2_nocoeff = rd_stats->rate;
    ret_val = motion_mode_rd(cpi, tile_data, x, bsize, rd_stats, rd_stats_y,
                             rd_stats_uv, disable_skip, args, ref_best_rd,
                             &rate_mv, &orig_dst, best_est_rd, do_tx_search,
                             inter_modes_info, 0);
#if CONFIG_COLLECT_COMPONENT_TIMING
    end_timing(cpi, motion_mode_rd_time);
#endif

    mode_info[ref_mv_idx].mv.as_int = mbmi->mv[0].as_int;
    mode_info[ref_mv_idx].rate_mv = rate_mv;
    if (ret_val != INT64_MAX) {
      int64_t tmp_rd = RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist);
      mode_info[ref_mv_idx].rd = tmp_rd;
      const THR_MODES mode_enum = get_prediction_mode_idx(
          mbmi->mode, mbmi->ref_frame[0], mbmi->ref_frame[1]);
      // Collect mode stats for multiwinner mode processing
      store_winner_mode_stats(
          &cpi->common, x, mbmi, rd_stats, rd_stats_y, rd_stats_uv, mode_enum,
          NULL, bsize, tmp_rd,
          cpi->sf.winner_mode_sf.enable_multiwinner_mode_process, do_tx_search);
      if (tmp_rd < best_rd) {
        best_rd_stats = *rd_stats;
        best_rd_stats_y = *rd_stats_y;
        best_rd_stats_uv = *rd_stats_uv;
        best_rd = tmp_rd;
        best_mbmi = *mbmi;
        best_disable_skip = *disable_skip;
        best_xskip = x->force_skip;
        memcpy(best_blk_skip, x->blk_skip,
               sizeof(best_blk_skip[0]) * xd->n4_h * xd->n4_w);
        av1_copy_array(best_tx_type_map, xd->tx_type_map, xd->n4_h * xd->n4_w);
        motion_mode_cand->rate_mv = rate_mv;
        motion_mode_cand->rate2_nocoeff = rate2_nocoeff;
      }

      if (tmp_rd < ref_best_rd) {
        ref_best_rd = tmp_rd;
        best_ref_mv_idx = ref_mv_idx;
      }
    }
    restore_dst_buf(xd, orig_dst, num_planes);
  }

  if (best_rd == INT64_MAX) return INT64_MAX;

  // re-instate status of the best choice
  *rd_stats = best_rd_stats;
  *rd_stats_y = best_rd_stats_y;
  *rd_stats_uv = best_rd_stats_uv;
  *mbmi = best_mbmi;
  *disable_skip = best_disable_skip;
  x->force_skip = best_xskip;
  assert(IMPLIES(mbmi->comp_group_idx == 1,
                 mbmi->interinter_comp.type != COMPOUND_AVERAGE));
  memcpy(x->blk_skip, best_blk_skip,
         sizeof(best_blk_skip[0]) * xd->n4_h * xd->n4_w);
  av1_copy_array(xd->tx_type_map, best_tx_type_map, xd->n4_h * xd->n4_w);

  rd_stats->rdcost = RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist);

  return rd_stats->rdcost;
}

static int64_t rd_pick_intrabc_mode_sb(const AV1_COMP *cpi, MACROBLOCK *x,
                                       PICK_MODE_CONTEXT *ctx,
                                       RD_STATS *rd_stats, BLOCK_SIZE bsize,
                                       int64_t best_rd) {
  const AV1_COMMON *const cm = &cpi->common;
  if (!av1_allow_intrabc(cm) || !cpi->oxcf.enable_intrabc) return INT64_MAX;
  const int num_planes = av1_num_planes(cm);

  MACROBLOCKD *const xd = &x->e_mbd;
  const TileInfo *tile = &xd->tile;
  MB_MODE_INFO *mbmi = xd->mi[0];
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  const int w = block_size_wide[bsize];
  const int h = block_size_high[bsize];
  const int sb_row = mi_row >> cm->seq_params.mib_size_log2;
  const int sb_col = mi_col >> cm->seq_params.mib_size_log2;

  MB_MODE_INFO_EXT *const mbmi_ext = x->mbmi_ext;
  MV_REFERENCE_FRAME ref_frame = INTRA_FRAME;
  av1_find_mv_refs(cm, xd, mbmi, ref_frame, mbmi_ext->ref_mv_count,
                   xd->ref_mv_stack, xd->weight, NULL, mbmi_ext->global_mvs,
                   mbmi_ext->mode_context);
  // TODO(Ravi): Populate mbmi_ext->ref_mv_stack[ref_frame][4] and
  // mbmi_ext->weight[ref_frame][4] inside av1_find_mv_refs.
  av1_copy_usable_ref_mv_stack_and_weight(xd, mbmi_ext, ref_frame);
  int_mv nearestmv, nearmv;
  av1_find_best_ref_mvs_from_stack(0, mbmi_ext, ref_frame, &nearestmv, &nearmv,
                                   0);

  if (nearestmv.as_int == INVALID_MV) {
    nearestmv.as_int = 0;
  }
  if (nearmv.as_int == INVALID_MV) {
    nearmv.as_int = 0;
  }

  int_mv dv_ref = nearestmv.as_int == 0 ? nearmv : nearestmv;
  if (dv_ref.as_int == 0) {
    av1_find_ref_dv(&dv_ref, tile, cm->seq_params.mib_size, mi_row, mi_col);
  }
  // Ref DV should not have sub-pel.
  assert((dv_ref.as_mv.col & 7) == 0);
  assert((dv_ref.as_mv.row & 7) == 0);
  mbmi_ext->ref_mv_stack[INTRA_FRAME][0].this_mv = dv_ref;

  struct buf_2d yv12_mb[MAX_MB_PLANE];
  av1_setup_pred_block(xd, yv12_mb, xd->cur_buf, NULL, NULL, num_planes);
  for (int i = 0; i < num_planes; ++i) {
    xd->plane[i].pre[0] = yv12_mb[i];
  }

  enum IntrabcMotionDirection {
    IBC_MOTION_ABOVE,
    IBC_MOTION_LEFT,
    IBC_MOTION_DIRECTIONS
  };

  MB_MODE_INFO best_mbmi = *mbmi;
  RD_STATS best_rdstats = *rd_stats;
  uint8_t best_blk_skip[MAX_MIB_SIZE * MAX_MIB_SIZE] = { 0 };
  uint8_t best_tx_type_map[MAX_MIB_SIZE * MAX_MIB_SIZE];
  av1_copy_array(best_tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);

  for (enum IntrabcMotionDirection dir = IBC_MOTION_ABOVE;
       dir < IBC_MOTION_DIRECTIONS; ++dir) {
    const MvLimits tmp_mv_limits = x->mv_limits;
    switch (dir) {
      case IBC_MOTION_ABOVE:
        x->mv_limits.col_min = (tile->mi_col_start - mi_col) * MI_SIZE;
        x->mv_limits.col_max = (tile->mi_col_end - mi_col) * MI_SIZE - w;
        x->mv_limits.row_min = (tile->mi_row_start - mi_row) * MI_SIZE;
        x->mv_limits.row_max =
            (sb_row * cm->seq_params.mib_size - mi_row) * MI_SIZE - h;
        break;
      case IBC_MOTION_LEFT:
        x->mv_limits.col_min = (tile->mi_col_start - mi_col) * MI_SIZE;
        x->mv_limits.col_max =
            (sb_col * cm->seq_params.mib_size - mi_col) * MI_SIZE - w;
        // TODO(aconverse@google.com): Minimize the overlap between above and
        // left areas.
        x->mv_limits.row_min = (tile->mi_row_start - mi_row) * MI_SIZE;
        int bottom_coded_mi_edge =
            AOMMIN((sb_row + 1) * cm->seq_params.mib_size, tile->mi_row_end);
        x->mv_limits.row_max = (bottom_coded_mi_edge - mi_row) * MI_SIZE - h;
        break;
      default: assert(0);
    }
    assert(x->mv_limits.col_min >= tmp_mv_limits.col_min);
    assert(x->mv_limits.col_max <= tmp_mv_limits.col_max);
    assert(x->mv_limits.row_min >= tmp_mv_limits.row_min);
    assert(x->mv_limits.row_max <= tmp_mv_limits.row_max);
    av1_set_mv_search_range(&x->mv_limits, &dv_ref.as_mv);

    if (x->mv_limits.col_max < x->mv_limits.col_min ||
        x->mv_limits.row_max < x->mv_limits.row_min) {
      x->mv_limits = tmp_mv_limits;
      continue;
    }

    int step_param = cpi->mv_step_param;
    MV mvp_full = dv_ref.as_mv;
    mvp_full.col >>= 3;
    mvp_full.row >>= 3;
    const int sadpb = x->sadperbit16;
    int cost_list[5];
    const int bestsme = av1_full_pixel_search(
        cpi, x, bsize, &mvp_full, step_param, 1, cpi->sf.mv_sf.search_method, 0,
        sadpb, cond_cost_list(cpi, cost_list), &dv_ref.as_mv, INT_MAX, 1,
        (MI_SIZE * mi_col), (MI_SIZE * mi_row), 1,
        &cpi->ss_cfg[SS_CFG_LOOKAHEAD], 1);

    x->mv_limits = tmp_mv_limits;
    if (bestsme == INT_MAX) continue;
    mvp_full = x->best_mv.as_mv;
    const MV dv = { .row = mvp_full.row * 8, .col = mvp_full.col * 8 };
    if (mv_check_bounds(&x->mv_limits, &dv)) continue;
    if (!av1_is_dv_valid(dv, cm, xd, mi_row, mi_col, bsize,
                         cm->seq_params.mib_size_log2))
      continue;

    // DV should not have sub-pel.
    assert((dv.col & 7) == 0);
    assert((dv.row & 7) == 0);
    memset(&mbmi->palette_mode_info, 0, sizeof(mbmi->palette_mode_info));
    mbmi->filter_intra_mode_info.use_filter_intra = 0;
    mbmi->use_intrabc = 1;
    mbmi->mode = DC_PRED;
    mbmi->uv_mode = UV_DC_PRED;
    mbmi->motion_mode = SIMPLE_TRANSLATION;
    mbmi->mv[0].as_mv = dv;
    mbmi->interp_filters = av1_broadcast_interp_filter(BILINEAR);
    mbmi->skip = 0;
    av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, NULL, bsize, 0,
                                  av1_num_planes(cm) - 1);

    int *dvcost[2] = { (int *)&cpi->dv_cost[0][MV_MAX],
                       (int *)&cpi->dv_cost[1][MV_MAX] };
    // TODO(aconverse@google.com): The full motion field defining discount
    // in MV_COST_WEIGHT is too large. Explore other values.
    const int rate_mv = av1_mv_bit_cost(&dv, &dv_ref.as_mv, cpi->dv_joint_cost,
                                        dvcost, MV_COST_WEIGHT_SUB);
    const int rate_mode = x->intrabc_cost[1];
    RD_STATS rd_stats_yuv, rd_stats_y, rd_stats_uv;
    if (!txfm_search(cpi, NULL, x, bsize, &rd_stats_yuv, &rd_stats_y,
                     &rd_stats_uv, rate_mode + rate_mv, INT64_MAX))
      continue;
    rd_stats_yuv.rdcost =
        RDCOST(x->rdmult, rd_stats_yuv.rate, rd_stats_yuv.dist);
    if (rd_stats_yuv.rdcost < best_rd) {
      best_rd = rd_stats_yuv.rdcost;
      best_mbmi = *mbmi;
      best_rdstats = rd_stats_yuv;
      memcpy(best_blk_skip, x->blk_skip,
             sizeof(x->blk_skip[0]) * xd->n4_h * xd->n4_w);
      av1_copy_array(best_tx_type_map, xd->tx_type_map, xd->n4_h * xd->n4_w);
    }
  }
  *mbmi = best_mbmi;
  *rd_stats = best_rdstats;
  memcpy(x->blk_skip, best_blk_skip,
         sizeof(x->blk_skip[0]) * xd->n4_h * xd->n4_w);
  av1_copy_array(xd->tx_type_map, best_tx_type_map, ctx->num_4x4_blk);
#if CONFIG_RD_DEBUG
  mbmi->rd_stats = *rd_stats;
#endif
  return best_rd;
}

void av1_rd_pick_intra_mode_sb(const AV1_COMP *cpi, MACROBLOCK *x,
                               RD_STATS *rd_cost, BLOCK_SIZE bsize,
                               PICK_MODE_CONTEXT *ctx, int64_t best_rd) {
  const AV1_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  const int num_planes = av1_num_planes(cm);
  int rate_y = 0, rate_uv = 0, rate_y_tokenonly = 0, rate_uv_tokenonly = 0;
  int y_skip = 0, uv_skip = 0;
  int64_t dist_y = 0, dist_uv = 0;
  TX_SIZE max_uv_tx_size;

  ctx->rd_stats.skip = 0;
  mbmi->ref_frame[0] = INTRA_FRAME;
  mbmi->ref_frame[1] = NONE_FRAME;
  mbmi->use_intrabc = 0;
  mbmi->mv[0].as_int = 0;
  mbmi->skip_mode = 0;

  const int64_t intra_yrd =
      rd_pick_intra_sby_mode(cpi, x, &rate_y, &rate_y_tokenonly, &dist_y,
                             &y_skip, bsize, best_rd, ctx);

  // Initialize default mode evaluation params
  set_mode_eval_params(cpi, x, DEFAULT_EVAL);

  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  if (intra_yrd < best_rd) {
    // Only store reconstructed luma when there's chroma RDO. When there's no
    // chroma RDO, the reconstructed luma will be stored in encode_superblock().
    xd->cfl.is_chroma_reference =
        is_chroma_reference(mi_row, mi_col, bsize, cm->seq_params.subsampling_x,
                            cm->seq_params.subsampling_y);
    xd->cfl.store_y = store_cfl_required_rdo(cm, x);
    if (xd->cfl.store_y) {
      // Restore reconstructed luma values.
      memcpy(x->blk_skip, ctx->blk_skip,
             sizeof(x->blk_skip[0]) * ctx->num_4x4_blk);
      av1_copy_array(xd->tx_type_map, ctx->tx_type_map, ctx->num_4x4_blk);
      av1_encode_intra_block_plane(cpi, x, bsize, AOM_PLANE_Y,
                                   cpi->optimize_seg_arr[mbmi->segment_id]);
      av1_copy_array(ctx->tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);
      xd->cfl.store_y = 0;
    }
    if (num_planes > 1) {
      max_uv_tx_size = av1_get_tx_size(AOM_PLANE_U, xd);
      init_sbuv_mode(mbmi);
      if (!x->skip_chroma_rd)
        rd_pick_intra_sbuv_mode(cpi, x, &rate_uv, &rate_uv_tokenonly, &dist_uv,
                                &uv_skip, bsize, max_uv_tx_size);
    }

    // Intra block is always coded as non-skip
    rd_cost->rate =
        rate_y + rate_uv + x->skip_cost[av1_get_skip_context(xd)][0];
    rd_cost->dist = dist_y + dist_uv;
    rd_cost->rdcost = RDCOST(x->rdmult, rd_cost->rate, rd_cost->dist);
    rd_cost->skip = 0;
  } else {
    rd_cost->rate = INT_MAX;
  }

  if (rd_cost->rate != INT_MAX && rd_cost->rdcost < best_rd)
    best_rd = rd_cost->rdcost;
  if (rd_pick_intrabc_mode_sb(cpi, x, ctx, rd_cost, bsize, best_rd) < best_rd) {
    ctx->rd_stats.skip = mbmi->skip;
    memcpy(ctx->blk_skip, x->blk_skip,
           sizeof(x->blk_skip[0]) * ctx->num_4x4_blk);
    assert(rd_cost->rate != INT_MAX);
  }
  if (rd_cost->rate == INT_MAX) return;

  ctx->mic = *xd->mi[0];
  ctx->mbmi_ext = *x->mbmi_ext;
  av1_copy_array(ctx->tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);
}

static AOM_INLINE void restore_uv_color_map(const AV1_COMP *const cpi,
                                            MACROBLOCK *x) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  PALETTE_MODE_INFO *const pmi = &mbmi->palette_mode_info;
  const BLOCK_SIZE bsize = mbmi->sb_type;
  int src_stride = x->plane[1].src.stride;
  const uint8_t *const src_u = x->plane[1].src.buf;
  const uint8_t *const src_v = x->plane[2].src.buf;
  int *const data = x->palette_buffer->kmeans_data_buf;
  int centroids[2 * PALETTE_MAX_SIZE];
  uint8_t *const color_map = xd->plane[1].color_index_map;
  int r, c;
  const uint16_t *const src_u16 = CONVERT_TO_SHORTPTR(src_u);
  const uint16_t *const src_v16 = CONVERT_TO_SHORTPTR(src_v);
  int plane_block_width, plane_block_height, rows, cols;
  av1_get_block_dimensions(bsize, 1, xd, &plane_block_width,
                           &plane_block_height, &rows, &cols);

  for (r = 0; r < rows; ++r) {
    for (c = 0; c < cols; ++c) {
      if (cpi->common.seq_params.use_highbitdepth) {
        data[(r * cols + c) * 2] = src_u16[r * src_stride + c];
        data[(r * cols + c) * 2 + 1] = src_v16[r * src_stride + c];
      } else {
        data[(r * cols + c) * 2] = src_u[r * src_stride + c];
        data[(r * cols + c) * 2 + 1] = src_v[r * src_stride + c];
      }
    }
  }

  for (r = 1; r < 3; ++r) {
    for (c = 0; c < pmi->palette_size[1]; ++c) {
      centroids[c * 2 + r - 1] = pmi->palette_colors[r * PALETTE_MAX_SIZE + c];
    }
  }

  av1_calc_indices(data, centroids, color_map, rows * cols,
                   pmi->palette_size[1], 2);
  extend_palette_color_map(color_map, cols, rows, plane_block_width,
                           plane_block_height);
}

static AOM_INLINE void calc_target_weighted_pred(
    const AV1_COMMON *cm, const MACROBLOCK *x, const MACROBLOCKD *xd,
    const uint8_t *above, int above_stride, const uint8_t *left,
    int left_stride);

static AOM_INLINE void rd_pick_skip_mode(
    RD_STATS *rd_cost, InterModeSearchState *search_state,
    const AV1_COMP *const cpi, MACROBLOCK *const x, BLOCK_SIZE bsize,
    struct buf_2d yv12_mb[REF_FRAMES][MAX_MB_PLANE]) {
  const AV1_COMMON *const cm = &cpi->common;
  const SkipModeInfo *const skip_mode_info = &cm->current_frame.skip_mode_info;
  const int num_planes = av1_num_planes(cm);
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];

  x->compound_idx = 1;  // COMPOUND_AVERAGE
  RD_STATS skip_mode_rd_stats;
  av1_invalid_rd_stats(&skip_mode_rd_stats);

  if (skip_mode_info->ref_frame_idx_0 == INVALID_IDX ||
      skip_mode_info->ref_frame_idx_1 == INVALID_IDX) {
    return;
  }

  const MV_REFERENCE_FRAME ref_frame =
      LAST_FRAME + skip_mode_info->ref_frame_idx_0;
  const MV_REFERENCE_FRAME second_ref_frame =
      LAST_FRAME + skip_mode_info->ref_frame_idx_1;
  const PREDICTION_MODE this_mode = NEAREST_NEARESTMV;
  const THR_MODES mode_index =
      get_prediction_mode_idx(this_mode, ref_frame, second_ref_frame);

  if (mode_index == THR_INVALID) {
    return;
  }

  if ((!cpi->oxcf.enable_onesided_comp ||
       cpi->sf.inter_sf.disable_onesided_comp) &&
      cpi->all_one_sided_refs) {
    return;
  }

  mbmi->mode = this_mode;
  mbmi->uv_mode = UV_DC_PRED;
  mbmi->ref_frame[0] = ref_frame;
  mbmi->ref_frame[1] = second_ref_frame;
  const uint8_t ref_frame_type = av1_ref_frame_type(mbmi->ref_frame);
  if (x->mbmi_ext->ref_mv_count[ref_frame_type] == UINT8_MAX) {
    if (x->mbmi_ext->ref_mv_count[ref_frame] == UINT8_MAX ||
        x->mbmi_ext->ref_mv_count[second_ref_frame] == UINT8_MAX) {
      return;
    }
    MB_MODE_INFO_EXT *mbmi_ext = x->mbmi_ext;
    av1_find_mv_refs(cm, xd, mbmi, ref_frame_type, mbmi_ext->ref_mv_count,
                     xd->ref_mv_stack, xd->weight, NULL, mbmi_ext->global_mvs,
                     mbmi_ext->mode_context);
    // TODO(Ravi): Populate mbmi_ext->ref_mv_stack[ref_frame][4] and
    // mbmi_ext->weight[ref_frame][4] inside av1_find_mv_refs.
    av1_copy_usable_ref_mv_stack_and_weight(xd, mbmi_ext, ref_frame_type);
  }

  assert(this_mode == NEAREST_NEARESTMV);
  if (!build_cur_mv(mbmi->mv, this_mode, cm, x)) {
    return;
  }

  mbmi->filter_intra_mode_info.use_filter_intra = 0;
  mbmi->interintra_mode = (INTERINTRA_MODE)(II_DC_PRED - 1);
  mbmi->comp_group_idx = 0;
  mbmi->compound_idx = x->compound_idx;
  mbmi->interinter_comp.type = COMPOUND_AVERAGE;
  mbmi->motion_mode = SIMPLE_TRANSLATION;
  mbmi->ref_mv_idx = 0;
  mbmi->skip_mode = mbmi->skip = 1;

  set_default_interp_filters(mbmi, cm->interp_filter);

  set_ref_ptrs(cm, xd, mbmi->ref_frame[0], mbmi->ref_frame[1]);
  for (int i = 0; i < num_planes; i++) {
    xd->plane[i].pre[0] = yv12_mb[mbmi->ref_frame[0]][i];
    xd->plane[i].pre[1] = yv12_mb[mbmi->ref_frame[1]][i];
  }

  BUFFER_SET orig_dst;
  for (int i = 0; i < num_planes; i++) {
    orig_dst.plane[i] = xd->plane[i].dst.buf;
    orig_dst.stride[i] = xd->plane[i].dst.stride;
  }

  // Obtain the rdcost for skip_mode.
  skip_mode_rd(&skip_mode_rd_stats, cpi, x, bsize, &orig_dst);

  // Compare the use of skip_mode with the best intra/inter mode obtained.
  const int skip_mode_ctx = av1_get_skip_mode_context(xd);
  int64_t best_intra_inter_mode_cost = INT64_MAX;
  if (rd_cost->dist < INT64_MAX && rd_cost->rate < INT32_MAX) {
    best_intra_inter_mode_cost =
        RDCOST(x->rdmult, rd_cost->rate + x->skip_mode_cost[skip_mode_ctx][0],
               rd_cost->dist);
    // Account for non-skip mode rate in total rd stats
    rd_cost->rate += x->skip_mode_cost[skip_mode_ctx][0];
    av1_rd_cost_update(x->rdmult, rd_cost);
  }

  if (skip_mode_rd_stats.rdcost <= best_intra_inter_mode_cost &&
      (!xd->lossless[mbmi->segment_id] || skip_mode_rd_stats.dist == 0)) {
    assert(mode_index != THR_INVALID);
    search_state->best_mbmode.skip_mode = 1;
    search_state->best_mbmode = *mbmi;

    search_state->best_mbmode.skip_mode = search_state->best_mbmode.skip = 1;
    search_state->best_mbmode.mode = NEAREST_NEARESTMV;
    search_state->best_mbmode.ref_frame[0] = mbmi->ref_frame[0];
    search_state->best_mbmode.ref_frame[1] = mbmi->ref_frame[1];
    search_state->best_mbmode.mv[0].as_int = mbmi->mv[0].as_int;
    search_state->best_mbmode.mv[1].as_int = mbmi->mv[1].as_int;
    search_state->best_mbmode.ref_mv_idx = 0;

    // Set up tx_size related variables for skip-specific loop filtering.
    search_state->best_mbmode.tx_size =
        block_signals_txsize(bsize)
            ? tx_size_from_tx_mode(bsize, x->tx_mode_search_type)
            : max_txsize_rect_lookup[bsize];
    memset(search_state->best_mbmode.inter_tx_size,
           search_state->best_mbmode.tx_size,
           sizeof(search_state->best_mbmode.inter_tx_size));
    set_txfm_ctxs(search_state->best_mbmode.tx_size, xd->n4_w, xd->n4_h,
                  search_state->best_mbmode.skip && is_inter_block(mbmi), xd);

    // Set up color-related variables for skip mode.
    search_state->best_mbmode.uv_mode = UV_DC_PRED;
    search_state->best_mbmode.palette_mode_info.palette_size[0] = 0;
    search_state->best_mbmode.palette_mode_info.palette_size[1] = 0;

    search_state->best_mbmode.comp_group_idx = 0;
    search_state->best_mbmode.compound_idx = x->compound_idx;
    search_state->best_mbmode.interinter_comp.type = COMPOUND_AVERAGE;
    search_state->best_mbmode.motion_mode = SIMPLE_TRANSLATION;

    search_state->best_mbmode.interintra_mode =
        (INTERINTRA_MODE)(II_DC_PRED - 1);
    search_state->best_mbmode.filter_intra_mode_info.use_filter_intra = 0;

    set_default_interp_filters(&search_state->best_mbmode, cm->interp_filter);

    search_state->best_mode_index = mode_index;

    // Update rd_cost
    rd_cost->rate = skip_mode_rd_stats.rate;
    rd_cost->dist = rd_cost->sse = skip_mode_rd_stats.dist;
    rd_cost->rdcost = skip_mode_rd_stats.rdcost;

    search_state->best_rd = rd_cost->rdcost;
    search_state->best_skip2 = 1;
    search_state->best_mode_skippable = 1;

    x->force_skip = 1;
  }
}

// Get winner mode stats of given mode index
static AOM_INLINE MB_MODE_INFO *get_winner_mode_stats(
    MACROBLOCK *x, MB_MODE_INFO *best_mbmode, RD_STATS *best_rd_cost,
    int best_rate_y, int best_rate_uv, THR_MODES *best_mode_index,
    RD_STATS **winner_rd_cost, int *winner_rate_y, int *winner_rate_uv,
    THR_MODES *winner_mode_index, int enable_multiwinner_mode_process,
    int mode_idx) {
  MB_MODE_INFO *winner_mbmi;
  if (enable_multiwinner_mode_process) {
    assert(mode_idx >= 0 && mode_idx < x->winner_mode_count);
    WinnerModeStats *winner_mode_stat = &x->winner_mode_stats[mode_idx];
    winner_mbmi = &winner_mode_stat->mbmi;

    *winner_rd_cost = &winner_mode_stat->rd_cost;
    *winner_rate_y = winner_mode_stat->rate_y;
    *winner_rate_uv = winner_mode_stat->rate_uv;
    *winner_mode_index = winner_mode_stat->mode_index;
  } else {
    winner_mbmi = best_mbmode;
    *winner_rd_cost = best_rd_cost;
    *winner_rate_y = best_rate_y;
    *winner_rate_uv = best_rate_uv;
    *winner_mode_index = *best_mode_index;
  }
  return winner_mbmi;
}

// speed feature: fast intra/inter transform type search
// Used for speed >= 2
// When this speed feature is on, in rd mode search, only DCT is used.
// After the mode is determined, this function is called, to select
// transform types and get accurate rdcost.
static AOM_INLINE void refine_winner_mode_tx(
    const AV1_COMP *cpi, MACROBLOCK *x, RD_STATS *rd_cost, BLOCK_SIZE bsize,
    PICK_MODE_CONTEXT *ctx, THR_MODES *best_mode_index,
    MB_MODE_INFO *best_mbmode, struct buf_2d yv12_mb[REF_FRAMES][MAX_MB_PLANE],
    int best_rate_y, int best_rate_uv, int *best_skip2, int winner_mode_count) {
  const AV1_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  int64_t best_rd;
  const int num_planes = av1_num_planes(cm);

  if (!is_winner_mode_processing_enabled(cpi, best_mbmode, best_mbmode->mode))
    return;

  // Set params for winner mode evaluation
  set_mode_eval_params(cpi, x, WINNER_MODE_EVAL);

  // No best mode identified so far
  if (*best_mode_index == THR_INVALID) return;

  best_rd = RDCOST(x->rdmult, rd_cost->rate, rd_cost->dist);
  for (int mode_idx = 0; mode_idx < winner_mode_count; mode_idx++) {
    RD_STATS *winner_rd_stats = NULL;
    int winner_rate_y = 0, winner_rate_uv = 0;
    THR_MODES winner_mode_index = 0;

    // TODO(any): Combine best mode and multi-winner mode processing paths
    // Get winner mode stats for current mode index
    MB_MODE_INFO *winner_mbmi = get_winner_mode_stats(
        x, best_mbmode, rd_cost, best_rate_y, best_rate_uv, best_mode_index,
        &winner_rd_stats, &winner_rate_y, &winner_rate_uv, &winner_mode_index,
        cpi->sf.winner_mode_sf.enable_multiwinner_mode_process, mode_idx);

    if (xd->lossless[winner_mbmi->segment_id] == 0 &&
        winner_mode_index != THR_INVALID &&
        is_winner_mode_processing_enabled(cpi, winner_mbmi,
                                          winner_mbmi->mode)) {
      RD_STATS rd_stats = *winner_rd_stats;
      int skip_blk = 0;
      RD_STATS rd_stats_y, rd_stats_uv;
      const int skip_ctx = av1_get_skip_context(xd);

      *mbmi = *winner_mbmi;

      set_ref_ptrs(cm, xd, mbmi->ref_frame[0], mbmi->ref_frame[1]);

      // Select prediction reference frames.
      for (int i = 0; i < num_planes; i++) {
        xd->plane[i].pre[0] = yv12_mb[mbmi->ref_frame[0]][i];
        if (has_second_ref(mbmi))
          xd->plane[i].pre[1] = yv12_mb[mbmi->ref_frame[1]][i];
      }

      if (is_inter_mode(mbmi->mode)) {
        const int mi_row = xd->mi_row;
        const int mi_col = xd->mi_col;
        av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, NULL, bsize, 0,
                                      av1_num_planes(cm) - 1);
        if (mbmi->motion_mode == OBMC_CAUSAL)
          av1_build_obmc_inter_predictors_sb(cm, xd);

        av1_subtract_plane(x, bsize, 0);
        if (x->tx_mode_search_type == TX_MODE_SELECT &&
            !xd->lossless[mbmi->segment_id]) {
          pick_tx_size_type_yrd(cpi, x, &rd_stats_y, bsize, INT64_MAX);
          assert(rd_stats_y.rate != INT_MAX);
        } else {
          super_block_yrd(cpi, x, &rd_stats_y, bsize, INT64_MAX);
          memset(mbmi->inter_tx_size, mbmi->tx_size,
                 sizeof(mbmi->inter_tx_size));
          for (int i = 0; i < xd->n4_h * xd->n4_w; ++i)
            set_blk_skip(x, 0, i, rd_stats_y.skip);
        }
      } else {
        super_block_yrd(cpi, x, &rd_stats_y, bsize, INT64_MAX);
      }

      if (num_planes > 1) {
        super_block_uvrd(cpi, x, &rd_stats_uv, bsize, INT64_MAX);
      } else {
        av1_init_rd_stats(&rd_stats_uv);
      }

      if (is_inter_mode(mbmi->mode) &&
          RDCOST(x->rdmult,
                 x->skip_cost[skip_ctx][0] + rd_stats_y.rate + rd_stats_uv.rate,
                 (rd_stats_y.dist + rd_stats_uv.dist)) >
              RDCOST(x->rdmult, x->skip_cost[skip_ctx][1],
                     (rd_stats_y.sse + rd_stats_uv.sse))) {
        skip_blk = 1;
        rd_stats_y.rate = x->skip_cost[skip_ctx][1];
        rd_stats_uv.rate = 0;
        rd_stats_y.dist = rd_stats_y.sse;
        rd_stats_uv.dist = rd_stats_uv.sse;
      } else {
        skip_blk = 0;
        rd_stats_y.rate += x->skip_cost[skip_ctx][0];
      }
      int this_rate = rd_stats.rate + rd_stats_y.rate + rd_stats_uv.rate -
                      winner_rate_y - winner_rate_uv;
      int64_t this_rd =
          RDCOST(x->rdmult, this_rate, (rd_stats_y.dist + rd_stats_uv.dist));
      if (best_rd > this_rd) {
        *best_mbmode = *mbmi;
        *best_mode_index = winner_mode_index;
        av1_copy_array(ctx->blk_skip, x->blk_skip, ctx->num_4x4_blk);
        av1_copy_array(ctx->tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);
        rd_cost->rate = this_rate;
        rd_cost->dist = rd_stats_y.dist + rd_stats_uv.dist;
        rd_cost->sse = rd_stats_y.sse + rd_stats_uv.sse;
        rd_cost->rdcost = this_rd;
        best_rd = this_rd;
        *best_skip2 = skip_blk;
      }
    }
  }
}

typedef struct {
  // Mask for each reference frame, specifying which prediction modes to NOT try
  // during search.
  uint32_t pred_modes[REF_FRAMES];
  // If ref_combo[i][j + 1] is true, do NOT try prediction using combination of
  // reference frames (i, j).
  // Note: indexing with 'j + 1' is due to the fact that 2nd reference can be -1
  // (NONE_FRAME).
  bool ref_combo[REF_FRAMES][REF_FRAMES + 1];
} mode_skip_mask_t;

// Update 'ref_combo' mask to disable given 'ref' in single and compound modes.
static AOM_INLINE void disable_reference(
    MV_REFERENCE_FRAME ref, bool ref_combo[REF_FRAMES][REF_FRAMES + 1]) {
  for (MV_REFERENCE_FRAME ref2 = NONE_FRAME; ref2 < REF_FRAMES; ++ref2) {
    ref_combo[ref][ref2 + 1] = true;
  }
}

// Update 'ref_combo' mask to disable all inter references except ALTREF.
static AOM_INLINE void disable_inter_references_except_altref(
    bool ref_combo[REF_FRAMES][REF_FRAMES + 1]) {
  disable_reference(LAST_FRAME, ref_combo);
  disable_reference(LAST2_FRAME, ref_combo);
  disable_reference(LAST3_FRAME, ref_combo);
  disable_reference(GOLDEN_FRAME, ref_combo);
  disable_reference(BWDREF_FRAME, ref_combo);
  disable_reference(ALTREF2_FRAME, ref_combo);
}

static const MV_REFERENCE_FRAME reduced_ref_combos[][2] = {
  { LAST_FRAME, NONE_FRAME },     { ALTREF_FRAME, NONE_FRAME },
  { LAST_FRAME, ALTREF_FRAME },   { GOLDEN_FRAME, NONE_FRAME },
  { INTRA_FRAME, NONE_FRAME },    { GOLDEN_FRAME, ALTREF_FRAME },
  { LAST_FRAME, GOLDEN_FRAME },   { LAST_FRAME, INTRA_FRAME },
  { LAST_FRAME, BWDREF_FRAME },   { LAST_FRAME, LAST3_FRAME },
  { GOLDEN_FRAME, BWDREF_FRAME }, { GOLDEN_FRAME, INTRA_FRAME },
  { BWDREF_FRAME, NONE_FRAME },   { BWDREF_FRAME, ALTREF_FRAME },
  { ALTREF_FRAME, INTRA_FRAME },  { BWDREF_FRAME, INTRA_FRAME },
};

static const MV_REFERENCE_FRAME real_time_ref_combos[][2] = {
  { LAST_FRAME, NONE_FRAME },
  { ALTREF_FRAME, NONE_FRAME },
  { GOLDEN_FRAME, NONE_FRAME },
  { INTRA_FRAME, NONE_FRAME }
};

typedef enum { REF_SET_FULL, REF_SET_REDUCED, REF_SET_REALTIME } REF_SET;

static AOM_INLINE void default_skip_mask(mode_skip_mask_t *mask,
                                         REF_SET ref_set) {
  if (ref_set == REF_SET_FULL) {
    // Everything available by default.
    memset(mask, 0, sizeof(*mask));
  } else {
    // All modes available by default.
    memset(mask->pred_modes, 0, sizeof(mask->pred_modes));
    // All references disabled first.
    for (MV_REFERENCE_FRAME ref1 = INTRA_FRAME; ref1 < REF_FRAMES; ++ref1) {
      for (MV_REFERENCE_FRAME ref2 = NONE_FRAME; ref2 < REF_FRAMES; ++ref2) {
        mask->ref_combo[ref1][ref2 + 1] = true;
      }
    }
    const MV_REFERENCE_FRAME(*ref_set_combos)[2];
    int num_ref_combos;

    // Then enable reduced set of references explicitly.
    switch (ref_set) {
      case REF_SET_REDUCED:
        ref_set_combos = reduced_ref_combos;
        num_ref_combos =
            (int)sizeof(reduced_ref_combos) / sizeof(reduced_ref_combos[0]);
        break;
      case REF_SET_REALTIME:
        ref_set_combos = real_time_ref_combos;
        num_ref_combos =
            (int)sizeof(real_time_ref_combos) / sizeof(real_time_ref_combos[0]);
        break;
      default: assert(0); num_ref_combos = 0;
    }

    for (int i = 0; i < num_ref_combos; ++i) {
      const MV_REFERENCE_FRAME *const this_combo = ref_set_combos[i];
      mask->ref_combo[this_combo[0]][this_combo[1] + 1] = false;
    }
  }
}

static AOM_INLINE void init_mode_skip_mask(mode_skip_mask_t *mask,
                                           const AV1_COMP *cpi, MACROBLOCK *x,
                                           BLOCK_SIZE bsize) {
  const AV1_COMMON *const cm = &cpi->common;
  const struct segmentation *const seg = &cm->seg;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  unsigned char segment_id = mbmi->segment_id;
  const SPEED_FEATURES *const sf = &cpi->sf;
  REF_SET ref_set = REF_SET_FULL;

  if (sf->rt_sf.use_real_time_ref_set)
    ref_set = REF_SET_REALTIME;
  else if (cpi->oxcf.enable_reduced_reference_set)
    ref_set = REF_SET_REDUCED;

  default_skip_mask(mask, ref_set);

  int min_pred_mv_sad = INT_MAX;
  MV_REFERENCE_FRAME ref_frame;
  if (ref_set == REF_SET_REALTIME) {
    // For real-time encoding, we only look at a subset of ref frames. So the
    // threshold for pruning should be computed from this subset as well.
    const int num_rt_refs =
        sizeof(real_time_ref_combos) / sizeof(*real_time_ref_combos);
    for (int r_idx = 0; r_idx < num_rt_refs; r_idx++) {
      const MV_REFERENCE_FRAME ref = real_time_ref_combos[r_idx][0];
      if (ref != INTRA_FRAME) {
        min_pred_mv_sad = AOMMIN(min_pred_mv_sad, x->pred_mv_sad[ref]);
      }
    }
  } else {
    for (ref_frame = LAST_FRAME; ref_frame <= ALTREF_FRAME; ++ref_frame)
      min_pred_mv_sad = AOMMIN(min_pred_mv_sad, x->pred_mv_sad[ref_frame]);
  }

  for (ref_frame = LAST_FRAME; ref_frame <= ALTREF_FRAME; ++ref_frame) {
    if (!(cpi->ref_frame_flags & av1_ref_frame_flag_list[ref_frame])) {
      // Skip checking missing reference in both single and compound reference
      // modes.
      disable_reference(ref_frame, mask->ref_combo);
    } else {
      // Skip fixed mv modes for poor references
      if ((x->pred_mv_sad[ref_frame] >> 2) > min_pred_mv_sad) {
        mask->pred_modes[ref_frame] |= INTER_NEAREST_NEAR_ZERO;
      }
    }
    if (segfeature_active(seg, segment_id, SEG_LVL_REF_FRAME) &&
        get_segdata(seg, segment_id, SEG_LVL_REF_FRAME) != (int)ref_frame) {
      // Reference not used for the segment.
      disable_reference(ref_frame, mask->ref_combo);
    }
  }
  // Note: We use the following drop-out only if the SEG_LVL_REF_FRAME feature
  // is disabled for this segment. This is to prevent the possibility that we
  // end up unable to pick any mode.
  if (!segfeature_active(seg, segment_id, SEG_LVL_REF_FRAME)) {
    // Only consider GLOBALMV/ALTREF_FRAME for alt ref frame,
    // unless ARNR filtering is enabled in which case we want
    // an unfiltered alternative. We allow near/nearest as well
    // because they may result in zero-zero MVs but be cheaper.
    if (cpi->rc.is_src_frame_alt_ref && (cpi->oxcf.arnr_max_frames == 0)) {
      disable_inter_references_except_altref(mask->ref_combo);

      mask->pred_modes[ALTREF_FRAME] = ~INTER_NEAREST_NEAR_ZERO;
      const MV_REFERENCE_FRAME tmp_ref_frames[2] = { ALTREF_FRAME, NONE_FRAME };
      int_mv near_mv, nearest_mv, global_mv;
      get_this_mv(&nearest_mv, NEARESTMV, 0, 0, tmp_ref_frames, x->mbmi_ext);
      get_this_mv(&near_mv, NEARMV, 0, 0, tmp_ref_frames, x->mbmi_ext);
      get_this_mv(&global_mv, GLOBALMV, 0, 0, tmp_ref_frames, x->mbmi_ext);

      if (near_mv.as_int != global_mv.as_int)
        mask->pred_modes[ALTREF_FRAME] |= (1 << NEARMV);
      if (nearest_mv.as_int != global_mv.as_int)
        mask->pred_modes[ALTREF_FRAME] |= (1 << NEARESTMV);
    }
  }

  if (cpi->rc.is_src_frame_alt_ref) {
    if (sf->inter_sf.alt_ref_search_fp) {
      assert(cpi->ref_frame_flags & av1_ref_frame_flag_list[ALTREF_FRAME]);
      mask->pred_modes[ALTREF_FRAME] = 0;
      disable_inter_references_except_altref(mask->ref_combo);
      disable_reference(INTRA_FRAME, mask->ref_combo);
    }
  }

  if (sf->inter_sf.alt_ref_search_fp) {
    if (!cm->show_frame && x->best_pred_mv_sad < INT_MAX) {
      int sad_thresh = x->best_pred_mv_sad + (x->best_pred_mv_sad >> 3);
      // Conservatively skip the modes w.r.t. BWDREF, ALTREF2 and ALTREF, if
      // those are past frames
      for (ref_frame = BWDREF_FRAME; ref_frame <= ALTREF_FRAME; ref_frame++) {
        if (cpi->ref_relative_dist[ref_frame - LAST_FRAME] < 0)
          if (x->pred_mv_sad[ref_frame] > sad_thresh)
            mask->pred_modes[ref_frame] |= INTER_ALL;
      }
    }
  }

  if (sf->inter_sf.adaptive_mode_search) {
    if (cm->show_frame && !cpi->rc.is_src_frame_alt_ref &&
        cpi->rc.frames_since_golden >= 3)
      if ((x->pred_mv_sad[GOLDEN_FRAME] >> 1) > x->pred_mv_sad[LAST_FRAME])
        mask->pred_modes[GOLDEN_FRAME] |= INTER_ALL;
  }

  if (bsize > sf->part_sf.max_intra_bsize) {
    disable_reference(INTRA_FRAME, mask->ref_combo);
  }

  mask->pred_modes[INTRA_FRAME] |=
      ~(sf->intra_sf.intra_y_mode_mask[max_txsize_lookup[bsize]]);
}

// Please add/modify parameter setting in this function, making it consistent
// and easy to read and maintain.
static AOM_INLINE void set_params_rd_pick_inter_mode(
    const AV1_COMP *cpi, MACROBLOCK *x, HandleInterModeArgs *args,
    BLOCK_SIZE bsize, mode_skip_mask_t *mode_skip_mask, int skip_ref_frame_mask,
    unsigned int ref_costs_single[REF_FRAMES],
    unsigned int ref_costs_comp[REF_FRAMES][REF_FRAMES],
    struct buf_2d yv12_mb[REF_FRAMES][MAX_MB_PLANE]) {
  const AV1_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  MB_MODE_INFO_EXT *const mbmi_ext = x->mbmi_ext;
  unsigned char segment_id = mbmi->segment_id;

  if (is_cur_buf_hbd(xd)) {
    int len = sizeof(uint16_t);
    args->above_pred_buf[0] = CONVERT_TO_BYTEPTR(x->above_pred_buf);
    args->above_pred_buf[1] =
        CONVERT_TO_BYTEPTR(x->above_pred_buf + (MAX_SB_SQUARE >> 1) * len);
    args->above_pred_buf[2] =
        CONVERT_TO_BYTEPTR(x->above_pred_buf + MAX_SB_SQUARE * len);
    args->left_pred_buf[0] = CONVERT_TO_BYTEPTR(x->left_pred_buf);
    args->left_pred_buf[1] =
        CONVERT_TO_BYTEPTR(x->left_pred_buf + (MAX_SB_SQUARE >> 1) * len);
    args->left_pred_buf[2] =
        CONVERT_TO_BYTEPTR(x->left_pred_buf + MAX_SB_SQUARE * len);
  } else {
    args->above_pred_buf[0] = x->above_pred_buf;
    args->above_pred_buf[1] = x->above_pred_buf + (MAX_SB_SQUARE >> 1);
    args->above_pred_buf[2] = x->above_pred_buf + MAX_SB_SQUARE;
    args->left_pred_buf[0] = x->left_pred_buf;
    args->left_pred_buf[1] = x->left_pred_buf + (MAX_SB_SQUARE >> 1);
    args->left_pred_buf[2] = x->left_pred_buf + MAX_SB_SQUARE;
  }

  av1_collect_neighbors_ref_counts(xd);

  estimate_ref_frame_costs(cm, xd, x, segment_id, ref_costs_single,
                           ref_costs_comp);

  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  MV_REFERENCE_FRAME ref_frame;
  x->best_pred_mv_sad = INT_MAX;
  for (ref_frame = LAST_FRAME; ref_frame <= ALTREF_FRAME; ++ref_frame) {
    x->pred_mv_sad[ref_frame] = INT_MAX;
    x->mbmi_ext->mode_context[ref_frame] = 0;
    mbmi_ext->ref_mv_count[ref_frame] = UINT8_MAX;
    if (cpi->ref_frame_flags & av1_ref_frame_flag_list[ref_frame]) {
      if (mbmi->partition != PARTITION_NONE &&
          mbmi->partition != PARTITION_SPLIT) {
        if (skip_ref_frame_mask & (1 << ref_frame)) {
          int skip = 1;
          for (int r = ALTREF_FRAME + 1; r < MODE_CTX_REF_FRAMES; ++r) {
            if (!(skip_ref_frame_mask & (1 << r))) {
              const MV_REFERENCE_FRAME *rf = ref_frame_map[r - REF_FRAMES];
              if (rf[0] == ref_frame || rf[1] == ref_frame) {
                skip = 0;
                break;
              }
            }
          }
          if (skip) continue;
        }
      }
      assert(get_ref_frame_yv12_buf(cm, ref_frame) != NULL);
      setup_buffer_ref_mvs_inter(cpi, x, ref_frame, bsize, yv12_mb);
    }
    // Store the best pred_mv_sad across all past frames
    if (cpi->sf.inter_sf.alt_ref_search_fp &&
        cpi->ref_relative_dist[ref_frame - LAST_FRAME] < 0)
      x->best_pred_mv_sad =
          AOMMIN(x->best_pred_mv_sad, x->pred_mv_sad[ref_frame]);
  }
  // ref_frame = ALTREF_FRAME
  if (!cpi->sf.rt_sf.use_real_time_ref_set) {
    // No second reference on RT ref set, so no need to initialize
    for (; ref_frame < MODE_CTX_REF_FRAMES; ++ref_frame) {
      x->mbmi_ext->mode_context[ref_frame] = 0;
      mbmi_ext->ref_mv_count[ref_frame] = UINT8_MAX;
      const MV_REFERENCE_FRAME *rf = ref_frame_map[ref_frame - REF_FRAMES];
      if (!((cpi->ref_frame_flags & av1_ref_frame_flag_list[rf[0]]) &&
            (cpi->ref_frame_flags & av1_ref_frame_flag_list[rf[1]]))) {
        continue;
      }

      if (mbmi->partition != PARTITION_NONE &&
          mbmi->partition != PARTITION_SPLIT) {
        if (skip_ref_frame_mask & (1 << ref_frame)) {
          continue;
        }
      }
      av1_find_mv_refs(cm, xd, mbmi, ref_frame, mbmi_ext->ref_mv_count,
                       xd->ref_mv_stack, xd->weight, NULL, mbmi_ext->global_mvs,
                       mbmi_ext->mode_context);
      // TODO(Ravi): Populate mbmi_ext->ref_mv_stack[ref_frame][4] and
      // mbmi_ext->weight[ref_frame][4] inside av1_find_mv_refs.
      av1_copy_usable_ref_mv_stack_and_weight(xd, mbmi_ext, ref_frame);
    }
  }

  av1_count_overlappable_neighbors(cm, xd);
  const FRAME_UPDATE_TYPE update_type = get_frame_update_type(&cpi->gf_group);
  const int prune_obmc = cpi->obmc_probs[update_type][bsize] <
                         cpi->sf.inter_sf.prune_obmc_prob_thresh;
  if (cpi->oxcf.enable_obmc && !cpi->sf.inter_sf.disable_obmc && !prune_obmc) {
    if (check_num_overlappable_neighbors(mbmi) &&
        is_motion_variation_allowed_bsize(bsize)) {
      int dst_width1[MAX_MB_PLANE] = { MAX_SB_SIZE, MAX_SB_SIZE, MAX_SB_SIZE };
      int dst_width2[MAX_MB_PLANE] = { MAX_SB_SIZE >> 1, MAX_SB_SIZE >> 1,
                                       MAX_SB_SIZE >> 1 };
      int dst_height1[MAX_MB_PLANE] = { MAX_SB_SIZE >> 1, MAX_SB_SIZE >> 1,
                                        MAX_SB_SIZE >> 1 };
      int dst_height2[MAX_MB_PLANE] = { MAX_SB_SIZE, MAX_SB_SIZE, MAX_SB_SIZE };
      av1_build_prediction_by_above_preds(cm, xd, args->above_pred_buf,
                                          dst_width1, dst_height1,
                                          args->above_pred_stride);
      av1_build_prediction_by_left_preds(cm, xd, args->left_pred_buf,
                                         dst_width2, dst_height2,
                                         args->left_pred_stride);
      const int num_planes = av1_num_planes(cm);
      av1_setup_dst_planes(xd->plane, bsize, &cm->cur_frame->buf, mi_row,
                           mi_col, 0, num_planes);
      calc_target_weighted_pred(
          cm, x, xd, args->above_pred_buf[0], args->above_pred_stride[0],
          args->left_pred_buf[0], args->left_pred_stride[0]);
    }
  }

  init_mode_skip_mask(mode_skip_mask, cpi, x, bsize);

  // Set params for mode evaluation
  set_mode_eval_params(cpi, x, MODE_EVAL);

  x->comp_rd_stats_idx = 0;
}

static AOM_INLINE void search_palette_mode(
    const AV1_COMP *cpi, MACROBLOCK *x, RD_STATS *rd_cost,
    PICK_MODE_CONTEXT *ctx, BLOCK_SIZE bsize, MB_MODE_INFO *const mbmi,
    PALETTE_MODE_INFO *const pmi, unsigned int *ref_costs_single,
    InterModeSearchState *search_state) {
  const AV1_COMMON *const cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  MACROBLOCKD *const xd = &x->e_mbd;
  int rate2 = 0;
  int64_t distortion2 = 0, best_rd_palette = search_state->best_rd, this_rd,
          best_model_rd_palette = INT64_MAX;
  int skippable = 0;
  TX_SIZE uv_tx = TX_4X4;
  uint8_t *const best_palette_color_map =
      x->palette_buffer->best_palette_color_map;
  uint8_t *const color_map = xd->plane[0].color_index_map;
  MB_MODE_INFO best_mbmi_palette = *mbmi;
  uint8_t best_blk_skip[MAX_MIB_SIZE * MAX_MIB_SIZE];
  uint8_t best_tx_type_map[MAX_MIB_SIZE * MAX_MIB_SIZE];
  const int *const intra_mode_cost = x->mbmode_cost[size_group_lookup[bsize]];
  const int rows = block_size_high[bsize];
  const int cols = block_size_wide[bsize];

  mbmi->mode = DC_PRED;
  mbmi->uv_mode = UV_DC_PRED;
  mbmi->ref_frame[0] = INTRA_FRAME;
  mbmi->ref_frame[1] = NONE_FRAME;
  RD_STATS rd_stats_y;
  av1_invalid_rd_stats(&rd_stats_y);
  rd_pick_palette_intra_sby(
      cpi, x, bsize, intra_mode_cost[DC_PRED], &best_mbmi_palette,
      best_palette_color_map, &best_rd_palette, &best_model_rd_palette,
      &rd_stats_y.rate, NULL, &rd_stats_y.dist, &rd_stats_y.skip, NULL, ctx,
      best_blk_skip, best_tx_type_map);
  if (rd_stats_y.rate == INT_MAX || pmi->palette_size[0] == 0) return;

  memcpy(x->blk_skip, best_blk_skip,
         sizeof(best_blk_skip[0]) * bsize_to_num_blk(bsize));
  av1_copy_array(xd->tx_type_map, best_tx_type_map, ctx->num_4x4_blk);
  memcpy(color_map, best_palette_color_map,
         rows * cols * sizeof(best_palette_color_map[0]));

  skippable = rd_stats_y.skip;
  distortion2 = rd_stats_y.dist;
  rate2 = rd_stats_y.rate + ref_costs_single[INTRA_FRAME];
  if (num_planes > 1) {
    uv_tx = av1_get_tx_size(AOM_PLANE_U, xd);
    if (search_state->rate_uv_intra == INT_MAX) {
      choose_intra_uv_mode(cpi, x, bsize, uv_tx, &search_state->rate_uv_intra,
                           &search_state->rate_uv_tokenonly,
                           &search_state->dist_uvs, &search_state->skip_uvs,
                           &search_state->mode_uv);
      search_state->pmi_uv = *pmi;
      search_state->uv_angle_delta = mbmi->angle_delta[PLANE_TYPE_UV];
    }
    mbmi->uv_mode = search_state->mode_uv;
    pmi->palette_size[1] = search_state->pmi_uv.palette_size[1];
    if (pmi->palette_size[1] > 0) {
      memcpy(pmi->palette_colors + PALETTE_MAX_SIZE,
             search_state->pmi_uv.palette_colors + PALETTE_MAX_SIZE,
             2 * PALETTE_MAX_SIZE * sizeof(pmi->palette_colors[0]));
    }
    mbmi->angle_delta[PLANE_TYPE_UV] = search_state->uv_angle_delta;
    skippable = skippable && search_state->skip_uvs;
    distortion2 += search_state->dist_uvs;
    rate2 += search_state->rate_uv_intra;
  }

  if (skippable) {
    rate2 -= rd_stats_y.rate;
    if (num_planes > 1) rate2 -= search_state->rate_uv_tokenonly;
    rate2 += x->skip_cost[av1_get_skip_context(xd)][1];
  } else {
    rate2 += x->skip_cost[av1_get_skip_context(xd)][0];
  }
  this_rd = RDCOST(x->rdmult, rate2, distortion2);
  if (this_rd < search_state->best_rd) {
    search_state->best_mode_index = THR_DC;
    mbmi->mv[0].as_int = 0;
    rd_cost->rate = rate2;
    rd_cost->dist = distortion2;
    rd_cost->rdcost = this_rd;
    search_state->best_rd = this_rd;
    search_state->best_mbmode = *mbmi;
    search_state->best_skip2 = 0;
    search_state->best_mode_skippable = skippable;
    memcpy(ctx->blk_skip, x->blk_skip,
           sizeof(x->blk_skip[0]) * ctx->num_4x4_blk);
    av1_copy_array(ctx->tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);
  }
}

static AOM_INLINE void init_inter_mode_search_state(
    InterModeSearchState *search_state, const AV1_COMP *cpi,
    const MACROBLOCK *x, BLOCK_SIZE bsize, int64_t best_rd_so_far) {
  search_state->best_rd = best_rd_so_far;

  av1_zero(search_state->best_mbmode);

  search_state->best_rate_y = INT_MAX;

  search_state->best_rate_uv = INT_MAX;

  search_state->best_mode_skippable = 0;

  search_state->best_skip2 = 0;

  search_state->best_mode_index = THR_INVALID;

  const MACROBLOCKD *const xd = &x->e_mbd;
  const MB_MODE_INFO *const mbmi = xd->mi[0];
  const unsigned char segment_id = mbmi->segment_id;

  search_state->skip_intra_modes = 0;

  search_state->num_available_refs = 0;
  memset(search_state->dist_refs, -1, sizeof(search_state->dist_refs));
  memset(search_state->dist_order_refs, -1,
         sizeof(search_state->dist_order_refs));

  for (int i = 0; i <= LAST_NEW_MV_INDEX; ++i)
    search_state->mode_threshold[i] = 0;
  const int *const rd_threshes = cpi->rd.threshes[segment_id][bsize];
  for (int i = LAST_NEW_MV_INDEX + 1; i < MAX_MODES; ++i)
    search_state->mode_threshold[i] =
        ((int64_t)rd_threshes[i] * x->thresh_freq_fact[bsize][i]) >> 5;

  search_state->best_intra_mode = DC_PRED;
  search_state->best_intra_rd = INT64_MAX;

  search_state->angle_stats_ready = 0;
  av1_zero(search_state->directional_mode_skip_mask);

  search_state->best_pred_sse = UINT_MAX;
  search_state->rate_uv_intra = INT_MAX;

  av1_zero(search_state->pmi_uv);

  for (int i = 0; i < REFERENCE_MODES; ++i)
    search_state->best_pred_rd[i] = INT64_MAX;

  av1_zero(search_state->single_newmv);
  av1_zero(search_state->single_newmv_rate);
  av1_zero(search_state->single_newmv_valid);
  for (int i = 0; i < MB_MODE_COUNT; ++i) {
    for (int j = 0; j < MAX_REF_MV_SEARCH; ++j) {
      for (int ref_frame = 0; ref_frame < REF_FRAMES; ++ref_frame) {
        search_state->modelled_rd[i][j][ref_frame] = INT64_MAX;
        search_state->simple_rd[i][j][ref_frame] = INT64_MAX;
      }
    }
  }

  for (int dir = 0; dir < 2; ++dir) {
    for (int mode = 0; mode < SINGLE_INTER_MODE_NUM; ++mode) {
      for (int ref_frame = 0; ref_frame < FWD_REFS; ++ref_frame) {
        SingleInterModeState *state;

        state = &search_state->single_state[dir][mode][ref_frame];
        state->ref_frame = NONE_FRAME;
        state->rd = INT64_MAX;

        state = &search_state->single_state_modelled[dir][mode][ref_frame];
        state->ref_frame = NONE_FRAME;
        state->rd = INT64_MAX;
      }
    }
  }
  for (int dir = 0; dir < 2; ++dir) {
    for (int mode = 0; mode < SINGLE_INTER_MODE_NUM; ++mode) {
      for (int ref_frame = 0; ref_frame < FWD_REFS; ++ref_frame) {
        search_state->single_rd_order[dir][mode][ref_frame] = NONE_FRAME;
      }
    }
  }
  av1_zero(search_state->single_state_cnt);
  av1_zero(search_state->single_state_modelled_cnt);
}

static bool mask_says_skip(const mode_skip_mask_t *mode_skip_mask,
                           const MV_REFERENCE_FRAME *ref_frame,
                           const PREDICTION_MODE this_mode) {
  if (mode_skip_mask->pred_modes[ref_frame[0]] & (1 << this_mode)) {
    return true;
  }

  return mode_skip_mask->ref_combo[ref_frame[0]][ref_frame[1] + 1];
}

static int inter_mode_compatible_skip(const AV1_COMP *cpi, const MACROBLOCK *x,
                                      BLOCK_SIZE bsize,
                                      PREDICTION_MODE curr_mode,
                                      const MV_REFERENCE_FRAME *ref_frames) {
  const int comp_pred = ref_frames[1] > INTRA_FRAME;
  if (comp_pred) {
    if (!is_comp_ref_allowed(bsize)) return 1;
    if (!(cpi->ref_frame_flags & av1_ref_frame_flag_list[ref_frames[1]])) {
      return 1;
    }

    const AV1_COMMON *const cm = &cpi->common;
    if (frame_is_intra_only(cm)) return 1;

    const CurrentFrame *const current_frame = &cm->current_frame;
    if (current_frame->reference_mode == SINGLE_REFERENCE) return 1;

    const struct segmentation *const seg = &cm->seg;
    const unsigned char segment_id = x->e_mbd.mi[0]->segment_id;
    // Do not allow compound prediction if the segment level reference frame
    // feature is in use as in this case there can only be one reference.
    if (segfeature_active(seg, segment_id, SEG_LVL_REF_FRAME)) return 1;
  }

  if (ref_frames[0] > INTRA_FRAME && ref_frames[1] == INTRA_FRAME) {
    // Mode must be compatible
    if (!is_interintra_allowed_bsize(bsize)) return 1;
    if (!is_interintra_allowed_mode(curr_mode)) return 1;
  }

  return 0;
}

static int fetch_picked_ref_frames_mask(const MACROBLOCK *const x,
                                        BLOCK_SIZE bsize, int mib_size) {
  const int sb_size_mask = mib_size - 1;
  const MACROBLOCKD *const xd = &x->e_mbd;
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  const int mi_row_in_sb = mi_row & sb_size_mask;
  const int mi_col_in_sb = mi_col & sb_size_mask;
  const int mi_w = mi_size_wide[bsize];
  const int mi_h = mi_size_high[bsize];
  int picked_ref_frames_mask = 0;
  for (int i = mi_row_in_sb; i < mi_row_in_sb + mi_h; ++i) {
    for (int j = mi_col_in_sb; j < mi_col_in_sb + mi_w; ++j) {
      picked_ref_frames_mask |= x->picked_ref_frames_mask[i * 32 + j];
    }
  }
  return picked_ref_frames_mask;
}

// Case 1: return 0, means don't skip this mode
// Case 2: return 1, means skip this mode completely
// Case 3: return 2, means skip compound only, but still try single motion modes
static int inter_mode_search_order_independent_skip(
    const AV1_COMP *cpi, const MACROBLOCK *x, mode_skip_mask_t *mode_skip_mask,
    InterModeSearchState *search_state, int skip_ref_frame_mask,
    PREDICTION_MODE mode, const MV_REFERENCE_FRAME *ref_frame) {
  if (mask_says_skip(mode_skip_mask, ref_frame, mode)) {
    return 1;
  }

  // This is only used in motion vector unit test.
  if (cpi->oxcf.motion_vector_unit_test && ref_frame[0] == INTRA_FRAME)
    return 1;

  const AV1_COMMON *const cm = &cpi->common;
  if (skip_repeated_mv(cm, x, mode, ref_frame, search_state)) {
    return 1;
  }

  const int comp_pred = ref_frame[1] > INTRA_FRAME;
  if ((!cpi->oxcf.enable_onesided_comp ||
       cpi->sf.inter_sf.disable_onesided_comp) &&
      comp_pred && cpi->all_one_sided_refs) {
    return 1;
  }

  const MB_MODE_INFO *const mbmi = x->e_mbd.mi[0];
  // If no valid mode has been found so far in PARTITION_NONE when finding a
  // valid partition is required, do not skip mode.
  if (search_state->best_rd == INT64_MAX && mbmi->partition == PARTITION_NONE &&
      x->must_find_valid_partition)
    return 0;

  int skip_motion_mode = 0;
  if (mbmi->partition != PARTITION_NONE && mbmi->partition != PARTITION_SPLIT) {
    const int ref_type = av1_ref_frame_type(ref_frame);
    int skip_ref = skip_ref_frame_mask & (1 << ref_type);
    if (ref_type <= ALTREF_FRAME && skip_ref) {
      // Since the compound ref modes depends on the motion estimation result of
      // two single ref modes( best mv of single ref modes as the start point )
      // If current single ref mode is marked skip, we need to check if it will
      // be used in compound ref modes.
      for (int r = ALTREF_FRAME + 1; r < MODE_CTX_REF_FRAMES; ++r) {
        if (skip_ref_frame_mask & (1 << r)) continue;
        const MV_REFERENCE_FRAME *rf = ref_frame_map[r - REF_FRAMES];
        if (rf[0] == ref_type || rf[1] == ref_type) {
          // Found a not skipped compound ref mode which contains current
          // single ref. So this single ref can't be skipped completly
          // Just skip it's motion mode search, still try it's simple
          // transition mode.
          skip_motion_mode = 1;
          skip_ref = 0;
          break;
        }
      }
    }
    if (skip_ref) return 1;
  }

  const SPEED_FEATURES *const sf = &cpi->sf;
  if (ref_frame[0] == INTRA_FRAME) {
    if (mode != DC_PRED) {
      // Disable intra modes other than DC_PRED for blocks with low variance
      // Threshold for intra skipping based on source variance
      // TODO(debargha): Specialize the threshold for super block sizes
      const unsigned int skip_intra_var_thresh = 64;
      if ((sf->rt_sf.mode_search_skip_flags & FLAG_SKIP_INTRA_LOWVAR) &&
          x->source_variance < skip_intra_var_thresh)
        return 1;
    }
  }

  if (prune_ref_by_selective_ref_frame(cpi, ref_frame,
                                       cm->cur_frame->ref_display_order_hint,
                                       cm->current_frame.display_order_hint))
    return 1;

  if (skip_motion_mode) return 2;

  return 0;
}

static INLINE void init_mbmi(MB_MODE_INFO *mbmi, PREDICTION_MODE curr_mode,
                             const MV_REFERENCE_FRAME *ref_frames,
                             const AV1_COMMON *cm) {
  PALETTE_MODE_INFO *const pmi = &mbmi->palette_mode_info;
  mbmi->ref_mv_idx = 0;
  mbmi->mode = curr_mode;
  mbmi->uv_mode = UV_DC_PRED;
  mbmi->ref_frame[0] = ref_frames[0];
  mbmi->ref_frame[1] = ref_frames[1];
  pmi->palette_size[0] = 0;
  pmi->palette_size[1] = 0;
  mbmi->filter_intra_mode_info.use_filter_intra = 0;
  mbmi->mv[0].as_int = mbmi->mv[1].as_int = 0;
  mbmi->motion_mode = SIMPLE_TRANSLATION;
  mbmi->interintra_mode = (INTERINTRA_MODE)(II_DC_PRED - 1);
  set_default_interp_filters(mbmi, cm->interp_filter);
}

static int64_t handle_intra_mode(InterModeSearchState *search_state,
                                 const AV1_COMP *cpi, MACROBLOCK *x,
                                 BLOCK_SIZE bsize, int ref_frame_cost,
                                 const PICK_MODE_CONTEXT *ctx, int disable_skip,
                                 RD_STATS *rd_stats, RD_STATS *rd_stats_y,
                                 RD_STATS *rd_stats_uv) {
  const AV1_COMMON *cm = &cpi->common;
  const SPEED_FEATURES *const sf = &cpi->sf;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  assert(mbmi->ref_frame[0] == INTRA_FRAME);
  const PREDICTION_MODE mode = mbmi->mode;
  const int mode_cost =
      x->mbmode_cost[size_group_lookup[bsize]][mode] + ref_frame_cost;
  const int intra_cost_penalty = av1_get_intra_cost_penalty(
      cm->base_qindex, cm->y_dc_delta_q, cm->seq_params.bit_depth);
  const int skip_ctx = av1_get_skip_context(xd);

  int known_rate = mode_cost;
  known_rate += ref_frame_cost;
  if (mode != DC_PRED && mode != PAETH_PRED) known_rate += intra_cost_penalty;
  known_rate += AOMMIN(x->skip_cost[skip_ctx][0], x->skip_cost[skip_ctx][1]);
  const int64_t known_rd = RDCOST(x->rdmult, known_rate, 0);
  if (known_rd > search_state->best_rd) {
    search_state->skip_intra_modes = 1;
    return INT64_MAX;
  }

  const int is_directional_mode = av1_is_directional_mode(mode);
  if (is_directional_mode && av1_use_angle_delta(bsize) &&
      cpi->oxcf.enable_angle_delta) {
    if (sf->intra_sf.intra_pruning_with_hog &&
        !search_state->angle_stats_ready) {
      prune_intra_mode_with_hog(x, bsize,
                                cpi->sf.intra_sf.intra_pruning_with_hog_thresh,
                                search_state->directional_mode_skip_mask);
      search_state->angle_stats_ready = 1;
    }
    if (search_state->directional_mode_skip_mask[mode]) return INT64_MAX;
    av1_init_rd_stats(rd_stats_y);
    rd_stats_y->rate = INT_MAX;
    int64_t model_rd = INT64_MAX;
    int rate_dummy;
    rd_pick_intra_angle_sby(cpi, x, &rate_dummy, rd_stats_y, bsize, mode_cost,
                            search_state->best_rd, &model_rd, 0);

  } else {
    av1_init_rd_stats(rd_stats_y);
    mbmi->angle_delta[PLANE_TYPE_Y] = 0;
    super_block_yrd(cpi, x, rd_stats_y, bsize, search_state->best_rd);
  }

  // Pick filter intra modes.
  if (mode == DC_PRED && av1_filter_intra_allowed_bsize(cm, bsize)) {
    int try_filter_intra = 0;
    int64_t best_rd_so_far = INT64_MAX;
    if (rd_stats_y->rate != INT_MAX) {
      const int tmp_rate =
          rd_stats_y->rate + x->filter_intra_cost[bsize][0] + mode_cost;
      best_rd_so_far = RDCOST(x->rdmult, tmp_rate, rd_stats_y->dist);
      try_filter_intra = (best_rd_so_far / 2) <= search_state->best_rd;
    } else {
      try_filter_intra = !search_state->best_mbmode.skip;
    }

    if (try_filter_intra) {
      RD_STATS rd_stats_y_fi;
      int filter_intra_selected_flag = 0;
      TX_SIZE best_tx_size = mbmi->tx_size;
      FILTER_INTRA_MODE best_fi_mode = FILTER_DC_PRED;
      uint8_t best_blk_skip[MAX_MIB_SIZE * MAX_MIB_SIZE];
      memcpy(best_blk_skip, x->blk_skip,
             sizeof(best_blk_skip[0]) * ctx->num_4x4_blk);
      uint8_t best_tx_type_map[MAX_MIB_SIZE * MAX_MIB_SIZE];
      av1_copy_array(best_tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);
      mbmi->filter_intra_mode_info.use_filter_intra = 1;
      for (FILTER_INTRA_MODE fi_mode = FILTER_DC_PRED;
           fi_mode < FILTER_INTRA_MODES; ++fi_mode) {
        mbmi->filter_intra_mode_info.filter_intra_mode = fi_mode;
        super_block_yrd(cpi, x, &rd_stats_y_fi, bsize, search_state->best_rd);
        if (rd_stats_y_fi.rate == INT_MAX) continue;
        const int this_rate_tmp =
            rd_stats_y_fi.rate +
            intra_mode_info_cost_y(cpi, x, mbmi, bsize, mode_cost);
        const int64_t this_rd_tmp =
            RDCOST(x->rdmult, this_rate_tmp, rd_stats_y_fi.dist);

        if (this_rd_tmp != INT64_MAX &&
            this_rd_tmp / 2 > search_state->best_rd) {
          break;
        }
        if (this_rd_tmp < best_rd_so_far) {
          best_tx_size = mbmi->tx_size;
          av1_copy_array(best_tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);
          memcpy(best_blk_skip, x->blk_skip,
                 sizeof(best_blk_skip[0]) * ctx->num_4x4_blk);
          best_fi_mode = fi_mode;
          *rd_stats_y = rd_stats_y_fi;
          filter_intra_selected_flag = 1;
          best_rd_so_far = this_rd_tmp;
        }
      }

      mbmi->tx_size = best_tx_size;
      av1_copy_array(xd->tx_type_map, best_tx_type_map, ctx->num_4x4_blk);
      memcpy(x->blk_skip, best_blk_skip,
             sizeof(x->blk_skip[0]) * ctx->num_4x4_blk);

      if (filter_intra_selected_flag) {
        mbmi->filter_intra_mode_info.use_filter_intra = 1;
        mbmi->filter_intra_mode_info.filter_intra_mode = best_fi_mode;
      } else {
        mbmi->filter_intra_mode_info.use_filter_intra = 0;
      }
    }
  }

  if (rd_stats_y->rate == INT_MAX) return INT64_MAX;

  const int mode_cost_y =
      intra_mode_info_cost_y(cpi, x, mbmi, bsize, mode_cost);
  av1_init_rd_stats(rd_stats);
  av1_init_rd_stats(rd_stats_uv);
  const int num_planes = av1_num_planes(cm);
  if (num_planes > 1) {
    PALETTE_MODE_INFO *const pmi = &mbmi->palette_mode_info;
    const int try_palette =
        cpi->oxcf.enable_palette &&
        av1_allow_palette(cm->allow_screen_content_tools, mbmi->sb_type);
    const TX_SIZE uv_tx = av1_get_tx_size(AOM_PLANE_U, xd);
    if (search_state->rate_uv_intra == INT_MAX) {
      const int rate_y =
          rd_stats_y->skip ? x->skip_cost[skip_ctx][1] : rd_stats_y->rate;
      const int64_t rdy =
          RDCOST(x->rdmult, rate_y + mode_cost_y, rd_stats_y->dist);
      if (search_state->best_rd < (INT64_MAX / 2) &&
          rdy > (search_state->best_rd + (search_state->best_rd >> 2))) {
        search_state->skip_intra_modes = 1;
        return INT64_MAX;
      }
      choose_intra_uv_mode(cpi, x, bsize, uv_tx, &search_state->rate_uv_intra,
                           &search_state->rate_uv_tokenonly,
                           &search_state->dist_uvs, &search_state->skip_uvs,
                           &search_state->mode_uv);
      if (try_palette) search_state->pmi_uv = *pmi;
      search_state->uv_angle_delta = mbmi->angle_delta[PLANE_TYPE_UV];

      const int uv_rate = search_state->rate_uv_tokenonly;
      const int64_t uv_dist = search_state->dist_uvs;
      const int64_t uv_rd = RDCOST(x->rdmult, uv_rate, uv_dist);
      if (uv_rd > search_state->best_rd) {
        search_state->skip_intra_modes = 1;
        return INT64_MAX;
      }
    }

    rd_stats_uv->rate = search_state->rate_uv_tokenonly;
    rd_stats_uv->dist = search_state->dist_uvs;
    rd_stats_uv->skip = search_state->skip_uvs;
    rd_stats->skip = rd_stats_y->skip && rd_stats_uv->skip;
    mbmi->uv_mode = search_state->mode_uv;
    if (try_palette) {
      pmi->palette_size[1] = search_state->pmi_uv.palette_size[1];
      memcpy(pmi->palette_colors + PALETTE_MAX_SIZE,
             search_state->pmi_uv.palette_colors + PALETTE_MAX_SIZE,
             2 * PALETTE_MAX_SIZE * sizeof(pmi->palette_colors[0]));
    }
    mbmi->angle_delta[PLANE_TYPE_UV] = search_state->uv_angle_delta;
  }

  rd_stats->rate = rd_stats_y->rate + mode_cost_y;
  if (!xd->lossless[mbmi->segment_id] && block_signals_txsize(bsize)) {
    // super_block_yrd above includes the cost of the tx_size in the
    // tokenonly rate, but for intra blocks, tx_size is always coded
    // (prediction granularity), so we account for it in the full rate,
    // not the tokenonly rate.
    rd_stats_y->rate -= tx_size_cost(x, bsize, mbmi->tx_size);
  }
  if (num_planes > 1 && !x->skip_chroma_rd) {
    const int uv_mode_cost =
        x->intra_uv_mode_cost[is_cfl_allowed(xd)][mode][mbmi->uv_mode];
    rd_stats->rate +=
        rd_stats_uv->rate +
        intra_mode_info_cost_uv(cpi, x, mbmi, bsize, uv_mode_cost);
  }
  if (mode != DC_PRED && mode != PAETH_PRED) {
    rd_stats->rate += intra_cost_penalty;
  }

  // Intra block is always coded as non-skip
  rd_stats->skip = 0;
  rd_stats->dist = rd_stats_y->dist + rd_stats_uv->dist;
  // Add in the cost of the no skip flag.
  rd_stats->rate += x->skip_cost[skip_ctx][0];
  // Calculate the final RD estimate for this mode.
  const int64_t this_rd = RDCOST(x->rdmult, rd_stats->rate, rd_stats->dist);
  // Keep record of best intra rd
  if (this_rd < search_state->best_intra_rd) {
    search_state->best_intra_rd = this_rd;
    search_state->best_intra_mode = mode;
  }

  if (sf->intra_sf.skip_intra_in_interframe) {
    if (search_state->best_rd < (INT64_MAX / 2) &&
        this_rd > (search_state->best_rd + (search_state->best_rd >> 1)))
      search_state->skip_intra_modes = 1;
  }

  if (!disable_skip) {
    for (int i = 0; i < REFERENCE_MODES; ++i) {
      search_state->best_pred_rd[i] =
          AOMMIN(search_state->best_pred_rd[i], this_rd);
    }
  }
  return this_rd;
}

static AOM_INLINE void collect_single_states(MACROBLOCK *x,
                                             InterModeSearchState *search_state,
                                             const MB_MODE_INFO *const mbmi) {
  int i, j;
  const MV_REFERENCE_FRAME ref_frame = mbmi->ref_frame[0];
  const PREDICTION_MODE this_mode = mbmi->mode;
  const int dir = ref_frame <= GOLDEN_FRAME ? 0 : 1;
  const int mode_offset = INTER_OFFSET(this_mode);
  const int ref_set = get_drl_refmv_count(x, mbmi->ref_frame, this_mode);

  // Simple rd
  int64_t simple_rd = search_state->simple_rd[this_mode][0][ref_frame];
  for (int ref_mv_idx = 1; ref_mv_idx < ref_set; ++ref_mv_idx) {
    const int64_t rd =
        search_state->simple_rd[this_mode][ref_mv_idx][ref_frame];
    if (rd < simple_rd) simple_rd = rd;
  }

  // Insertion sort of single_state
  const SingleInterModeState this_state_s = { simple_rd, ref_frame, 1 };
  SingleInterModeState *state_s = search_state->single_state[dir][mode_offset];
  i = search_state->single_state_cnt[dir][mode_offset];
  for (j = i; j > 0 && state_s[j - 1].rd > this_state_s.rd; --j)
    state_s[j] = state_s[j - 1];
  state_s[j] = this_state_s;
  search_state->single_state_cnt[dir][mode_offset]++;

  // Modelled rd
  int64_t modelled_rd = search_state->modelled_rd[this_mode][0][ref_frame];
  for (int ref_mv_idx = 1; ref_mv_idx < ref_set; ++ref_mv_idx) {
    const int64_t rd =
        search_state->modelled_rd[this_mode][ref_mv_idx][ref_frame];
    if (rd < modelled_rd) modelled_rd = rd;
  }

  // Insertion sort of single_state_modelled
  const SingleInterModeState this_state_m = { modelled_rd, ref_frame, 1 };
  SingleInterModeState *state_m =
      search_state->single_state_modelled[dir][mode_offset];
  i = search_state->single_state_modelled_cnt[dir][mode_offset];
  for (j = i; j > 0 && state_m[j - 1].rd > this_state_m.rd; --j)
    state_m[j] = state_m[j - 1];
  state_m[j] = this_state_m;
  search_state->single_state_modelled_cnt[dir][mode_offset]++;
}

static AOM_INLINE void analyze_single_states(
    const AV1_COMP *cpi, InterModeSearchState *search_state) {
  const int prune_level = cpi->sf.inter_sf.prune_comp_search_by_single_result;
  assert(prune_level >= 1);
  int i, j, dir, mode;

  for (dir = 0; dir < 2; ++dir) {
    int64_t best_rd;
    SingleInterModeState(*state)[FWD_REFS];
    const int prune_factor = prune_level >= 2 ? 6 : 5;

    // Use the best rd of GLOBALMV or NEWMV to prune the unlikely
    // reference frames for all the modes (NEARESTMV and NEARMV may not
    // have same motion vectors). Always keep the best of each mode
    // because it might form the best possible combination with other mode.
    state = search_state->single_state[dir];
    best_rd = AOMMIN(state[INTER_OFFSET(NEWMV)][0].rd,
                     state[INTER_OFFSET(GLOBALMV)][0].rd);
    for (mode = 0; mode < SINGLE_INTER_MODE_NUM; ++mode) {
      for (i = 1; i < search_state->single_state_cnt[dir][mode]; ++i) {
        if (state[mode][i].rd != INT64_MAX &&
            (state[mode][i].rd >> 3) * prune_factor > best_rd) {
          state[mode][i].valid = 0;
        }
      }
    }

    state = search_state->single_state_modelled[dir];
    best_rd = AOMMIN(state[INTER_OFFSET(NEWMV)][0].rd,
                     state[INTER_OFFSET(GLOBALMV)][0].rd);
    for (mode = 0; mode < SINGLE_INTER_MODE_NUM; ++mode) {
      for (i = 1; i < search_state->single_state_modelled_cnt[dir][mode]; ++i) {
        if (state[mode][i].rd != INT64_MAX &&
            (state[mode][i].rd >> 3) * prune_factor > best_rd) {
          state[mode][i].valid = 0;
        }
      }
    }
  }

  // Ordering by simple rd first, then by modelled rd
  for (dir = 0; dir < 2; ++dir) {
    for (mode = 0; mode < SINGLE_INTER_MODE_NUM; ++mode) {
      const int state_cnt_s = search_state->single_state_cnt[dir][mode];
      const int state_cnt_m =
          search_state->single_state_modelled_cnt[dir][mode];
      SingleInterModeState *state_s = search_state->single_state[dir][mode];
      SingleInterModeState *state_m =
          search_state->single_state_modelled[dir][mode];
      int count = 0;
      const int max_candidates = AOMMAX(state_cnt_s, state_cnt_m);
      for (i = 0; i < state_cnt_s; ++i) {
        if (state_s[i].rd == INT64_MAX) break;
        if (state_s[i].valid) {
          search_state->single_rd_order[dir][mode][count++] =
              state_s[i].ref_frame;
        }
      }
      if (count >= max_candidates) continue;

      for (i = 0; i < state_cnt_m && count < max_candidates; ++i) {
        if (state_m[i].rd == INT64_MAX) break;
        if (!state_m[i].valid) continue;
        const int ref_frame = state_m[i].ref_frame;
        int match = 0;
        // Check if existing already
        for (j = 0; j < count; ++j) {
          if (search_state->single_rd_order[dir][mode][j] == ref_frame) {
            match = 1;
            break;
          }
        }
        if (match) continue;
        // Check if this ref_frame is removed in simple rd
        int valid = 1;
        for (j = 0; j < state_cnt_s; ++j) {
          if (ref_frame == state_s[j].ref_frame) {
            valid = state_s[j].valid;
            break;
          }
        }
        if (valid) {
          search_state->single_rd_order[dir][mode][count++] = ref_frame;
        }
      }
    }
  }
}

static int compound_skip_get_candidates(
    const AV1_COMP *cpi, const InterModeSearchState *search_state,
    const int dir, const PREDICTION_MODE mode) {
  const int mode_offset = INTER_OFFSET(mode);
  const SingleInterModeState *state =
      search_state->single_state[dir][mode_offset];
  const SingleInterModeState *state_modelled =
      search_state->single_state_modelled[dir][mode_offset];

  int max_candidates = 0;
  for (int i = 0; i < FWD_REFS; ++i) {
    if (search_state->single_rd_order[dir][mode_offset][i] == NONE_FRAME) break;
    max_candidates++;
  }

  int candidates = max_candidates;
  if (cpi->sf.inter_sf.prune_comp_search_by_single_result >= 2) {
    candidates = AOMMIN(2, max_candidates);
  }
  if (cpi->sf.inter_sf.prune_comp_search_by_single_result >= 3) {
    if (state[0].rd != INT64_MAX && state_modelled[0].rd != INT64_MAX &&
        state[0].ref_frame == state_modelled[0].ref_frame)
      candidates = 1;
    if (mode == NEARMV || mode == GLOBALMV) candidates = 1;
  }

  if (cpi->sf.inter_sf.prune_comp_search_by_single_result >= 4) {
    // Limit the number of candidates to 1 in each direction for compound
    // prediction
    candidates = AOMMIN(1, candidates);
  }
  return candidates;
}

static int compound_skip_by_single_states(
    const AV1_COMP *cpi, const InterModeSearchState *search_state,
    const PREDICTION_MODE this_mode, const MV_REFERENCE_FRAME ref_frame,
    const MV_REFERENCE_FRAME second_ref_frame, const MACROBLOCK *x) {
  const MV_REFERENCE_FRAME refs[2] = { ref_frame, second_ref_frame };
  const int mode[2] = { compound_ref0_mode(this_mode),
                        compound_ref1_mode(this_mode) };
  const int mode_offset[2] = { INTER_OFFSET(mode[0]), INTER_OFFSET(mode[1]) };
  const int mode_dir[2] = { refs[0] <= GOLDEN_FRAME ? 0 : 1,
                            refs[1] <= GOLDEN_FRAME ? 0 : 1 };
  int ref_searched[2] = { 0, 0 };
  int ref_mv_match[2] = { 1, 1 };
  int i, j;

  for (i = 0; i < 2; ++i) {
    const SingleInterModeState *state =
        search_state->single_state[mode_dir[i]][mode_offset[i]];
    const int state_cnt =
        search_state->single_state_cnt[mode_dir[i]][mode_offset[i]];
    for (j = 0; j < state_cnt; ++j) {
      if (state[j].ref_frame == refs[i]) {
        ref_searched[i] = 1;
        break;
      }
    }
  }

  const int ref_set = get_drl_refmv_count(x, refs, this_mode);
  for (i = 0; i < 2; ++i) {
    if (!ref_searched[i] || (mode[i] != NEARESTMV && mode[i] != NEARMV)) {
      continue;
    }
    const MV_REFERENCE_FRAME single_refs[2] = { refs[i], NONE_FRAME };
    for (int ref_mv_idx = 0; ref_mv_idx < ref_set; ref_mv_idx++) {
      int_mv single_mv;
      int_mv comp_mv;
      get_this_mv(&single_mv, mode[i], 0, ref_mv_idx, single_refs, x->mbmi_ext);
      get_this_mv(&comp_mv, this_mode, i, ref_mv_idx, refs, x->mbmi_ext);
      if (single_mv.as_int != comp_mv.as_int) {
        ref_mv_match[i] = 0;
        break;
      }
    }
  }

  for (i = 0; i < 2; ++i) {
    if (!ref_searched[i] || !ref_mv_match[i]) continue;
    const int candidates =
        compound_skip_get_candidates(cpi, search_state, mode_dir[i], mode[i]);
    const MV_REFERENCE_FRAME *ref_order =
        search_state->single_rd_order[mode_dir[i]][mode_offset[i]];
    int match = 0;
    for (j = 0; j < candidates; ++j) {
      if (refs[i] == ref_order[j]) {
        match = 1;
        break;
      }
    }
    if (!match) return 1;
  }

  return 0;
}

static int compare_int64(const void *a, const void *b) {
  int64_t a64 = *((int64_t *)a);
  int64_t b64 = *((int64_t *)b);
  if (a64 < b64) {
    return -1;
  } else if (a64 == b64) {
    return 0;
  } else {
    return 1;
  }
}

static INLINE void update_search_state(
    InterModeSearchState *search_state, RD_STATS *best_rd_stats_dst,
    PICK_MODE_CONTEXT *ctx, const RD_STATS *new_best_rd_stats,
    const RD_STATS *new_best_rd_stats_y, const RD_STATS *new_best_rd_stats_uv,
    THR_MODES new_best_mode, const MACROBLOCK *x, int txfm_search_done) {
  const MACROBLOCKD *xd = &x->e_mbd;
  const MB_MODE_INFO *mbmi = xd->mi[0];
  const int skip_ctx = av1_get_skip_context(xd);
  const int mode_is_intra =
      (av1_mode_defs[new_best_mode].mode < INTRA_MODE_END);
  const int skip = mbmi->skip && !mode_is_intra;

  search_state->best_rd = new_best_rd_stats->rdcost;
  search_state->best_mode_index = new_best_mode;
  *best_rd_stats_dst = *new_best_rd_stats;
  search_state->best_mbmode = *mbmi;
  search_state->best_skip2 = skip;
  search_state->best_mode_skippable = new_best_rd_stats->skip;
  // When !txfm_search_done, new_best_rd_stats won't provide correct rate_y and
  // rate_uv because txfm_search process is replaced by rd estimation.
  // Therfore, we should avoid updating best_rate_y and best_rate_uv here.
  // These two values will be updated when txfm_search is called.
  if (txfm_search_done) {
    search_state->best_rate_y =
        new_best_rd_stats_y->rate +
        x->skip_cost[skip_ctx][new_best_rd_stats->skip || skip];
    search_state->best_rate_uv = new_best_rd_stats_uv->rate;
  }
  memcpy(ctx->blk_skip, x->blk_skip, sizeof(x->blk_skip[0]) * ctx->num_4x4_blk);
  av1_copy_array(ctx->tx_type_map, xd->tx_type_map, ctx->num_4x4_blk);
}

// Find the best RD for a reference frame (among single reference modes)
// and store +10% of it in the 0-th element in ref_frame_rd.
static AOM_INLINE void find_top_ref(int64_t ref_frame_rd[REF_FRAMES]) {
  assert(ref_frame_rd[0] == INT64_MAX);
  int64_t ref_copy[REF_FRAMES - 1];
  memcpy(ref_copy, ref_frame_rd + 1,
         sizeof(ref_frame_rd[0]) * (REF_FRAMES - 1));
  qsort(ref_copy, REF_FRAMES - 1, sizeof(int64_t), compare_int64);

  int64_t cutoff = ref_copy[0];
  // The cut-off is within 10% of the best.
  if (cutoff != INT64_MAX) {
    assert(cutoff < INT64_MAX / 200);
    cutoff = (110 * cutoff) / 100;
  }
  ref_frame_rd[0] = cutoff;
}

// Check if either frame is within the cutoff.
static INLINE bool in_single_ref_cutoff(int64_t ref_frame_rd[REF_FRAMES],
                                        MV_REFERENCE_FRAME frame1,
                                        MV_REFERENCE_FRAME frame2) {
  assert(frame2 > 0);
  return ref_frame_rd[frame1] <= ref_frame_rd[0] ||
         ref_frame_rd[frame2] <= ref_frame_rd[0];
}

static AOM_INLINE void evaluate_motion_mode_for_winner_candidates(
    const AV1_COMP *const cpi, MACROBLOCK *const x, RD_STATS *const rd_cost,
    HandleInterModeArgs *const args, TileDataEnc *const tile_data,
    PICK_MODE_CONTEXT *const ctx,
    struct buf_2d yv12_mb[REF_FRAMES][MAX_MB_PLANE],
    const motion_mode_best_st_candidate *const best_motion_mode_cands,
    int do_tx_search, const BLOCK_SIZE bsize, int64_t *const best_est_rd,
    InterModeSearchState *const search_state) {
  const AV1_COMMON *const cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  InterModesInfo *const inter_modes_info = x->inter_modes_info;
  const int num_best_cand = best_motion_mode_cands->num_motion_mode_cand;

  for (int cand = 0; cand < num_best_cand; cand++) {
    RD_STATS rd_stats;
    RD_STATS rd_stats_y;
    RD_STATS rd_stats_uv;
    av1_init_rd_stats(&rd_stats);
    av1_init_rd_stats(&rd_stats_y);
    av1_init_rd_stats(&rd_stats_uv);
    int disable_skip = 0, rate_mv;

    rate_mv = best_motion_mode_cands->motion_mode_cand[cand].rate_mv;
    args->skip_motion_mode =
        best_motion_mode_cands->motion_mode_cand[cand].skip_motion_mode;
    *mbmi = best_motion_mode_cands->motion_mode_cand[cand].mbmi;
    rd_stats.rate =
        best_motion_mode_cands->motion_mode_cand[cand].rate2_nocoeff;

    // Continue if the best candidate is compound.
    if (!is_inter_singleref_mode(mbmi->mode)) continue;

    x->force_skip = 0;
    const int mode_index = get_prediction_mode_idx(
        mbmi->mode, mbmi->ref_frame[0], mbmi->ref_frame[1]);
    struct macroblockd_plane *p = xd->plane;
    const BUFFER_SET orig_dst = {
      { p[0].dst.buf, p[1].dst.buf, p[2].dst.buf },
      { p[0].dst.stride, p[1].dst.stride, p[2].dst.stride },
    };

    set_ref_ptrs(cm, xd, mbmi->ref_frame[0], mbmi->ref_frame[1]);
    args->simple_rd_state = x->simple_rd_state[mode_index];
    // Initialize motion mode to simple translation
    // Calculation of switchable rate depends on it.
    mbmi->motion_mode = 0;
    const int is_comp_pred = mbmi->ref_frame[1] > INTRA_FRAME;
    for (int i = 0; i < num_planes; i++) {
      xd->plane[i].pre[0] = yv12_mb[mbmi->ref_frame[0]][i];
      if (is_comp_pred) xd->plane[i].pre[1] = yv12_mb[mbmi->ref_frame[1]][i];
    }

    int64_t ret_value = motion_mode_rd(
        cpi, tile_data, x, bsize, &rd_stats, &rd_stats_y, &rd_stats_uv,
        &disable_skip, args, search_state->best_rd, &rate_mv, &orig_dst,
        best_est_rd, do_tx_search, inter_modes_info, 1);

    if (ret_value != INT64_MAX) {
      rd_stats.rdcost = RDCOST(x->rdmult, rd_stats.rate, rd_stats.dist);
      const THR_MODES mode_enum = get_prediction_mode_idx(
          mbmi->mode, mbmi->ref_frame[0], mbmi->ref_frame[1]);
      // Collect mode stats for multiwinner mode processing
      store_winner_mode_stats(
          &cpi->common, x, mbmi, &rd_stats, &rd_stats_y, &rd_stats_uv,
          mode_enum, NULL, bsize, rd_stats.rdcost,
          cpi->sf.winner_mode_sf.enable_multiwinner_mode_process, do_tx_search);
      if (rd_stats.rdcost < search_state->best_rd) {
        update_search_state(search_state, rd_cost, ctx, &rd_stats, &rd_stats_y,
                            &rd_stats_uv, mode_enum, x, do_tx_search);
      }
    }
  }
}

void av1_rd_pick_inter_mode_sb(AV1_COMP *cpi, TileDataEnc *tile_data,
                               MACROBLOCK *x, RD_STATS *rd_cost,
                               const BLOCK_SIZE bsize, PICK_MODE_CONTEXT *ctx,
                               int64_t best_rd_so_far) {
  AV1_COMMON *const cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  const SPEED_FEATURES *const sf = &cpi->sf;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  int i;
  const int *comp_inter_cost =
      x->comp_inter_cost[av1_get_reference_mode_context(xd)];

  InterModeSearchState search_state;
  init_inter_mode_search_state(&search_state, cpi, x, bsize, best_rd_so_far);
  INTERINTRA_MODE interintra_modes[REF_FRAMES] = {
    INTERINTRA_MODES, INTERINTRA_MODES, INTERINTRA_MODES, INTERINTRA_MODES,
    INTERINTRA_MODES, INTERINTRA_MODES, INTERINTRA_MODES, INTERINTRA_MODES
  };
  HandleInterModeArgs args = { { NULL },
                               { MAX_SB_SIZE, MAX_SB_SIZE, MAX_SB_SIZE },
                               { NULL },
                               { MAX_SB_SIZE >> 1, MAX_SB_SIZE >> 1,
                                 MAX_SB_SIZE >> 1 },
                               NULL,
                               NULL,
                               NULL,
                               search_state.modelled_rd,
                               INT_MAX,
                               INT_MAX,
                               search_state.simple_rd,
                               0,
                               interintra_modes,
                               1,
                               NULL,
                               { { { 0 }, { { 0 } }, { 0 }, 0, 0, 0, 0 } },
                               0 };
  const int max_winner_motion_mode_cand = cpi->num_winner_motion_modes;
  motion_mode_candidate motion_mode_cand;
  motion_mode_best_st_candidate best_motion_mode_cands;
  // Initializing the number of motion mode candidates to zero.
  best_motion_mode_cands.num_motion_mode_cand = 0;
  for (i = 0; i < MAX_WINNER_MOTION_MODES; ++i)
    best_motion_mode_cands.motion_mode_cand[i].rd_cost = INT64_MAX;

  for (i = 0; i < REF_FRAMES; ++i) x->pred_sse[i] = INT_MAX;

  av1_invalid_rd_stats(rd_cost);

  // Ref frames that are selected by square partition blocks.
  int picked_ref_frames_mask = 0;
  if (cpi->sf.inter_sf.prune_ref_frame_for_rect_partitions &&
      mbmi->partition != PARTITION_NONE && mbmi->partition != PARTITION_SPLIT) {
    // prune_ref_frame_for_rect_partitions = 1 implies prune only extended
    // partition blocks. prune_ref_frame_for_rect_partitions >=2
    // implies prune for vert, horiz and extended partition blocks.
    if ((mbmi->partition != PARTITION_VERT &&
         mbmi->partition != PARTITION_HORZ) ||
        cpi->sf.inter_sf.prune_ref_frame_for_rect_partitions >= 2) {
      picked_ref_frames_mask =
          fetch_picked_ref_frames_mask(x, bsize, cm->seq_params.mib_size);
    }
  }

  // Skip ref frames that never selected by square blocks.
  const int skip_ref_frame_mask =
      picked_ref_frames_mask ? ~picked_ref_frames_mask : 0;
  mode_skip_mask_t mode_skip_mask;
  unsigned int ref_costs_single[REF_FRAMES];
  unsigned int ref_costs_comp[REF_FRAMES][REF_FRAMES];
  struct buf_2d yv12_mb[REF_FRAMES][MAX_MB_PLANE];
  // init params, set frame modes, speed features
  set_params_rd_pick_inter_mode(cpi, x, &args, bsize, &mode_skip_mask,
                                skip_ref_frame_mask, ref_costs_single,
                                ref_costs_comp, yv12_mb);

  int64_t best_est_rd = INT64_MAX;
  const InterModeRdModel *md = &tile_data->inter_mode_rd_models[bsize];
  // If do_tx_search is 0, only estimated RD should be computed.
  // If do_tx_search is 1, all modes have TX search performed.
  const int do_tx_search =
      !((cpi->sf.inter_sf.inter_mode_rd_model_estimation == 1 && md->ready) ||
        (cpi->sf.inter_sf.inter_mode_rd_model_estimation == 2 &&
         num_pels_log2_lookup[bsize] > 8) ||
        cpi->sf.rt_sf.force_tx_search_off);
  InterModesInfo *inter_modes_info = x->inter_modes_info;
  inter_modes_info->num = 0;

  int intra_mode_num = 0;
  int intra_mode_idx_ls[INTRA_MODES];
  int reach_first_comp_mode = 0;

  // Temporary buffers used by handle_inter_mode().
  uint8_t *const tmp_buf = get_buf_by_bd(xd, x->tmp_obmc_bufs[0]);

  // The best RD found for the reference frame, among single reference modes.
  // Note that the 0-th element will contain a cut-off that is later used
  // to determine if we should skip a compound mode.
  int64_t ref_frame_rd[REF_FRAMES] = { INT64_MAX, INT64_MAX, INT64_MAX,
                                       INT64_MAX, INT64_MAX, INT64_MAX,
                                       INT64_MAX, INT64_MAX };
  const int skip_ctx = av1_get_skip_context(xd);

  // Prepared stats used later to check if we could skip intra mode eval.
  int64_t inter_cost = -1;
  int64_t intra_cost = -1;
  // Need to tweak the threshold for hdres speed 0 & 1.
  const int mi_row = xd->mi_row;
  const int mi_col = xd->mi_col;
  const int do_pruning =
      (AOMMIN(cm->width, cm->height) > 480 && cpi->speed <= 1) ? 0 : 1;
  if (do_pruning && sf->intra_sf.skip_intra_in_interframe) {
    // Only consider full SB.
    int len = tpl_blocks_in_sb(cm->seq_params.sb_size);
    if (len == x->valid_cost_b) {
      const BLOCK_SIZE tpl_bsize = convert_length_to_bsize(MC_FLOW_BSIZE_1D);
      const int tplw = mi_size_wide[tpl_bsize];
      const int tplh = mi_size_high[tpl_bsize];
      const int nw = mi_size_wide[bsize] / tplw;
      const int nh = mi_size_high[bsize] / tplh;
      if (nw >= 1 && nh >= 1) {
        const int of_h = mi_row % mi_size_high[cm->seq_params.sb_size];
        const int of_w = mi_col % mi_size_wide[cm->seq_params.sb_size];
        const int start = of_h / tplh * x->cost_stride + of_w / tplw;

        for (int k = 0; k < nh; k++) {
          for (int l = 0; l < nw; l++) {
            inter_cost += x->inter_cost_b[start + k * x->cost_stride + l];
            intra_cost += x->intra_cost_b[start + k * x->cost_stride + l];
          }
        }
        inter_cost /= nw * nh;
        intra_cost /= nw * nh;
      }
    }
  }

  const int last_single_ref_mode_idx =
      find_last_single_ref_mode_idx(av1_default_mode_order);
  int prune_cpd_using_sr_stats_ready = 0;

  // Initialize best mode stats for winner mode processing
  av1_zero(x->winner_mode_stats);
  x->winner_mode_count = 0;
  store_winner_mode_stats(
      &cpi->common, x, mbmi, NULL, NULL, NULL, THR_INVALID, NULL, bsize,
      best_rd_so_far, cpi->sf.winner_mode_sf.enable_multiwinner_mode_process,
      0);

  // Here midx is just an interator index that should not be used by itself
  // except to keep track of the number of modes searched. It should be used
  // with av1_default_mode_order to get the enum that defines the mode, which
  // can be used with av1_mode_defs to get the prediction mode and the ref
  // frames.
  for (int midx = 0; midx < MAX_MODES; ++midx) {
    // After we done with single reference modes, find the 2nd best RD
    // for a reference frame. Only search compound modes that have a reference
    // frame at least as good as the 2nd best.
    if (sf->inter_sf.prune_compound_using_single_ref &&
        midx == last_single_ref_mode_idx + 1) {
      find_top_ref(ref_frame_rd);
      prune_cpd_using_sr_stats_ready = 1;
    }

    const THR_MODES mode_enum = av1_default_mode_order[midx];
    const MODE_DEFINITION *mode_def = &av1_mode_defs[mode_enum];
    const PREDICTION_MODE this_mode = mode_def->mode;
    const MV_REFERENCE_FRAME *ref_frames = mode_def->ref_frame;

    if (inter_mode_compatible_skip(cpi, x, bsize, this_mode, ref_frames))
      continue;
    const int ret = inter_mode_search_order_independent_skip(
        cpi, x, &mode_skip_mask, &search_state, skip_ref_frame_mask, this_mode,
        mode_def->ref_frame);
    if (ret == 1) continue;
    args.skip_motion_mode = (ret == 2);

    const MV_REFERENCE_FRAME ref_frame = ref_frames[0];
    const MV_REFERENCE_FRAME second_ref_frame = ref_frames[1];
    const int is_single_pred =
        ref_frame > INTRA_FRAME && second_ref_frame == NONE_FRAME;
    const int comp_pred = second_ref_frame > INTRA_FRAME;

    if (sf->inter_sf.prune_compound_using_single_ref &&
        prune_cpd_using_sr_stats_ready && comp_pred &&
        !in_single_ref_cutoff(ref_frame_rd, ref_frame, second_ref_frame)) {
      continue;
    }

    // Reach the first compound prediction mode
    if (sf->inter_sf.prune_comp_search_by_single_result > 0 && comp_pred &&
        reach_first_comp_mode == 0) {
      analyze_single_states(cpi, &search_state);
      reach_first_comp_mode = 1;
    }

    init_mbmi(mbmi, this_mode, ref_frames, cm);

    x->force_skip = 0;
    set_ref_ptrs(cm, xd, ref_frame, second_ref_frame);

    if (search_state.best_rd < search_state.mode_threshold[mode_enum]) continue;

    if (sf->inter_sf.prune_comp_search_by_single_result > 0 && comp_pred) {
      if (compound_skip_by_single_states(cpi, &search_state, this_mode,
                                         ref_frame, second_ref_frame, x))
        continue;
    }

    const int compmode_cost =
        is_comp_ref_allowed(mbmi->sb_type) ? comp_inter_cost[comp_pred] : 0;
    const int real_compmode_cost =
        cm->current_frame.reference_mode == REFERENCE_MODE_SELECT
            ? compmode_cost
            : 0;

    if (ref_frame == INTRA_FRAME) {
      if ((!cpi->oxcf.enable_smooth_intra ||
           sf->intra_sf.disable_smooth_intra) &&
          (mbmi->mode == SMOOTH_PRED || mbmi->mode == SMOOTH_H_PRED ||
           mbmi->mode == SMOOTH_V_PRED))
        continue;
      if (!cpi->oxcf.enable_paeth_intra && mbmi->mode == PAETH_PRED) continue;
      if (sf->inter_sf.adaptive_mode_search > 1)
        if ((x->source_variance << num_pels_log2_lookup[bsize]) >
            search_state.best_pred_sse)
          continue;

      // Intra modes will be handled in another loop later.
      assert(intra_mode_num < INTRA_MODES);
      intra_mode_idx_ls[intra_mode_num++] = mode_enum;
      continue;
    }

    // Select prediction reference frames.
    for (i = 0; i < num_planes; i++) {
      xd->plane[i].pre[0] = yv12_mb[ref_frame][i];
      if (comp_pred) xd->plane[i].pre[1] = yv12_mb[second_ref_frame][i];
    }

    mbmi->angle_delta[PLANE_TYPE_Y] = 0;
    mbmi->angle_delta[PLANE_TYPE_UV] = 0;
    mbmi->filter_intra_mode_info.use_filter_intra = 0;
    mbmi->ref_mv_idx = 0;

    const int64_t ref_best_rd = search_state.best_rd;
    int disable_skip = 0;
    RD_STATS rd_stats, rd_stats_y, rd_stats_uv;
    av1_init_rd_stats(&rd_stats);

    const int ref_frame_cost = comp_pred
                                   ? ref_costs_comp[ref_frame][second_ref_frame]
                                   : ref_costs_single[ref_frame];
    // Point to variables that are maintained between loop iterations
    args.single_newmv = search_state.single_newmv;
    args.single_newmv_rate = search_state.single_newmv_rate;
    args.single_newmv_valid = search_state.single_newmv_valid;
    args.single_comp_cost = real_compmode_cost;
    args.ref_frame_cost = ref_frame_cost;
    if (is_single_pred) {
      args.simple_rd_state = x->simple_rd_state[mode_enum];
    }

    int64_t this_rd = handle_inter_mode(
        cpi, tile_data, x, bsize, &rd_stats, &rd_stats_y, &rd_stats_uv,
        &disable_skip, &args, ref_best_rd, tmp_buf, &x->comp_rd_buffer,
        &best_est_rd, do_tx_search, inter_modes_info, &motion_mode_cand);

    if (sf->inter_sf.prune_comp_search_by_single_result > 0 &&
        is_inter_singleref_mode(this_mode) && args.single_ref_first_pass) {
      collect_single_states(x, &search_state, mbmi);
    }

    if (this_rd == INT64_MAX) continue;

    if (mbmi->skip) {
      rd_stats_y.rate = 0;
      rd_stats_uv.rate = 0;
    }

    if (sf->inter_sf.prune_compound_using_single_ref && is_single_pred &&
        this_rd < ref_frame_rd[ref_frame]) {
      ref_frame_rd[ref_frame] = this_rd;
    }

    // Did this mode help, i.e., is it the new best mode
    if (this_rd < search_state.best_rd) {
      assert(IMPLIES(comp_pred,
                     cm->current_frame.reference_mode != SINGLE_REFERENCE));
      search_state.best_pred_sse = x->pred_sse[ref_frame];
      update_search_state(&search_state, rd_cost, ctx, &rd_stats, &rd_stats_y,
                          &rd_stats_uv, mode_enum, x, do_tx_search);
    }
    if (cpi->sf.winner_mode_sf.motion_mode_for_winner_cand) {
      const int num_motion_mode_cand =
          best_motion_mode_cands.num_motion_mode_cand;
      int valid_motion_mode_cand_loc = num_motion_mode_cand;

      // find the best location to insert new motion mode candidate
      for (int j = 0; j < num_motion_mode_cand; j++) {
        if (this_rd < best_motion_mode_cands.motion_mode_cand[j].rd_cost) {
          valid_motion_mode_cand_loc = j;
          break;
        }
      }

      if (valid_motion_mode_cand_loc < max_winner_motion_mode_cand) {
        if (num_motion_mode_cand > 0 &&
            valid_motion_mode_cand_loc < max_winner_motion_mode_cand - 1)
          memmove(
              &best_motion_mode_cands
                   .motion_mode_cand[valid_motion_mode_cand_loc + 1],
              &best_motion_mode_cands
                   .motion_mode_cand[valid_motion_mode_cand_loc],
              (AOMMIN(num_motion_mode_cand, max_winner_motion_mode_cand - 1) -
               valid_motion_mode_cand_loc) *
                  sizeof(best_motion_mode_cands.motion_mode_cand[0]));
        motion_mode_cand.mbmi = *mbmi;
        motion_mode_cand.rd_cost = this_rd;
        motion_mode_cand.skip_motion_mode = args.skip_motion_mode;
        best_motion_mode_cands.motion_mode_cand[valid_motion_mode_cand_loc] =
            motion_mode_cand;
        best_motion_mode_cands.num_motion_mode_cand =
            AOMMIN(max_winner_motion_mode_cand,
                   best_motion_mode_cands.num_motion_mode_cand + 1);
      }
    }

    /* keep record of best compound/single-only prediction */
    if (!disable_skip) {
      int64_t single_rd, hybrid_rd, single_rate, hybrid_rate;

      if (cm->current_frame.reference_mode == REFERENCE_MODE_SELECT) {
        single_rate = rd_stats.rate - compmode_cost;
        hybrid_rate = rd_stats.rate;
      } else {
        single_rate = rd_stats.rate;
        hybrid_rate = rd_stats.rate + compmode_cost;
      }

      single_rd = RDCOST(x->rdmult, single_rate, rd_stats.dist);
      hybrid_rd = RDCOST(x->rdmult, hybrid_rate, rd_stats.dist);

      if (!comp_pred) {
        if (single_rd < search_state.best_pred_rd[SINGLE_REFERENCE])
          search_state.best_pred_rd[SINGLE_REFERENCE] = single_rd;
      } else {
        if (single_rd < search_state.best_pred_rd[COMPOUND_REFERENCE])
          search_state.best_pred_rd[COMPOUND_REFERENCE] = single_rd;
      }
      if (hybrid_rd < search_state.best_pred_rd[REFERENCE_MODE_SELECT])
        search_state.best_pred_rd[REFERENCE_MODE_SELECT] = hybrid_rd;
    }

    // TODO(anyone): evaluate the quality and speed trade-off of the early
    // termination logic below.
    // if (x->force_skip && !comp_pred) break;
  }

  if (cpi->sf.winner_mode_sf.motion_mode_for_winner_cand) {
    // For the single ref winner candidates, evaluate other motion modes (non
    // simple translation).
    evaluate_motion_mode_for_winner_candidates(
        cpi, x, rd_cost, &args, tile_data, ctx, yv12_mb,
        &best_motion_mode_cands, do_tx_search, bsize, &best_est_rd,
        &search_state);
  }

#if CONFIG_COLLECT_COMPONENT_TIMING
  start_timing(cpi, do_tx_search_time);
#endif
  if (do_tx_search != 1) {
    inter_modes_info_sort(inter_modes_info, inter_modes_info->rd_idx_pair_arr);
    search_state.best_rd = best_rd_so_far;
    search_state.best_mode_index = THR_INVALID;
    // Initialize best mode stats for winner mode processing
    x->winner_mode_count = 0;
    store_winner_mode_stats(
        &cpi->common, x, mbmi, NULL, NULL, NULL, THR_INVALID, NULL, bsize,
        best_rd_so_far, cpi->sf.winner_mode_sf.enable_multiwinner_mode_process,
        do_tx_search);
    inter_modes_info->num =
        inter_modes_info->num < cpi->sf.rt_sf.num_inter_modes_for_tx_search
            ? inter_modes_info->num
            : cpi->sf.rt_sf.num_inter_modes_for_tx_search;
    const int64_t top_est_rd =
        inter_modes_info->num > 0
            ? inter_modes_info
                  ->est_rd_arr[inter_modes_info->rd_idx_pair_arr[0].idx]
            : INT64_MAX;
    for (int j = 0; j < inter_modes_info->num; ++j) {
      const int data_idx = inter_modes_info->rd_idx_pair_arr[j].idx;
      *mbmi = inter_modes_info->mbmi_arr[data_idx];
      int64_t curr_est_rd = inter_modes_info->est_rd_arr[data_idx];
      if (curr_est_rd * 0.80 > top_est_rd) break;

      x->force_skip = 0;
      set_ref_ptrs(cm, xd, mbmi->ref_frame[0], mbmi->ref_frame[1]);

      // Select prediction reference frames.
      const int is_comp_pred = mbmi->ref_frame[1] > INTRA_FRAME;
      for (i = 0; i < num_planes; i++) {
        xd->plane[i].pre[0] = yv12_mb[mbmi->ref_frame[0]][i];
        if (is_comp_pred) xd->plane[i].pre[1] = yv12_mb[mbmi->ref_frame[1]][i];
      }

      av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, NULL, bsize, 0,
                                    av1_num_planes(cm) - 1);
      if (mbmi->motion_mode == OBMC_CAUSAL) {
        av1_build_obmc_inter_predictors_sb(cm, xd);
      }

      RD_STATS rd_stats;
      RD_STATS rd_stats_y;
      RD_STATS rd_stats_uv;
      const int mode_rate = inter_modes_info->mode_rate_arr[data_idx];
      if (!txfm_search(cpi, tile_data, x, bsize, &rd_stats, &rd_stats_y,
                       &rd_stats_uv, mode_rate, search_state.best_rd)) {
        continue;
      } else if (cpi->sf.inter_sf.inter_mode_rd_model_estimation == 1) {
        inter_mode_data_push(tile_data, mbmi->sb_type, rd_stats.sse,
                             rd_stats.dist,
                             rd_stats_y.rate + rd_stats_uv.rate +
                                 x->skip_cost[skip_ctx][mbmi->skip]);
      }
      rd_stats.rdcost = RDCOST(x->rdmult, rd_stats.rate, rd_stats.dist);
      // TODO(chiyotsai@google.com): get_prediction_mode_idx gives incorrect
      // output once we change the mode order. Fix this!
      const THR_MODES mode_enum = get_prediction_mode_idx(
          mbmi->mode, mbmi->ref_frame[0], mbmi->ref_frame[1]);

      // Collect mode stats for multiwinner mode processing
      const int txfm_search_done = 1;
      store_winner_mode_stats(
          &cpi->common, x, mbmi, &rd_stats, &rd_stats_y, &rd_stats_uv,
          mode_enum, NULL, bsize, rd_stats.rdcost,
          cpi->sf.winner_mode_sf.enable_multiwinner_mode_process,
          txfm_search_done);

      if (rd_stats.rdcost < search_state.best_rd) {
        update_search_state(&search_state, rd_cost, ctx, &rd_stats, &rd_stats_y,
                            &rd_stats_uv, mode_enum, x, txfm_search_done);
      }
    }
  }
#if CONFIG_COLLECT_COMPONENT_TIMING
  end_timing(cpi, do_tx_search_time);
#endif

#if CONFIG_COLLECT_COMPONENT_TIMING
  start_timing(cpi, handle_intra_mode_time);
#endif

  // Gate intra mode evaluation if best of inter is skip except when source
  // variance is extremely low
  if (sf->intra_sf.skip_intra_in_interframe &&
      (x->source_variance > sf->intra_sf.src_var_thresh_intra_skip)) {
    if (inter_cost >= 0 && intra_cost >= 0) {
      aom_clear_system_state();
      const NN_CONFIG *nn_config = (AOMMIN(cm->width, cm->height) <= 480)
                                       ? &av1_intrap_nn_config
                                       : &av1_intrap_hd_nn_config;
      float features[6];
      float scores[2] = { 0.0f };
      float probs[2] = { 0.0f };

      features[0] = (float)search_state.best_mbmode.skip;
      features[1] = (float)mi_size_wide_log2[bsize];
      features[2] = (float)mi_size_high_log2[bsize];
      features[3] = (float)intra_cost;
      features[4] = (float)inter_cost;
      const int ac_q = av1_ac_quant_QTX(x->qindex, 0, xd->bd);
      const int ac_q_max = av1_ac_quant_QTX(255, 0, xd->bd);
      features[5] = (float)(ac_q_max / ac_q);

      av1_nn_predict(features, nn_config, 1, scores);
      aom_clear_system_state();
      av1_nn_softmax(scores, probs, 2);

      if (probs[1] > 0.8) search_state.skip_intra_modes = 1;
    } else if ((search_state.best_mbmode.skip) &&
               (sf->intra_sf.skip_intra_in_interframe >= 2)) {
      search_state.skip_intra_modes = 1;
    }
  }

  const int intra_ref_frame_cost = ref_costs_single[INTRA_FRAME];
  for (int j = 0; j < intra_mode_num; ++j) {
    if (sf->intra_sf.skip_intra_in_interframe && search_state.skip_intra_modes)
      break;
    const THR_MODES mode_enum = intra_mode_idx_ls[j];
    const MODE_DEFINITION *mode_def = &av1_mode_defs[mode_enum];
    const PREDICTION_MODE this_mode = mode_def->mode;

    assert(av1_mode_defs[mode_enum].ref_frame[0] == INTRA_FRAME);
    assert(av1_mode_defs[mode_enum].ref_frame[1] == NONE_FRAME);
    init_mbmi(mbmi, this_mode, av1_mode_defs[mode_enum].ref_frame, cm);
    x->force_skip = 0;

    if (this_mode != DC_PRED) {
      // Only search the oblique modes if the best so far is
      // one of the neighboring directional modes
      if ((sf->rt_sf.mode_search_skip_flags & FLAG_SKIP_INTRA_BESTINTER) &&
          (this_mode >= D45_PRED && this_mode <= PAETH_PRED)) {
        if (search_state.best_mode_index != THR_INVALID &&
            search_state.best_mbmode.ref_frame[0] > INTRA_FRAME)
          continue;
      }
      if (sf->rt_sf.mode_search_skip_flags & FLAG_SKIP_INTRA_DIRMISMATCH) {
        if (conditional_skipintra(this_mode, search_state.best_intra_mode))
          continue;
      }
    }

    RD_STATS intra_rd_stats, intra_rd_stats_y, intra_rd_stats_uv;
    intra_rd_stats.rdcost = handle_intra_mode(
        &search_state, cpi, x, bsize, intra_ref_frame_cost, ctx, 0,
        &intra_rd_stats, &intra_rd_stats_y, &intra_rd_stats_uv);
    // Collect mode stats for multiwinner mode processing
    const int txfm_search_done = 1;
    store_winner_mode_stats(
        &cpi->common, x, mbmi, &intra_rd_stats, &intra_rd_stats_y,
        &intra_rd_stats_uv, mode_enum, NULL, bsize, intra_rd_stats.rdcost,
        cpi->sf.winner_mode_sf.enable_multiwinner_mode_process,
        txfm_search_done);
    if (intra_rd_stats.rdcost < search_state.best_rd) {
      update_search_state(&search_state, rd_cost, ctx, &intra_rd_stats,
                          &intra_rd_stats_y, &intra_rd_stats_uv, mode_enum, x,
                          txfm_search_done);
    }
  }
#if CONFIG_COLLECT_COMPONENT_TIMING
  end_timing(cpi, handle_intra_mode_time);
#endif

  int winner_mode_count = cpi->sf.winner_mode_sf.enable_multiwinner_mode_process
                              ? x->winner_mode_count
                              : 1;
  // In effect only when fast tx search speed features are enabled.
  refine_winner_mode_tx(
      cpi, x, rd_cost, bsize, ctx, &search_state.best_mode_index,
      &search_state.best_mbmode, yv12_mb, search_state.best_rate_y,
      search_state.best_rate_uv, &search_state.best_skip2, winner_mode_count);

  // Initialize default mode evaluation params
  set_mode_eval_params(cpi, x, DEFAULT_EVAL);

  // Only try palette mode when the best mode so far is an intra mode.
  const int try_palette =
      cpi->oxcf.enable_palette &&
      av1_allow_palette(cm->allow_screen_content_tools, mbmi->sb_type) &&
      !is_inter_mode(search_state.best_mbmode.mode);
  PALETTE_MODE_INFO *const pmi = &mbmi->palette_mode_info;
  if (try_palette) {
    search_palette_mode(cpi, x, rd_cost, ctx, bsize, mbmi, pmi,
                        ref_costs_single, &search_state);
  }

  search_state.best_mbmode.skip_mode = 0;
  if (cm->current_frame.skip_mode_info.skip_mode_flag &&
      is_comp_ref_allowed(bsize)) {
    const struct segmentation *const seg = &cm->seg;
    unsigned char segment_id = mbmi->segment_id;
    if (!segfeature_active(seg, segment_id, SEG_LVL_REF_FRAME)) {
      rd_pick_skip_mode(rd_cost, &search_state, cpi, x, bsize, yv12_mb);
    }
  }

  // Make sure that the ref_mv_idx is only nonzero when we're
  // using a mode which can support ref_mv_idx
  if (search_state.best_mbmode.ref_mv_idx != 0 &&
      !(search_state.best_mbmode.mode == NEWMV ||
        search_state.best_mbmode.mode == NEW_NEWMV ||
        have_nearmv_in_inter_mode(search_state.best_mbmode.mode))) {
    search_state.best_mbmode.ref_mv_idx = 0;
  }

  if (search_state.best_mode_index == THR_INVALID ||
      search_state.best_rd >= best_rd_so_far) {
    rd_cost->rate = INT_MAX;
    rd_cost->rdcost = INT64_MAX;
    return;
  }

  assert((cm->interp_filter == SWITCHABLE) ||
         (cm->interp_filter ==
          search_state.best_mbmode.interp_filters.as_filters.y_filter) ||
         !is_inter_block(&search_state.best_mbmode));
  assert((cm->interp_filter == SWITCHABLE) ||
         (cm->interp_filter ==
          search_state.best_mbmode.interp_filters.as_filters.x_filter) ||
         !is_inter_block(&search_state.best_mbmode));

  if (!cpi->rc.is_src_frame_alt_ref) {
    av1_update_rd_thresh_fact(cm, x->thresh_freq_fact,
                              sf->inter_sf.adaptive_rd_thresh, bsize,
                              search_state.best_mode_index);
  }

  // macroblock modes
  *mbmi = search_state.best_mbmode;
  x->force_skip |= search_state.best_skip2;

  // Note: this section is needed since the mode may have been forced to
  // GLOBALMV by the all-zero mode handling of ref-mv.
  if (mbmi->mode == GLOBALMV || mbmi->mode == GLOBAL_GLOBALMV) {
    // Correct the interp filters for GLOBALMV
    if (is_nontrans_global_motion(xd, xd->mi[0])) {
      int_interpfilters filters = av1_broadcast_interp_filter(
          av1_unswitchable_filter(cm->interp_filter));
      assert(mbmi->interp_filters.as_int == filters.as_int);
      (void)filters;
    }
  }

  for (i = 0; i < REFERENCE_MODES; ++i) {
    if (search_state.best_pred_rd[i] == INT64_MAX) {
      search_state.best_pred_diff[i] = INT_MIN;
    } else {
      search_state.best_pred_diff[i] =
          search_state.best_rd - search_state.best_pred_rd[i];
    }
  }

  x->force_skip |= search_state.best_mode_skippable;

  assert(search_state.best_mode_index != THR_INVALID);

#if CONFIG_INTERNAL_STATS
  store_coding_context(x, ctx, search_state.best_mode_index,
                       search_state.best_pred_diff,
                       search_state.best_mode_skippable);
#else
  store_coding_context(x, ctx, search_state.best_pred_diff,
                       search_state.best_mode_skippable);
#endif  // CONFIG_INTERNAL_STATS

  if (pmi->palette_size[1] > 0) {
    assert(try_palette);
    restore_uv_color_map(cpi, x);
  }
}

void av1_rd_pick_inter_mode_sb_seg_skip(const AV1_COMP *cpi,
                                        TileDataEnc *tile_data, MACROBLOCK *x,
                                        int mi_row, int mi_col,
                                        RD_STATS *rd_cost, BLOCK_SIZE bsize,
                                        PICK_MODE_CONTEXT *ctx,
                                        int64_t best_rd_so_far) {
  const AV1_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  unsigned char segment_id = mbmi->segment_id;
  const int comp_pred = 0;
  int i;
  int64_t best_pred_diff[REFERENCE_MODES];
  unsigned int ref_costs_single[REF_FRAMES];
  unsigned int ref_costs_comp[REF_FRAMES][REF_FRAMES];
  int *comp_inter_cost = x->comp_inter_cost[av1_get_reference_mode_context(xd)];
  InterpFilter best_filter = SWITCHABLE;
  int64_t this_rd = INT64_MAX;
  int rate2 = 0;
  const int64_t distortion2 = 0;
  (void)mi_row;
  (void)mi_col;
  (void)tile_data;

  av1_collect_neighbors_ref_counts(xd);

  estimate_ref_frame_costs(cm, xd, x, segment_id, ref_costs_single,
                           ref_costs_comp);

  for (i = 0; i < REF_FRAMES; ++i) x->pred_sse[i] = INT_MAX;
  for (i = LAST_FRAME; i < REF_FRAMES; ++i) x->pred_mv_sad[i] = INT_MAX;

  rd_cost->rate = INT_MAX;

  assert(segfeature_active(&cm->seg, segment_id, SEG_LVL_SKIP));

  mbmi->palette_mode_info.palette_size[0] = 0;
  mbmi->palette_mode_info.palette_size[1] = 0;
  mbmi->filter_intra_mode_info.use_filter_intra = 0;
  mbmi->mode = GLOBALMV;
  mbmi->motion_mode = SIMPLE_TRANSLATION;
  mbmi->uv_mode = UV_DC_PRED;
  if (segfeature_active(&cm->seg, segment_id, SEG_LVL_REF_FRAME))
    mbmi->ref_frame[0] = get_segdata(&cm->seg, segment_id, SEG_LVL_REF_FRAME);
  else
    mbmi->ref_frame[0] = LAST_FRAME;
  mbmi->ref_frame[1] = NONE_FRAME;
  mbmi->mv[0].as_int =
      gm_get_motion_vector(&cm->global_motion[mbmi->ref_frame[0]],
                           cm->allow_high_precision_mv, bsize, mi_col, mi_row,
                           cm->cur_frame_force_integer_mv)
          .as_int;
  mbmi->tx_size = max_txsize_lookup[bsize];
  x->force_skip = 1;

  mbmi->ref_mv_idx = 0;

  mbmi->motion_mode = SIMPLE_TRANSLATION;
  av1_count_overlappable_neighbors(cm, xd);
  if (is_motion_variation_allowed_bsize(bsize) && !has_second_ref(mbmi)) {
    int pts[SAMPLES_ARRAY_SIZE], pts_inref[SAMPLES_ARRAY_SIZE];
    mbmi->num_proj_ref = av1_findSamples(cm, xd, pts, pts_inref);
    // Select the samples according to motion vector difference
    if (mbmi->num_proj_ref > 1)
      mbmi->num_proj_ref = av1_selectSamples(&mbmi->mv[0].as_mv, pts, pts_inref,
                                             mbmi->num_proj_ref, bsize);
  }

  set_default_interp_filters(mbmi, cm->interp_filter);

  if (cm->interp_filter != SWITCHABLE) {
    best_filter = cm->interp_filter;
  } else {
    best_filter = EIGHTTAP_REGULAR;
    if (av1_is_interp_needed(xd) &&
        x->source_variance >=
            cpi->sf.interp_sf.disable_filter_search_var_thresh) {
      int rs;
      int best_rs = INT_MAX;
      for (i = 0; i < SWITCHABLE_FILTERS; ++i) {
        mbmi->interp_filters = av1_broadcast_interp_filter(i);
        rs = av1_get_switchable_rate(cm, x, xd);
        if (rs < best_rs) {
          best_rs = rs;
          best_filter = mbmi->interp_filters.as_filters.y_filter;
        }
      }
    }
  }
  // Set the appropriate filter
  mbmi->interp_filters = av1_broadcast_interp_filter(best_filter);
  rate2 += av1_get_switchable_rate(cm, x, xd);

  if (cm->current_frame.reference_mode == REFERENCE_MODE_SELECT)
    rate2 += comp_inter_cost[comp_pred];

  // Estimate the reference frame signaling cost and add it
  // to the rolling cost variable.
  rate2 += ref_costs_single[LAST_FRAME];
  this_rd = RDCOST(x->rdmult, rate2, distortion2);

  rd_cost->rate = rate2;
  rd_cost->dist = distortion2;
  rd_cost->rdcost = this_rd;

  if (this_rd >= best_rd_so_far) {
    rd_cost->rate = INT_MAX;
    rd_cost->rdcost = INT64_MAX;
    return;
  }

  assert((cm->interp_filter == SWITCHABLE) ||
         (cm->interp_filter == mbmi->interp_filters.as_filters.y_filter));

  av1_update_rd_thresh_fact(cm, x->thresh_freq_fact,
                            cpi->sf.inter_sf.adaptive_rd_thresh, bsize,
                            THR_GLOBALMV);

  av1_zero(best_pred_diff);

#if CONFIG_INTERNAL_STATS
  store_coding_context(x, ctx, THR_GLOBALMV, best_pred_diff, 0);
#else
  store_coding_context(x, ctx, best_pred_diff, 0);
#endif  // CONFIG_INTERNAL_STATS
}

struct calc_target_weighted_pred_ctxt {
  const MACROBLOCK *x;
  const uint8_t *tmp;
  int tmp_stride;
  int overlap;
};

static INLINE void calc_target_weighted_pred_above(
    MACROBLOCKD *xd, int rel_mi_row, int rel_mi_col, uint8_t op_mi_size,
    int dir, MB_MODE_INFO *nb_mi, void *fun_ctxt, const int num_planes) {
  (void)nb_mi;
  (void)num_planes;
  (void)rel_mi_row;
  (void)dir;

  struct calc_target_weighted_pred_ctxt *ctxt =
      (struct calc_target_weighted_pred_ctxt *)fun_ctxt;

  const int bw = xd->n4_w << MI_SIZE_LOG2;
  const uint8_t *const mask1d = av1_get_obmc_mask(ctxt->overlap);

  int32_t *wsrc = ctxt->x->wsrc_buf + (rel_mi_col * MI_SIZE);
  int32_t *mask = ctxt->x->mask_buf + (rel_mi_col * MI_SIZE);
  const uint8_t *tmp = ctxt->tmp + rel_mi_col * MI_SIZE;
  const int is_hbd = is_cur_buf_hbd(xd);

  if (!is_hbd) {
    for (int row = 0; row < ctxt->overlap; ++row) {
      const uint8_t m0 = mask1d[row];
      const uint8_t m1 = AOM_BLEND_A64_MAX_ALPHA - m0;
      for (int col = 0; col < op_mi_size * MI_SIZE; ++col) {
        wsrc[col] = m1 * tmp[col];
        mask[col] = m0;
      }
      wsrc += bw;
      mask += bw;
      tmp += ctxt->tmp_stride;
    }
  } else {
    const uint16_t *tmp16 = CONVERT_TO_SHORTPTR(tmp);

    for (int row = 0; row < ctxt->overlap; ++row) {
      const uint8_t m0 = mask1d[row];
      const uint8_t m1 = AOM_BLEND_A64_MAX_ALPHA - m0;
      for (int col = 0; col < op_mi_size * MI_SIZE; ++col) {
        wsrc[col] = m1 * tmp16[col];
        mask[col] = m0;
      }
      wsrc += bw;
      mask += bw;
      tmp16 += ctxt->tmp_stride;
    }
  }
}

static INLINE void calc_target_weighted_pred_left(
    MACROBLOCKD *xd, int rel_mi_row, int rel_mi_col, uint8_t op_mi_size,
    int dir, MB_MODE_INFO *nb_mi, void *fun_ctxt, const int num_planes) {
  (void)nb_mi;
  (void)num_planes;
  (void)rel_mi_col;
  (void)dir;

  struct calc_target_weighted_pred_ctxt *ctxt =
      (struct calc_target_weighted_pred_ctxt *)fun_ctxt;

  const int bw = xd->n4_w << MI_SIZE_LOG2;
  const uint8_t *const mask1d = av1_get_obmc_mask(ctxt->overlap);

  int32_t *wsrc = ctxt->x->wsrc_buf + (rel_mi_row * MI_SIZE * bw);
  int32_t *mask = ctxt->x->mask_buf + (rel_mi_row * MI_SIZE * bw);
  const uint8_t *tmp = ctxt->tmp + (rel_mi_row * MI_SIZE * ctxt->tmp_stride);
  const int is_hbd = is_cur_buf_hbd(xd);

  if (!is_hbd) {
    for (int row = 0; row < op_mi_size * MI_SIZE; ++row) {
      for (int col = 0; col < ctxt->overlap; ++col) {
        const uint8_t m0 = mask1d[col];
        const uint8_t m1 = AOM_BLEND_A64_MAX_ALPHA - m0;
        wsrc[col] = (wsrc[col] >> AOM_BLEND_A64_ROUND_BITS) * m0 +
                    (tmp[col] << AOM_BLEND_A64_ROUND_BITS) * m1;
        mask[col] = (mask[col] >> AOM_BLEND_A64_ROUND_BITS) * m0;
      }
      wsrc += bw;
      mask += bw;
      tmp += ctxt->tmp_stride;
    }
  } else {
    const uint16_t *tmp16 = CONVERT_TO_SHORTPTR(tmp);

    for (int row = 0; row < op_mi_size * MI_SIZE; ++row) {
      for (int col = 0; col < ctxt->overlap; ++col) {
        const uint8_t m0 = mask1d[col];
        const uint8_t m1 = AOM_BLEND_A64_MAX_ALPHA - m0;
        wsrc[col] = (wsrc[col] >> AOM_BLEND_A64_ROUND_BITS) * m0 +
                    (tmp16[col] << AOM_BLEND_A64_ROUND_BITS) * m1;
        mask[col] = (mask[col] >> AOM_BLEND_A64_ROUND_BITS) * m0;
      }
      wsrc += bw;
      mask += bw;
      tmp16 += ctxt->tmp_stride;
    }
  }
}

// This function has a structure similar to av1_build_obmc_inter_prediction
//
// The OBMC predictor is computed as:
//
//  PObmc(x,y) =
//    AOM_BLEND_A64(Mh(x),
//                  AOM_BLEND_A64(Mv(y), P(x,y), PAbove(x,y)),
//                  PLeft(x, y))
//
// Scaling up by AOM_BLEND_A64_MAX_ALPHA ** 2 and omitting the intermediate
// rounding, this can be written as:
//
//  AOM_BLEND_A64_MAX_ALPHA * AOM_BLEND_A64_MAX_ALPHA * Pobmc(x,y) =
//    Mh(x) * Mv(y) * P(x,y) +
//      Mh(x) * Cv(y) * Pabove(x,y) +
//      AOM_BLEND_A64_MAX_ALPHA * Ch(x) * PLeft(x, y)
//
// Where :
//
//  Cv(y) = AOM_BLEND_A64_MAX_ALPHA - Mv(y)
//  Ch(y) = AOM_BLEND_A64_MAX_ALPHA - Mh(y)
//
// This function computes 'wsrc' and 'mask' as:
//
//  wsrc(x, y) =
//    AOM_BLEND_A64_MAX_ALPHA * AOM_BLEND_A64_MAX_ALPHA * src(x, y) -
//      Mh(x) * Cv(y) * Pabove(x,y) +
//      AOM_BLEND_A64_MAX_ALPHA * Ch(x) * PLeft(x, y)
//
//  mask(x, y) = Mh(x) * Mv(y)
//
// These can then be used to efficiently approximate the error for any
// predictor P in the context of the provided neighbouring predictors by
// computing:
//
//  error(x, y) =
//    wsrc(x, y) - mask(x, y) * P(x, y) / (AOM_BLEND_A64_MAX_ALPHA ** 2)
//
static AOM_INLINE void calc_target_weighted_pred(
    const AV1_COMMON *cm, const MACROBLOCK *x, const MACROBLOCKD *xd,
    const uint8_t *above, int above_stride, const uint8_t *left,
    int left_stride) {
  const BLOCK_SIZE bsize = xd->mi[0]->sb_type;
  const int bw = xd->n4_w << MI_SIZE_LOG2;
  const int bh = xd->n4_h << MI_SIZE_LOG2;
  int32_t *mask_buf = x->mask_buf;
  int32_t *wsrc_buf = x->wsrc_buf;

  const int is_hbd = is_cur_buf_hbd(xd);
  const int src_scale = AOM_BLEND_A64_MAX_ALPHA * AOM_BLEND_A64_MAX_ALPHA;

  // plane 0 should not be subsampled
  assert(xd->plane[0].subsampling_x == 0);
  assert(xd->plane[0].subsampling_y == 0);

  av1_zero_array(wsrc_buf, bw * bh);
  for (int i = 0; i < bw * bh; ++i) mask_buf[i] = AOM_BLEND_A64_MAX_ALPHA;

  // handle above row
  if (xd->up_available) {
    const int overlap =
        AOMMIN(block_size_high[bsize], block_size_high[BLOCK_64X64]) >> 1;
    struct calc_target_weighted_pred_ctxt ctxt = { x, above, above_stride,
                                                   overlap };
    foreach_overlappable_nb_above(cm, (MACROBLOCKD *)xd,
                                  max_neighbor_obmc[mi_size_wide_log2[bsize]],
                                  calc_target_weighted_pred_above, &ctxt);
  }

  for (int i = 0; i < bw * bh; ++i) {
    wsrc_buf[i] *= AOM_BLEND_A64_MAX_ALPHA;
    mask_buf[i] *= AOM_BLEND_A64_MAX_ALPHA;
  }

  // handle left column
  if (xd->left_available) {
    const int overlap =
        AOMMIN(block_size_wide[bsize], block_size_wide[BLOCK_64X64]) >> 1;
    struct calc_target_weighted_pred_ctxt ctxt = { x, left, left_stride,
                                                   overlap };
    foreach_overlappable_nb_left(cm, (MACROBLOCKD *)xd,
                                 max_neighbor_obmc[mi_size_high_log2[bsize]],
                                 calc_target_weighted_pred_left, &ctxt);
  }

  if (!is_hbd) {
    const uint8_t *src = x->plane[0].src.buf;

    for (int row = 0; row < bh; ++row) {
      for (int col = 0; col < bw; ++col) {
        wsrc_buf[col] = src[col] * src_scale - wsrc_buf[col];
      }
      wsrc_buf += bw;
      src += x->plane[0].src.stride;
    }
  } else {
    const uint16_t *src = CONVERT_TO_SHORTPTR(x->plane[0].src.buf);

    for (int row = 0; row < bh; ++row) {
      for (int col = 0; col < bw; ++col) {
        wsrc_buf[col] = src[col] * src_scale - wsrc_buf[col];
      }
      wsrc_buf += bw;
      src += x->plane[0].src.stride;
    }
  }
}

/* Use standard 3x3 Sobel matrix. Macro so it can be used for either high or
   low bit-depth arrays. */
#define SOBEL_X(src, stride, i, j)                       \
  ((src)[((i)-1) + (stride) * ((j)-1)] -                 \
   (src)[((i) + 1) + (stride) * ((j)-1)] +  /* NOLINT */ \
   2 * (src)[((i)-1) + (stride) * (j)] -    /* NOLINT */ \
   2 * (src)[((i) + 1) + (stride) * (j)] +  /* NOLINT */ \
   (src)[((i)-1) + (stride) * ((j) + 1)] -  /* NOLINT */ \
   (src)[((i) + 1) + (stride) * ((j) + 1)]) /* NOLINT */
#define SOBEL_Y(src, stride, i, j)                       \
  ((src)[((i)-1) + (stride) * ((j)-1)] +                 \
   2 * (src)[(i) + (stride) * ((j)-1)] +    /* NOLINT */ \
   (src)[((i) + 1) + (stride) * ((j)-1)] -  /* NOLINT */ \
   (src)[((i)-1) + (stride) * ((j) + 1)] -  /* NOLINT */ \
   2 * (src)[(i) + (stride) * ((j) + 1)] -  /* NOLINT */ \
   (src)[((i) + 1) + (stride) * ((j) + 1)]) /* NOLINT */

sobel_xy av1_sobel(const uint8_t *input, int stride, int i, int j,
                   bool high_bd) {
  int16_t s_x;
  int16_t s_y;
  if (high_bd) {
    const uint16_t *src = CONVERT_TO_SHORTPTR(input);
    s_x = SOBEL_X(src, stride, i, j);
    s_y = SOBEL_Y(src, stride, i, j);
  } else {
    s_x = SOBEL_X(input, stride, i, j);
    s_y = SOBEL_Y(input, stride, i, j);
  }
  sobel_xy r = { .x = s_x, .y = s_y };
  return r;
}

// 8-tap Gaussian convolution filter with sigma = 1.3, sums to 128,
// all co-efficients must be even.
DECLARE_ALIGNED(16, static const int16_t, gauss_filter[8]) = { 2,  12, 30, 40,
                                                               30, 12, 2,  0 };

void av1_gaussian_blur(const uint8_t *src, int src_stride, int w, int h,
                       uint8_t *dst, bool high_bd, int bd) {
  ConvolveParams conv_params = get_conv_params(0, 0, bd);
  InterpFilterParams filter = { .filter_ptr = gauss_filter,
                                .taps = 8,
                                .subpel_shifts = 0,
                                .interp_filter = EIGHTTAP_REGULAR };
  // Requirements from the vector-optimized implementations.
  assert(h % 4 == 0);
  assert(w % 8 == 0);
  // Because we use an eight tap filter, the stride should be at least 7 + w.
  assert(src_stride >= w + 7);
#if CONFIG_AV1_HIGHBITDEPTH
  if (high_bd) {
    av1_highbd_convolve_2d_sr(CONVERT_TO_SHORTPTR(src), src_stride,
                              CONVERT_TO_SHORTPTR(dst), w, w, h, &filter,
                              &filter, 0, 0, &conv_params, bd);
  } else {
    av1_convolve_2d_sr(src, src_stride, dst, w, w, h, &filter, &filter, 0, 0,
                       &conv_params);
  }
#else
  (void)high_bd;
  av1_convolve_2d_sr(src, src_stride, dst, w, w, h, &filter, &filter, 0, 0,
                     &conv_params);
#endif
}

static EdgeInfo edge_probability(const uint8_t *input, int w, int h,
                                 bool high_bd, int bd) {
  // The probability of an edge in the whole image is the same as the highest
  // probability of an edge for any individual pixel. Use Sobel as the metric
  // for finding an edge.
  uint16_t highest = 0;
  uint16_t highest_x = 0;
  uint16_t highest_y = 0;
  // Ignore the 1 pixel border around the image for the computation.
  for (int j = 1; j < h - 1; ++j) {
    for (int i = 1; i < w - 1; ++i) {
      sobel_xy g = av1_sobel(input, w, i, j, high_bd);
      // Scale down to 8-bit to get same output regardless of bit depth.
      int16_t g_x = g.x >> (bd - 8);
      int16_t g_y = g.y >> (bd - 8);
      uint16_t magnitude = (uint16_t)sqrt(g_x * g_x + g_y * g_y);
      highest = AOMMAX(highest, magnitude);
      highest_x = AOMMAX(highest_x, g_x);
      highest_y = AOMMAX(highest_y, g_y);
    }
  }
  EdgeInfo ei = { .magnitude = highest, .x = highest_x, .y = highest_y };
  return ei;
}

/* Uses most of the Canny edge detection algorithm to find if there are any
 * edges in the image.
 */
EdgeInfo av1_edge_exists(const uint8_t *src, int src_stride, int w, int h,
                         bool high_bd, int bd) {
  if (w < 3 || h < 3) {
    EdgeInfo n = { .magnitude = 0, .x = 0, .y = 0 };
    return n;
  }
  uint8_t *blurred;
  if (high_bd) {
    blurred = CONVERT_TO_BYTEPTR(aom_memalign(32, sizeof(uint16_t) * w * h));
  } else {
    blurred = (uint8_t *)aom_memalign(32, sizeof(uint8_t) * w * h);
  }
  av1_gaussian_blur(src, src_stride, w, h, blurred, high_bd, bd);
  // Skip the non-maximum suppression step in Canny edge detection. We just
  // want a probability of an edge existing in the buffer, which is determined
  // by the strongest edge in it -- we don't need to eliminate the weaker
  // edges. Use Sobel for the edge detection.
  EdgeInfo prob = edge_probability(blurred, w, h, high_bd, bd);
  if (high_bd) {
    aom_free(CONVERT_TO_SHORTPTR(blurred));
  } else {
    aom_free(blurred);
  }
  return prob;
}
