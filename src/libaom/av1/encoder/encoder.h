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

#ifndef AOM_AV1_ENCODER_ENCODER_H_
#define AOM_AV1_ENCODER_ENCODER_H_

#include <stdio.h>

#include "config/aom_config.h"

#include "aom/aomcx.h"

#include "av1/common/alloccommon.h"
#include "av1/common/entropymode.h"
#include "av1/common/thread_common.h"
#include "av1/common/onyxc_int.h"
#include "av1/common/resize.h"
#include "av1/common/timing.h"
#include "av1/common/blockd.h"
#include "av1/common/enums.h"
#include "av1/encoder/aq_cyclicrefresh.h"
#include "av1/encoder/av1_quantize.h"
#include "av1/encoder/context_tree.h"
#include "av1/encoder/encodemb.h"
#include "av1/encoder/firstpass.h"
#include "av1/encoder/lookahead.h"
#include "av1/encoder/mbgraph.h"
#include "av1/encoder/mcomp.h"
#include "av1/encoder/ratectrl.h"
#include "av1/encoder/rd.h"
#include "av1/encoder/speed_features.h"
#include "av1/encoder/tokenize.h"
#include "av1/encoder/block.h"

#if CONFIG_INTERNAL_STATS
#include "aom_dsp/ssim.h"
#endif
#include "aom_dsp/variance.h"
#if CONFIG_DENOISE
#include "aom_dsp/noise_model.h"
#endif
#include "aom/internal/aom_codec_internal.h"
#include "aom_util/aom_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int nmv_vec_cost[MV_JOINTS];
  int nmv_costs[2][MV_VALS];
  int nmv_costs_hp[2][MV_VALS];

  FRAME_CONTEXT fc;
} CODING_CONTEXT;

enum {
  // regular inter frame
  REGULAR_FRAME = 0,
  // alternate reference frame
  ARF_FRAME = 1,
  // overlay frame
  OVERLAY_FRAME = 2,
  // golden frame
  GLD_FRAME = 3,
  // backward reference frame
  BRF_FRAME = 4,
  // extra alternate reference frame
  EXT_ARF_FRAME = 5,
  FRAME_CONTEXT_INDEXES
} UENUM1BYTE(FRAME_CONTEXT_INDEX);

enum {
  NORMAL = 0,
  FOURFIVE = 1,
  THREEFIVE = 2,
  ONETWO = 3
} UENUM1BYTE(AOM_SCALING);

enum {
  // Good Quality Fast Encoding. The encoder balances quality with the amount of
  // time it takes to encode the output. Speed setting controls how fast.
  GOOD,
  // Realtime Fast Encoding. Will force some restrictions on bitrate
  // constraints.
  REALTIME
} UENUM1BYTE(MODE);

enum {
  FRAMEFLAGS_KEY = 1 << 0,
  FRAMEFLAGS_GOLDEN = 1 << 1,
  FRAMEFLAGS_BWDREF = 1 << 2,
  // TODO(zoeliu): To determine whether a frame flag is needed for ALTREF2_FRAME
  FRAMEFLAGS_ALTREF = 1 << 3,
  FRAMEFLAGS_INTRAONLY = 1 << 4,
  FRAMEFLAGS_SWITCH = 1 << 5,
  FRAMEFLAGS_ERROR_RESILIENT = 1 << 6,
} UENUM1BYTE(FRAMETYPE_FLAGS);

enum {
  NO_AQ = 0,
  VARIANCE_AQ = 1,
  COMPLEXITY_AQ = 2,
  CYCLIC_REFRESH_AQ = 3,
  AQ_MODE_COUNT  // This should always be the last member of the enum
} UENUM1BYTE(AQ_MODE);
enum {
  NO_DELTA_Q = 0,
  DELTA_Q_ONLY = 1,
  DELTA_Q_LF = 2,
  DELTAQ_MODE_COUNT  // This should always be the last member of the enum
} UENUM1BYTE(DELTAQ_MODE);

enum {
  RESIZE_NONE = 0,    // No frame resizing allowed.
  RESIZE_FIXED = 1,   // All frames are coded at the specified scale.
  RESIZE_RANDOM = 2,  // All frames are coded at a random scale.
  RESIZE_MODES
} UENUM1BYTE(RESIZE_MODE);

enum {
  SUPERRES_NONE,     // No frame superres allowed.
  SUPERRES_FIXED,    // All frames are coded at the specified scale,
                     // and super-resolved.
  SUPERRES_RANDOM,   // All frames are coded at a random scale,
                     // and super-resolved.
  SUPERRES_QTHRESH,  // Superres scale for a frame is determined based on
                     // q_index.
  SUPERRES_AUTO,     // Automatically select superres for appropriate frames.
  SUPERRES_MODES
} UENUM1BYTE(SUPERRES_MODE);

typedef enum {
  kInvalid = 0,
  kLowSadLowSumdiff = 1,
  kLowSadHighSumdiff = 2,
  kHighSadLowSumdiff = 3,
  kHighSadHighSumdiff = 4,
  kLowVarHighSumdiff = 5,
  kVeryHighSad = 6,
} CONTENT_STATE_SB;

typedef struct TplDepStats {
  int64_t intra_cost;
  int64_t inter_cost;
  int64_t mc_flow;
  int64_t mc_dep_cost;
  int64_t mc_ref_cost;

  int ref_frame_index;
  int_mv mv;
} TplDepStats;

typedef struct TplDepFrame {
  uint8_t is_valid;
  TplDepStats *tpl_stats_ptr;
  int stride;
  int width;
  int height;
  int mi_rows;
  int mi_cols;
  int base_qindex;
} TplDepFrame;

typedef enum {
  COST_UPD_SB,
  COST_UPD_SBROW,
  COST_UPD_TILE,
} COST_UPDATE_TYPE;

#define TPL_DEP_COST_SCALE_LOG2 4

typedef struct AV1EncoderConfig {
  BITSTREAM_PROFILE profile;
  aom_bit_depth_t bit_depth;     // Codec bit-depth.
  int width;                     // width of data passed to the compressor
  int height;                    // height of data passed to the compressor
  int forced_max_frame_width;    // forced maximum width of frame (if != 0)
  int forced_max_frame_height;   // forced maximum height of frame (if != 0)
  unsigned int input_bit_depth;  // Input bit depth.
  double init_framerate;         // set to passed in framerate
  int64_t target_bandwidth;      // bandwidth to be used in bits per second

  int noise_sensitivity;  // pre processing blur: recommendation 0
  int sharpness;          // sharpening output: recommendation 0:
  int speed;
  // maximum allowed bitrate for any intra frame in % of bitrate target.
  unsigned int rc_max_intra_bitrate_pct;
  // maximum allowed bitrate for any inter frame in % of bitrate target.
  unsigned int rc_max_inter_bitrate_pct;
  // percent of rate boost for golden frame in CBR mode.
  unsigned int gf_cbr_boost_pct;

  MODE mode;
  int pass;

  // Key Framing Operations
  int auto_key;  // autodetect cut scenes and set the keyframes
  int key_freq;  // maximum distance to key frame.
  int sframe_dist;
  int sframe_mode;
  int sframe_enabled;
  int lag_in_frames;  // how many frames lag before we start encoding
  int fwd_kf_enabled;

  // ----------------------------------------------------------------
  // DATARATE CONTROL OPTIONS

  // vbr, cbr, constrained quality or constant quality
  enum aom_rc_mode rc_mode;

  // buffer targeting aggressiveness
  int under_shoot_pct;
  int over_shoot_pct;

  // buffering parameters
  int64_t starting_buffer_level_ms;
  int64_t optimal_buffer_level_ms;
  int64_t maximum_buffer_size_ms;

  // Frame drop threshold.
  int drop_frames_water_mark;

  // controlling quality
  int fixed_q;
  int worst_allowed_q;
  int best_allowed_q;
  int cq_level;
  AQ_MODE aq_mode;  // Adaptive Quantization mode
  DELTAQ_MODE deltaq_mode;
  int enable_cdef;
  int enable_restoration;
  int enable_obmc;
  int disable_trellis_quant;
  int using_qm;
  int qm_y;
  int qm_u;
  int qm_v;
  int qm_minlevel;
  int qm_maxlevel;
#if CONFIG_DIST_8X8
  int using_dist_8x8;
#endif
  unsigned int num_tile_groups;
  unsigned int mtu;

  // Internal frame size scaling.
  RESIZE_MODE resize_mode;
  uint8_t resize_scale_denominator;
  uint8_t resize_kf_scale_denominator;

  // Frame Super-Resolution size scaling.
  SUPERRES_MODE superres_mode;
  uint8_t superres_scale_denominator;
  uint8_t superres_kf_scale_denominator;
  int superres_qthresh;
  int superres_kf_qthresh;

  // Enable feature to reduce the frame quantization every x frames.
  int frame_periodic_boost;

  // two pass datarate control
  int two_pass_vbrbias;  // two pass datarate control tweaks
  int two_pass_vbrmin_section;
  int two_pass_vbrmax_section;
  // END DATARATE CONTROL OPTIONS
  // ----------------------------------------------------------------

  int enable_auto_arf;
  int enable_auto_brf;  // (b)ackward (r)ef (f)rame

  /* Bitfield defining the error resiliency features to enable.
   * Can provide decodable frames after losses in previous
   * frames and decodable partitions after losses in the same frame.
   */
  unsigned int error_resilient_mode;

  unsigned int s_frame_mode;

  /* Bitfield defining the parallel decoding mode where the
   * decoding in successive frames may be conducted in parallel
   * just by decoding the frame headers.
   */
  unsigned int frame_parallel_decoding_mode;

  unsigned int limit;

  int arnr_max_frames;
  int arnr_strength;

  int min_gf_interval;
  int max_gf_interval;
  int gf_max_pyr_height;

  int row_mt;
  int tile_columns;
  int tile_rows;
  int tile_width_count;
  int tile_height_count;
  int tile_widths[MAX_TILE_COLS];
  int tile_heights[MAX_TILE_ROWS];

  int enable_tpl_model;

  int max_threads;

  aom_fixed_buf_t two_pass_stats_in;

#if CONFIG_FP_MB_STATS
  aom_fixed_buf_t firstpass_mb_stats_in;
#endif

  aom_tune_metric tuning;
  aom_tune_content content;
  int use_highbitdepth;
  aom_color_primaries_t color_primaries;
  aom_transfer_characteristics_t transfer_characteristics;
  aom_matrix_coefficients_t matrix_coefficients;
  aom_chroma_sample_position_t chroma_sample_position;
  int color_range;
  int render_width;
  int render_height;
  int timing_info_present;
  aom_timing_info_t timing_info;
  int decoder_model_info_present_flag;
  int display_model_info_present_flag;
  int buffer_removal_time_present;
  aom_dec_model_info_t buffer_model;
  int film_grain_test_vector;
  const char *film_grain_table_filename;

  uint8_t cdf_update_mode;
  aom_superblock_size_t superblock_size;
  unsigned int large_scale_tile;
  unsigned int single_tile_decoding;
  uint8_t monochrome;
  unsigned int full_still_picture_hdr;
  int enable_dual_filter;
  unsigned int motion_vector_unit_test;
  const cfg_options_t *cfg;
  int enable_rect_partitions;
  int enable_intra_edge_filter;
  int enable_tx64;
  int enable_order_hint;
  int enable_dist_wtd_comp;
  int enable_ref_frame_mvs;
  unsigned int max_reference_frames;
  unsigned int allow_ref_frame_mvs;
  int enable_masked_comp;
  int enable_interintra_comp;
  int enable_smooth_interintra;
  int enable_diff_wtd_comp;
  int enable_interinter_wedge;
  int enable_interintra_wedge;
  int enable_global_motion;
  int enable_warped_motion;
  int allow_warped_motion;
  int enable_filter_intra;
  int enable_smooth_intra;
  int enable_paeth_intra;
  int enable_cfl_intra;
  int enable_superres;
  int enable_palette;
  int enable_intrabc;
  int enable_angle_delta;
  unsigned int save_as_annexb;

#if CONFIG_DENOISE
  float noise_level;
  int noise_block_size;
#endif

  unsigned int chroma_subsampling_x;
  unsigned int chroma_subsampling_y;
  int reduced_tx_type_set;
  int use_intra_dct_only;
  int use_inter_dct_only;
  int use_intra_default_tx_only;
  int quant_b_adapt;
  COST_UPDATE_TYPE coeff_cost_upd_freq;
  COST_UPDATE_TYPE mode_cost_upd_freq;
  int border_in_pixels;
} AV1EncoderConfig;

static INLINE int is_lossless_requested(const AV1EncoderConfig *cfg) {
  return cfg->best_allowed_q == 0 && cfg->worst_allowed_q == 0;
}

typedef struct FRAME_COUNTS {
// Note: This structure should only contain 'unsigned int' fields, or
// aggregates built solely from 'unsigned int' fields/elements
#if CONFIG_ENTROPY_STATS
  unsigned int kf_y_mode[KF_MODE_CONTEXTS][KF_MODE_CONTEXTS][INTRA_MODES];
  unsigned int angle_delta[DIRECTIONAL_MODES][2 * MAX_ANGLE_DELTA + 1];
  unsigned int y_mode[BLOCK_SIZE_GROUPS][INTRA_MODES];
  unsigned int uv_mode[CFL_ALLOWED_TYPES][INTRA_MODES][UV_INTRA_MODES];
  unsigned int cfl_sign[CFL_JOINT_SIGNS];
  unsigned int cfl_alpha[CFL_ALPHA_CONTEXTS][CFL_ALPHABET_SIZE];
  unsigned int palette_y_mode[PALATTE_BSIZE_CTXS][PALETTE_Y_MODE_CONTEXTS][2];
  unsigned int palette_uv_mode[PALETTE_UV_MODE_CONTEXTS][2];
  unsigned int palette_y_size[PALATTE_BSIZE_CTXS][PALETTE_SIZES];
  unsigned int palette_uv_size[PALATTE_BSIZE_CTXS][PALETTE_SIZES];
  unsigned int palette_y_color_index[PALETTE_SIZES]
                                    [PALETTE_COLOR_INDEX_CONTEXTS]
                                    [PALETTE_COLORS];
  unsigned int palette_uv_color_index[PALETTE_SIZES]
                                     [PALETTE_COLOR_INDEX_CONTEXTS]
                                     [PALETTE_COLORS];
  unsigned int partition[PARTITION_CONTEXTS][EXT_PARTITION_TYPES];
  unsigned int txb_skip[TOKEN_CDF_Q_CTXS][TX_SIZES][TXB_SKIP_CONTEXTS][2];
  unsigned int eob_extra[TOKEN_CDF_Q_CTXS][TX_SIZES][PLANE_TYPES]
                        [EOB_COEF_CONTEXTS][2];
  unsigned int dc_sign[PLANE_TYPES][DC_SIGN_CONTEXTS][2];
  unsigned int coeff_lps[TX_SIZES][PLANE_TYPES][BR_CDF_SIZE - 1][LEVEL_CONTEXTS]
                        [2];
  unsigned int eob_flag[TX_SIZES][PLANE_TYPES][EOB_COEF_CONTEXTS][2];
  unsigned int eob_multi16[TOKEN_CDF_Q_CTXS][PLANE_TYPES][2][5];
  unsigned int eob_multi32[TOKEN_CDF_Q_CTXS][PLANE_TYPES][2][6];
  unsigned int eob_multi64[TOKEN_CDF_Q_CTXS][PLANE_TYPES][2][7];
  unsigned int eob_multi128[TOKEN_CDF_Q_CTXS][PLANE_TYPES][2][8];
  unsigned int eob_multi256[TOKEN_CDF_Q_CTXS][PLANE_TYPES][2][9];
  unsigned int eob_multi512[TOKEN_CDF_Q_CTXS][PLANE_TYPES][2][10];
  unsigned int eob_multi1024[TOKEN_CDF_Q_CTXS][PLANE_TYPES][2][11];
  unsigned int coeff_lps_multi[TOKEN_CDF_Q_CTXS][TX_SIZES][PLANE_TYPES]
                              [LEVEL_CONTEXTS][BR_CDF_SIZE];
  unsigned int coeff_base_multi[TOKEN_CDF_Q_CTXS][TX_SIZES][PLANE_TYPES]
                               [SIG_COEF_CONTEXTS][NUM_BASE_LEVELS + 2];
  unsigned int coeff_base_eob_multi[TOKEN_CDF_Q_CTXS][TX_SIZES][PLANE_TYPES]
                                   [SIG_COEF_CONTEXTS_EOB][NUM_BASE_LEVELS + 1];
  unsigned int newmv_mode[NEWMV_MODE_CONTEXTS][2];
  unsigned int zeromv_mode[GLOBALMV_MODE_CONTEXTS][2];
  unsigned int refmv_mode[REFMV_MODE_CONTEXTS][2];
  unsigned int drl_mode[DRL_MODE_CONTEXTS][2];
  unsigned int inter_compound_mode[INTER_MODE_CONTEXTS][INTER_COMPOUND_MODES];
  unsigned int wedge_idx[BLOCK_SIZES_ALL][16];
  unsigned int interintra[BLOCK_SIZE_GROUPS][2];
  unsigned int interintra_mode[BLOCK_SIZE_GROUPS][INTERINTRA_MODES];
  unsigned int wedge_interintra[BLOCK_SIZES_ALL][2];
  unsigned int compound_type[BLOCK_SIZES_ALL][MASKED_COMPOUND_TYPES];
  unsigned int motion_mode[BLOCK_SIZES_ALL][MOTION_MODES];
  unsigned int obmc[BLOCK_SIZES_ALL][2];
  unsigned int intra_inter[INTRA_INTER_CONTEXTS][2];
  unsigned int comp_inter[COMP_INTER_CONTEXTS][2];
  unsigned int comp_ref_type[COMP_REF_TYPE_CONTEXTS][2];
  unsigned int uni_comp_ref[UNI_COMP_REF_CONTEXTS][UNIDIR_COMP_REFS - 1][2];
  unsigned int single_ref[REF_CONTEXTS][SINGLE_REFS - 1][2];
  unsigned int comp_ref[REF_CONTEXTS][FWD_REFS - 1][2];
  unsigned int comp_bwdref[REF_CONTEXTS][BWD_REFS - 1][2];
  unsigned int intrabc[2];

  unsigned int txfm_partition[TXFM_PARTITION_CONTEXTS][2];
  unsigned int intra_tx_size[MAX_TX_CATS][TX_SIZE_CONTEXTS][MAX_TX_DEPTH + 1];
  unsigned int skip_mode[SKIP_MODE_CONTEXTS][2];
  unsigned int skip[SKIP_CONTEXTS][2];
  unsigned int compound_index[COMP_INDEX_CONTEXTS][2];
  unsigned int comp_group_idx[COMP_GROUP_IDX_CONTEXTS][2];
  unsigned int delta_q[DELTA_Q_PROBS][2];
  unsigned int delta_lf_multi[FRAME_LF_COUNT][DELTA_LF_PROBS][2];
  unsigned int delta_lf[DELTA_LF_PROBS][2];

  unsigned int inter_ext_tx[EXT_TX_SETS_INTER][EXT_TX_SIZES][TX_TYPES];
  unsigned int intra_ext_tx[EXT_TX_SETS_INTRA][EXT_TX_SIZES][INTRA_MODES]
                           [TX_TYPES];
  unsigned int filter_intra_mode[FILTER_INTRA_MODES];
  unsigned int filter_intra[BLOCK_SIZES_ALL][2];
  unsigned int switchable_restore[RESTORE_SWITCHABLE_TYPES];
  unsigned int wiener_restore[2];
  unsigned int sgrproj_restore[2];
#endif  // CONFIG_ENTROPY_STATS

  unsigned int switchable_interp[SWITCHABLE_FILTER_CONTEXTS]
                                [SWITCHABLE_FILTERS];
} FRAME_COUNTS;

#define INTER_MODE_RD_DATA_OVERALL_SIZE 6400

typedef struct {
  int ready;
  double a;
  double b;
  double dist_mean;
  double ld_mean;
  double sse_mean;
  double sse_sse_mean;
  double sse_ld_mean;
  int num;
  double dist_sum;
  double ld_sum;
  double sse_sum;
  double sse_sse_sum;
  double sse_ld_sum;
} InterModeRdModel;

typedef struct {
  int idx;
  int64_t rd;
} RdIdxPair;
// TODO(angiebird): This is an estimated size. We still need to figure what is
// the maximum number of modes.
#define MAX_INTER_MODES 1024
typedef struct inter_modes_info {
  int num;
  MB_MODE_INFO mbmi_arr[MAX_INTER_MODES];
  int mode_rate_arr[MAX_INTER_MODES];
  int64_t sse_arr[MAX_INTER_MODES];
  int64_t est_rd_arr[MAX_INTER_MODES];
  RdIdxPair rd_idx_pair_arr[MAX_INTER_MODES];
} InterModesInfo;

// Encoder row synchronization
typedef struct AV1RowMTSyncData {
#if CONFIG_MULTITHREAD
  pthread_mutex_t *mutex_;
  pthread_cond_t *cond_;
#endif
  // Allocate memory to store the sb/mb block index in each row.
  int *cur_col;
  int sync_range;
  int rows;
} AV1RowMTSync;

typedef struct AV1RowMTInfo {
  int current_mi_row;
  int num_threads_working;
} AV1RowMTInfo;

// TODO(jingning) All spatially adaptive variables should go to TileDataEnc.
typedef struct TileDataEnc {
  TileInfo tile_info;
  int thresh_freq_fact[BLOCK_SIZES_ALL][MAX_MODES];
  int m_search_count;
  int ex_search_count;
  CFL_CTX cfl;
  DECLARE_ALIGNED(16, FRAME_CONTEXT, tctx);
  FRAME_CONTEXT *row_ctx;
  uint8_t allow_update_cdf;
  InterModeRdModel inter_mode_rd_models[BLOCK_SIZES_ALL];
  AV1RowMTSync row_mt_sync;
  AV1RowMTInfo row_mt_info;
} TileDataEnc;

typedef struct {
  TOKENEXTRA *start;
  TOKENEXTRA *stop;
  unsigned int count;
} TOKENLIST;

typedef struct MultiThreadHandle {
  int allocated_tile_rows;
  int allocated_tile_cols;
  int allocated_sb_rows;
  int thread_id_to_tile_id[MAX_NUM_THREADS];  // Mapping of threads to tiles
} MultiThreadHandle;

typedef struct RD_COUNTS {
  int64_t comp_pred_diff[REFERENCE_MODES];
  // Stores number of 4x4 blocks using global motion per reference frame.
  int global_motion_used[REF_FRAMES];
  int compound_ref_used_flag;
  int skip_mode_used_flag;
} RD_COUNTS;

typedef struct ThreadData {
  MACROBLOCK mb;
  RD_COUNTS rd_counts;
  FRAME_COUNTS *counts;
  PC_TREE *pc_tree;
  PC_TREE *pc_root[MAX_MIB_SIZE_LOG2 - MIN_MIB_SIZE_LOG2 + 1];
  tran_low_t *tree_coeff_buf[MAX_MB_PLANE];
  tran_low_t *tree_qcoeff_buf[MAX_MB_PLANE];
  tran_low_t *tree_dqcoeff_buf[MAX_MB_PLANE];
  InterModesInfo *inter_modes_info;
  uint32_t *hash_value_buffer[2][2];
  int32_t *wsrc_buf;
  int32_t *mask_buf;
  uint8_t *above_pred_buf;
  uint8_t *left_pred_buf;
  PALETTE_BUFFER *palette_buffer;
  CONV_BUF_TYPE *tmp_conv_dst;
  uint8_t *tmp_obmc_bufs[2];
  int intrabc_used;
  FRAME_CONTEXT *tctx;
} ThreadData;

struct EncWorkerData;

typedef struct ActiveMap {
  int enabled;
  int update;
  unsigned char *map;
} ActiveMap;

#if CONFIG_INTERNAL_STATS
// types of stats
enum {
  STAT_Y,
  STAT_U,
  STAT_V,
  STAT_ALL,
  NUM_STAT_TYPES  // This should always be the last member of the enum
} UENUM1BYTE(StatType);

typedef struct IMAGE_STAT {
  double stat[NUM_STAT_TYPES];
  double worst;
} ImageStat;
#endif  // CONFIG_INTERNAL_STATS

typedef struct {
  int ref_count;
  YV12_BUFFER_CONFIG buf;
} EncRefCntBuffer;

#if CONFIG_COLLECT_PARTITION_STATS
typedef struct PartitionStats {
  int partition_decisions[6][EXT_PARTITION_TYPES];
  int partition_attempts[6][EXT_PARTITION_TYPES];

  int partition_redo;
} PartitionStats;
#endif

typedef struct AV1_COMP {
  QUANTS quants;
  ThreadData td;
  FRAME_COUNTS counts;
  MB_MODE_INFO_EXT *mbmi_ext_base;
  CB_COEFF_BUFFER *coeff_buffer_base;
  Dequants dequants;
  AV1_COMMON common;
  AV1EncoderConfig oxcf;
  struct lookahead_ctx *lookahead;
  struct lookahead_entry *alt_ref_source;
  int no_show_kf;

  int optimize_seg_arr[MAX_SEGMENTS];

  YV12_BUFFER_CONFIG *source;
  YV12_BUFFER_CONFIG *last_source;  // NULL for first frame and alt_ref frames
  YV12_BUFFER_CONFIG *unscaled_source;
  YV12_BUFFER_CONFIG scaled_source;
  YV12_BUFFER_CONFIG *unscaled_last_source;
  YV12_BUFFER_CONFIG scaled_last_source;

  TplDepFrame tpl_stats[MAX_LAG_BUFFERS];
  YV12_BUFFER_CONFIG *tpl_recon_frames[INTER_REFS_PER_FRAME + 1];

  // For a still frame, this flag is set to 1 to skip partition search.
  int partition_search_skippable_frame;
  double csm_rate_array[32];
  double m_rate_array[32];
  int rate_size;
  int rate_index;
  hash_table *previous_hash_table;
  int previous_index;

  unsigned int row_mt;
  RefCntBuffer *scaled_ref_buf[INTER_REFS_PER_FRAME];

  RefCntBuffer *last_show_frame_buf;  // last show frame buffer

  // refresh_*_frame are boolean flags. If 'refresh_xyz_frame' is true, then
  // after the current frame is encoded, the XYZ reference frame gets refreshed
  // (updated) to be the current frame.
  //
  // Special case: 'refresh_last_frame' specifies that:
  // - LAST_FRAME reference should be updated to be the current frame (as usual)
  // - Also, LAST2_FRAME and LAST3_FRAME references are implicitly updated to be
  // the two past reference frames just before LAST_FRAME that are available.
  //
  // Note: Usually at most one of these refresh flags is true at a time.
  // But a key-frame is special, for which all the flags are true at once.
  int refresh_last_frame;
  int refresh_golden_frame;
  int refresh_bwd_ref_frame;
  int refresh_alt2_ref_frame;
  int refresh_alt_ref_frame;

  // For each type of reference frame, this contains the index of a reference
  // frame buffer for a reference frame of the same type.  We use this to
  // choose our primary reference frame (which is the most recent reference
  // frame of the same type as the current frame).
  int fb_of_context_type[REF_FRAMES];

  // When true, a new rule for backward (future) reference frames is in effect:
  // - BWDREF_FRAME is always the closest future frame available
  // - ALTREF2_FRAME is always the 2nd closest future frame available
  // - 'refresh_bwd_ref_frame' flag is used for updating both the BWDREF_FRAME
  // and ALTREF2_FRAME. ('refresh_alt2_ref_frame' flag is irrelevant).
  int new_bwdref_update_rule;

  int ext_refresh_frame_flags_pending;
  int ext_refresh_last_frame;
  int ext_refresh_golden_frame;
  int ext_refresh_bwd_ref_frame;
  int ext_refresh_alt2_ref_frame;
  int ext_refresh_alt_ref_frame;

  int ext_refresh_frame_context_pending;
  int ext_refresh_frame_context;
  int ext_use_ref_frame_mvs;
  int ext_use_error_resilient;
  int ext_use_s_frame;
  int ext_use_primary_ref_none;

  YV12_BUFFER_CONFIG last_frame_uf;
  YV12_BUFFER_CONFIG trial_frame_rst;

  // Ambient reconstruction err target for force key frames
  int64_t ambient_err;

  RD_OPT rd;

  CODING_CONTEXT coding_context;

  int gmtype_cost[TRANS_TYPES];
  int gmparams_cost[REF_FRAMES];

  int nmv_costs[2][MV_VALS];
  int nmv_costs_hp[2][MV_VALS];

  int64_t last_time_stamp_seen;
  int64_t last_end_time_stamp_seen;
  int64_t first_time_stamp_ever;

  RATE_CONTROL rc;
  double framerate;

  // Relevant for an inter frame.
  // - Index '0' corresponds to the values for the currently coded frame.
  // - Indices LAST_FRAME ... EXTREF_FRAMES are used to store values for all the
  // possible inter reference frames.
  int interp_filter_selected[REF_FRAMES + 1][SWITCHABLE];

  struct aom_codec_pkt_list *output_pkt_list;

  MBGRAPH_FRAME_STATS mbgraph_stats[MAX_LAG_BUFFERS];
  int mbgraph_n_frames;  // number of frames filled in the above
  int static_mb_pct;     // % forced skip mbs by segmentation
  int ref_frame_flags;
  int ext_ref_frame_flags;

  // speed is passed as a per-frame parameter into the encoder
  int speed;
  // sf contains fine-grained config set internally based on speed
  SPEED_FEATURES sf;

  unsigned int max_mv_magnitude;
  int mv_step_param;

  int all_one_sided_refs;

  uint8_t *segmentation_map;

  CYCLIC_REFRESH *cyclic_refresh;
  ActiveMap active_map;

  fractional_mv_step_fp *find_fractional_mv_step;
  av1_diamond_search_fn_t diamond_search_sad;
  aom_variance_fn_ptr_t fn_ptr[BLOCK_SIZES_ALL];
  uint64_t time_receive_data;
  uint64_t time_compress_data;
  uint64_t time_pick_lpf;

#if CONFIG_FP_MB_STATS
  int use_fp_mb_stats;
#endif

  TWO_PASS twopass;

  YV12_BUFFER_CONFIG alt_ref_buffer;

#if CONFIG_INTERNAL_STATS
  unsigned int mode_chosen_counts[MAX_MODES];

  int count;
  uint64_t total_sq_error;
  uint64_t total_samples;
  ImageStat psnr;

  double total_blockiness;
  double worst_blockiness;

  int bytes;
  double summed_quality;
  double summed_weights;
  unsigned int tot_recode_hits;
  double worst_ssim;

  ImageStat fastssim;
  ImageStat psnrhvs;

  int b_calculate_blockiness;
  int b_calculate_consistency;

  double total_inconsistency;
  double worst_consistency;
  Ssimv *ssim_vars;
  Metrics metrics;
#endif
  int b_calculate_psnr;
#if CONFIG_SPEED_STATS
  unsigned int tx_search_count;
#endif  // CONFIG_SPEED_STATS

  int droppable;

  int initial_width;
  int initial_height;
  int initial_mbs;  // Number of MBs in the full-size frame; to be used to
                    // normalize the firstpass stats. This will differ from the
                    // number of MBs in the current frame when the frame is
                    // scaled.

  // When resize is triggered through external control, the desired width/height
  // are stored here until use in the next frame coded. They are effective only
  // for
  // one frame and are reset after use.
  int resize_pending_width;
  int resize_pending_height;

  int frame_flags;

  search_site_config ss_cfg;

  TileDataEnc *tile_data;
  int allocated_tiles;  // Keep track of memory allocated for tiles.

  TOKENEXTRA *tile_tok[MAX_TILE_ROWS][MAX_TILE_COLS];
  TOKENLIST *tplist[MAX_TILE_ROWS][MAX_TILE_COLS];

  int resize_state;
  int resize_avg_qp;
  int resize_buffer_underflow;

  // Sequence parameters have been transmitted already and locked
  // or not. Once locked av1_change_config cannot change the seq
  // parameters.
  int seq_params_locked;

  // VARIANCE_AQ segment map refresh
  int vaq_refresh;

  // VAR_BASED_PARTITION thresholds
  // 0 - threshold_128x128; 1 - threshold_64x64;
  // 2 - threshold_32x32; 3 - threshold_16x16;
  // 4 - vbp_threshold_8x8;
  int64_t vbp_thresholds[5];
  int64_t vbp_threshold_minmax;
  int64_t vbp_threshold_sad;
  int64_t vbp_threshold_copy;
  BLOCK_SIZE vbp_bsize_min;

  // Multi-threading
  int num_workers;
  AVxWorker *workers;
  struct EncWorkerData *tile_thr_data;
  int existing_fb_idx_to_show;
  int is_arf_filter_off[MAX_EXT_ARFS + 1];
  int num_extra_arfs;
  int arf_pos_in_gf[MAX_EXT_ARFS + 1];
  int arf_pos_for_ovrly[MAX_EXT_ARFS + 1];
  int global_motion_search_done;
  int extra_arf_allowed;
  // A flag to indicate if intrabc is ever used in current frame.
  int intrabc_used;
  int dv_cost[2][MV_VALS];
  // TODO(huisu@google.com): we can update dv_joint_cost per SB.
  int dv_joint_cost[MV_JOINTS];
  int has_lossless_segment;

  // Factors to control gating of compound type selection based on best
  // approximate rd so far
  int max_comp_type_rd_threshold_mul;
  int max_comp_type_rd_threshold_div;

  unsigned int tx_domain_dist_threshold;

  // Factor to control R-D optimization of coeffs based on block
  // mse.
  unsigned int coeff_opt_dist_threshold;

  AV1LfSync lf_row_sync;
  AV1LrSync lr_row_sync;
  AV1LrStruct lr_ctxt;

  aom_film_grain_table_t *film_grain_table;
#if CONFIG_DENOISE
  struct aom_denoise_and_model_t *denoise_and_model;
#endif
  // Stores the default value of skip flag depending on chroma format
  // Set as 1 for monochrome and 3 for other color formats
  int default_interp_skip_flags;
  int preserve_arf_as_gld;
  MultiThreadHandle multi_thread_ctxt;
  void (*row_mt_sync_read_ptr)(AV1RowMTSync *const, int, int);
  void (*row_mt_sync_write_ptr)(AV1RowMTSync *const, int, int, const int);
#if CONFIG_MULTITHREAD
  pthread_mutex_t *row_mt_mutex_;
#endif
  // Set if screen content is set or relevant tools are enabled
  int is_screen_content_type;
#if CONFIG_COLLECT_PARTITION_STATS
  PartitionStats partition_stats;
#endif
} AV1_COMP;

typedef struct {
  YV12_BUFFER_CONFIG *source;
  YV12_BUFFER_CONFIG *last_source;
  int64_t ts_duration;
} EncodeFrameInput;

// EncodeFrameParams contains per-frame encoding parameters decided upon by
// av1_encode_strategy() and passed down to av1_encode()
struct EncodeFrameParams {
  int error_resilient_mode;
  FRAME_TYPE frame_type;
  int primary_ref_frame;
  int order_offset;
  int show_frame;

  // This is a bitmask of which reference slots can be used in this frame
  int ref_frame_flags;

  // Speed level to use for this frame: Bigger number means faster.
  int speed;

  unsigned int *frame_flags;
};
typedef struct EncodeFrameParams EncodeFrameParams;

// EncodeFrameResults contains information about the result of encoding a
// single frame
typedef struct {
  size_t size;  // Size of resulting bitstream
} EncodeFrameResults;

// Must not be called more than once.
void av1_initialize_enc(void);

struct AV1_COMP *av1_create_compressor(AV1EncoderConfig *oxcf,
                                       BufferPool *const pool);
void av1_remove_compressor(AV1_COMP *cpi);

void av1_change_config(AV1_COMP *cpi, const AV1EncoderConfig *oxcf);

// receive a frames worth of data. caller can assume that a copy of this
// frame is made and not just a copy of the pointer..
int av1_receive_raw_frame(AV1_COMP *cpi, aom_enc_frame_flags_t frame_flags,
                          YV12_BUFFER_CONFIG *sd, int64_t time_stamp,
                          int64_t end_time_stamp);

int av1_get_compressed_data(AV1_COMP *cpi, unsigned int *frame_flags,
                            size_t *size, uint8_t *dest, int64_t *time_stamp,
                            int64_t *time_end, int flush,
                            const aom_rational_t *timebase);

int av1_encode(AV1_COMP *const cpi, uint8_t *const dest,
               const EncodeFrameInput *const frame_input,
               const EncodeFrameParams *const frame_params,
               EncodeFrameResults *const frame_results);

int av1_get_preview_raw_frame(AV1_COMP *cpi, YV12_BUFFER_CONFIG *dest);

int av1_get_last_show_frame(AV1_COMP *cpi, YV12_BUFFER_CONFIG *frame);

aom_codec_err_t av1_copy_new_frame_enc(AV1_COMMON *cm,
                                       YV12_BUFFER_CONFIG *new_frame,
                                       YV12_BUFFER_CONFIG *sd);

int av1_use_as_reference(AV1_COMP *cpi, int ref_frame_flags);

int av1_copy_reference_enc(AV1_COMP *cpi, int idx, YV12_BUFFER_CONFIG *sd);

int av1_set_reference_enc(AV1_COMP *cpi, int idx, YV12_BUFFER_CONFIG *sd);

void av1_set_frame_size(AV1_COMP *cpi, int width, int height);

int av1_update_entropy(AV1_COMP *cpi, int update);

int av1_set_active_map(AV1_COMP *cpi, unsigned char *map, int rows, int cols);

int av1_get_active_map(AV1_COMP *cpi, unsigned char *map, int rows, int cols);

int av1_set_internal_size(AV1_COMP *cpi, AOM_SCALING horiz_mode,
                          AOM_SCALING vert_mode);

int av1_get_quantizer(struct AV1_COMP *cpi);

int av1_convert_sect5obus_to_annexb(uint8_t *buffer, size_t *input_size);

// av1 uses 10,000,000 ticks/second as time stamp
#define TICKS_PER_SEC 10000000LL

static INLINE int64_t timebase_units_to_ticks(const aom_rational_t *timebase,
                                              int64_t n) {
  return n * TICKS_PER_SEC * timebase->num / timebase->den;
}

static INLINE int64_t ticks_to_timebase_units(const aom_rational_t *timebase,
                                              int64_t n) {
  const int64_t round = TICKS_PER_SEC * timebase->num / 2 - 1;
  return (n * timebase->den + round) / timebase->num / TICKS_PER_SEC;
}

static INLINE int frame_is_kf_gf_arf(const AV1_COMP *cpi) {
  return frame_is_intra_only(&cpi->common) || cpi->refresh_alt_ref_frame ||
         (cpi->refresh_golden_frame && !cpi->rc.is_src_frame_alt_ref);
}

// TODO(huisu@google.com, youzhou@microsoft.com): enable hash-me for HBD.
static INLINE int av1_use_hash_me(const AV1_COMMON *const cm) {
  return cm->allow_screen_content_tools;
}

static INLINE hash_table *av1_get_ref_frame_hash_map(
    const AV1_COMMON *cm, MV_REFERENCE_FRAME ref_frame) {
  const int map_idx = get_ref_frame_map_idx(cm, ref_frame);
  RefCntBuffer *buf =
      (map_idx != INVALID_IDX) ? cm->ref_frame_map[map_idx] : NULL;
  return buf ? &buf->hash_table : NULL;
}

static INLINE const YV12_BUFFER_CONFIG *get_ref_frame_yv12_buf(
    const AV1_COMMON *const cm, MV_REFERENCE_FRAME ref_frame) {
  const RefCntBuffer *const buf = get_ref_frame_buf(cm, ref_frame);
  return buf != NULL ? &buf->buf : NULL;
}

static INLINE int enc_is_ref_frame_buf(const AV1_COMMON *const cm,
                                       const RefCntBuffer *const frame_buf) {
  MV_REFERENCE_FRAME ref_frame;
  for (ref_frame = LAST_FRAME; ref_frame <= ALTREF_FRAME; ++ref_frame) {
    const RefCntBuffer *const buf = get_ref_frame_buf(cm, ref_frame);
    if (buf == NULL) continue;
    if (frame_buf == buf) break;
  }
  return (ref_frame <= ALTREF_FRAME);
}

static INLINE void alloc_frame_mvs(AV1_COMMON *const cm, RefCntBuffer *buf) {
  assert(buf != NULL);
  ensure_mv_buffer(buf, cm);
  buf->width = cm->width;
  buf->height = cm->height;
}

// Token buffer is only used for palette tokens.
static INLINE unsigned int get_token_alloc(int mb_rows, int mb_cols,
                                           int sb_size_log2,
                                           const int num_planes) {
  // Calculate the maximum number of max superblocks in the image.
  const int shift = sb_size_log2 - 4;
  const int sb_size = 1 << sb_size_log2;
  const int sb_size_square = sb_size * sb_size;
  const int sb_rows = ALIGN_POWER_OF_TWO(mb_rows, shift) >> shift;
  const int sb_cols = ALIGN_POWER_OF_TWO(mb_cols, shift) >> shift;

  // One palette token for each pixel. There can be palettes on two planes.
  const int sb_palette_toks = AOMMIN(2, num_planes) * sb_size_square;

  return sb_rows * sb_cols * sb_palette_toks;
}

// Get the allocated token size for a tile. It does the same calculation as in
// the frame token allocation.
static INLINE unsigned int allocated_tokens(TileInfo tile, int sb_size_log2,
                                            int num_planes) {
  int tile_mb_rows = (tile.mi_row_end - tile.mi_row_start + 2) >> 2;
  int tile_mb_cols = (tile.mi_col_end - tile.mi_col_start + 2) >> 2;

  return get_token_alloc(tile_mb_rows, tile_mb_cols, sb_size_log2, num_planes);
}

static INLINE void get_start_tok(AV1_COMP *cpi, int tile_row, int tile_col,
                                 int mi_row, TOKENEXTRA **tok, int sb_size_log2,
                                 int num_planes) {
  AV1_COMMON *const cm = &cpi->common;
  const int tile_cols = cm->tile_cols;
  TileDataEnc *this_tile = &cpi->tile_data[tile_row * tile_cols + tile_col];
  const TileInfo *const tile_info = &this_tile->tile_info;

  const int tile_mb_cols =
      (tile_info->mi_col_end - tile_info->mi_col_start + 2) >> 2;
  const int tile_mb_row = (mi_row - tile_info->mi_row_start + 2) >> 2;

  *tok = cpi->tile_tok[tile_row][tile_col] +
         get_token_alloc(tile_mb_row, tile_mb_cols, sb_size_log2, num_planes);
}

void av1_apply_encoding_flags(AV1_COMP *cpi, aom_enc_frame_flags_t flags);

#define ALT_MIN_LAG 3
static INLINE int is_altref_enabled(const AV1_COMP *const cpi) {
  return cpi->oxcf.lag_in_frames >= ALT_MIN_LAG && cpi->oxcf.enable_auto_arf;
}

// TODO(zoeliu): To set up cpi->oxcf.enable_auto_brf

static INLINE void set_ref_ptrs(const AV1_COMMON *cm, MACROBLOCKD *xd,
                                MV_REFERENCE_FRAME ref0,
                                MV_REFERENCE_FRAME ref1) {
  xd->block_ref_scale_factors[0] =
      get_ref_scale_factors_const(cm, ref0 >= LAST_FRAME ? ref0 : 1);
  xd->block_ref_scale_factors[1] =
      get_ref_scale_factors_const(cm, ref1 >= LAST_FRAME ? ref1 : 1);
}

static INLINE int get_chessboard_index(int frame_index) {
  return frame_index & 0x1;
}

static INLINE int *cond_cost_list(const struct AV1_COMP *cpi, int *cost_list) {
  return cpi->sf.mv.subpel_search_method != SUBPEL_TREE ? cost_list : NULL;
}

void av1_new_framerate(AV1_COMP *cpi, double framerate);

void av1_setup_frame_size(AV1_COMP *cpi);

#define LAYER_IDS_TO_IDX(sl, tl, num_tl) ((sl) * (num_tl) + (tl))

// Returns 1 if a frame is scaled and 0 otherwise.
static INLINE int av1_resize_scaled(const AV1_COMMON *cm) {
  return !(cm->superres_upscaled_width == cm->render_width &&
           cm->superres_upscaled_height == cm->render_height);
}

static INLINE int av1_frame_scaled(const AV1_COMMON *cm) {
  return !av1_superres_scaled(cm) && av1_resize_scaled(cm);
}

// Don't allow a show_existing_frame to coincide with an error resilient
// frame. An exception can be made for a forward keyframe since it has no
// previous dependencies.
static INLINE int encode_show_existing_frame(const AV1_COMMON *cm) {
  return cm->show_existing_frame && (!cm->error_resilient_mode ||
                                     cm->current_frame.frame_type == KEY_FRAME);
}

// Lighter version of set_offsets that only sets the mode info
// pointers.
static INLINE void set_mode_info_offsets(const AV1_COMP *const cpi,
                                         MACROBLOCK *const x,
                                         MACROBLOCKD *const xd, int mi_row,
                                         int mi_col) {
  const AV1_COMMON *const cm = &cpi->common;
  const int idx_str = xd->mi_stride * mi_row + mi_col;
  xd->mi = cm->mi_grid_visible + idx_str;
  xd->mi[0] = cm->mi + idx_str;
  x->mbmi_ext = cpi->mbmi_ext_base + (mi_row * cm->mi_cols + mi_col);
}

// Check to see if the given partition size is allowed for a specified number
// of mi block rows and columns remaining in the image.
// If not then return the largest allowed partition size
static INLINE BLOCK_SIZE find_partition_size(BLOCK_SIZE bsize, int rows_left,
                                             int cols_left, int *bh, int *bw) {
  int int_size = (int)bsize;
  if (rows_left <= 0 || cols_left <= 0) {
    return AOMMIN(bsize, BLOCK_8X8);
  } else {
    for (; int_size > 0; int_size -= 3) {
      *bh = mi_size_high[int_size];
      *bw = mi_size_wide[int_size];
      if ((*bh <= rows_left) && (*bw <= cols_left)) {
        break;
      }
    }
  }
  return (BLOCK_SIZE)int_size;
}

// Returns a Sequence Header OBU stored in an aom_fixed_buf_t, or NULL upon
// failure. When a non-NULL aom_fixed_buf_t pointer is returned by this
// function, the memory must be freed by the caller. Both the buf member of the
// aom_fixed_buf_t, and the aom_fixed_buf_t pointer itself must be freed. Memory
// returned must be freed via call to free().
//
// Note: The OBU returned is in Low Overhead Bitstream Format. Specifically,
// the obu_has_size_field bit is set, and the buffer contains the obu_size
// field.
aom_fixed_buf_t *av1_get_global_headers(AV1_COMP *cpi);

#if CONFIG_COLLECT_PARTITION_STATS
static INLINE void av1_print_partition_stats(PartitionStats *part_stats) {
  FILE *f = fopen("partition_stats.csv", "w");
  if (!f) {
    return;
  }

  fprintf(f, "bsize,redo,");
  for (int part = 0; part < EXT_PARTITION_TYPES; part++) {
    fprintf(f, "decision_%d,", part);
  }
  for (int part = 0; part < EXT_PARTITION_TYPES; part++) {
    fprintf(f, "attempt_%d,", part);
  }
  fprintf(f, "\n");

  const int bsizes[6] = { 128, 64, 32, 16, 8, 4 };

  for (int bsize_idx = 0; bsize_idx < 6; bsize_idx++) {
    fprintf(f, "%d,%d,", bsizes[bsize_idx], part_stats->partition_redo);
    for (int part = 0; part < EXT_PARTITION_TYPES; part++) {
      fprintf(f, "%d,", part_stats->partition_decisions[bsize_idx][part]);
    }
    for (int part = 0; part < EXT_PARTITION_TYPES; part++) {
      fprintf(f, "%d,", part_stats->partition_attempts[bsize_idx][part]);
    }
    fprintf(f, "\n");
  }
  fclose(f);
}

static INLINE int av1_get_bsize_idx_for_part_stats(BLOCK_SIZE bsize) {
  assert(bsize == BLOCK_128X128 || bsize == BLOCK_64X64 ||
         bsize == BLOCK_32X32 || bsize == BLOCK_16X16 || bsize == BLOCK_8X8);
  switch (bsize) {
    case BLOCK_128X128: return 0;
    case BLOCK_64X64: return 1;
    case BLOCK_32X32: return 2;
    case BLOCK_16X16: return 3;
    case BLOCK_8X8: return 4;
    case BLOCK_4X4: return 5;
    default: assert(0 && "Invalid bsize for partition_stats."); return -1;
  }
}
#endif

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_ENCODER_ENCODER_H_
