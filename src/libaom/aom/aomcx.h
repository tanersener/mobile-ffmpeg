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
#ifndef AOM_AOM_AOMCX_H_
#define AOM_AOM_AOMCX_H_

/*!\defgroup aom_encoder AOMedia AOM/AV1 Encoder
 * \ingroup aom
 *
 * @{
 */
#include "aom/aom.h"
#include "aom/aom_encoder.h"

/*!\file
 * \brief Provides definitions for using AOM or AV1 encoder algorithm within the
 *        aom Codec Interface.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*!\name Algorithm interface for AV1
 *
 * This interface provides the capability to encode raw AV1 streams.
 * @{
 */
extern aom_codec_iface_t aom_codec_av1_cx_algo;
extern aom_codec_iface_t *aom_codec_av1_cx(void);
/*!@} - end algorithm interface member group*/

/*
 * Algorithm Flags
 */

/*!\brief Don't reference the last frame
 *
 * When this flag is set, the encoder will not use the last frame as a
 * predictor. When not set, the encoder will choose whether to use the
 * last frame or not automatically.
 */
#define AOM_EFLAG_NO_REF_LAST (1 << 16)
/*!\brief Don't reference the last2 frame
 *
 * When this flag is set, the encoder will not use the last2 frame as a
 * predictor. When not set, the encoder will choose whether to use the
 * last2 frame or not automatically.
 */
#define AOM_EFLAG_NO_REF_LAST2 (1 << 17)
/*!\brief Don't reference the last3 frame
 *
 * When this flag is set, the encoder will not use the last3 frame as a
 * predictor. When not set, the encoder will choose whether to use the
 * last3 frame or not automatically.
 */
#define AOM_EFLAG_NO_REF_LAST3 (1 << 18)
/*!\brief Don't reference the golden frame
 *
 * When this flag is set, the encoder will not use the golden frame as a
 * predictor. When not set, the encoder will choose whether to use the
 * golden frame or not automatically.
 */
#define AOM_EFLAG_NO_REF_GF (1 << 19)

/*!\brief Don't reference the alternate reference frame
 *
 * When this flag is set, the encoder will not use the alt ref frame as a
 * predictor. When not set, the encoder will choose whether to use the
 * alt ref frame or not automatically.
 */
#define AOM_EFLAG_NO_REF_ARF (1 << 20)
/*!\brief Don't reference the bwd reference frame
 *
 * When this flag is set, the encoder will not use the bwd ref frame as a
 * predictor. When not set, the encoder will choose whether to use the
 * bwd ref frame or not automatically.
 */
#define AOM_EFLAG_NO_REF_BWD (1 << 21)
/*!\brief Don't reference the alt2 reference frame
 *
 * When this flag is set, the encoder will not use the alt2 ref frame as a
 * predictor. When not set, the encoder will choose whether to use the
 * alt2 ref frame or not automatically.
 */
#define AOM_EFLAG_NO_REF_ARF2 (1 << 22)

/*!\brief Don't update the last frame
 *
 * When this flag is set, the encoder will not update the last frame with
 * the contents of the current frame.
 */
#define AOM_EFLAG_NO_UPD_LAST (1 << 23)

/*!\brief Don't update the golden frame
 *
 * When this flag is set, the encoder will not update the golden frame with
 * the contents of the current frame.
 */
#define AOM_EFLAG_NO_UPD_GF (1 << 24)

/*!\brief Don't update the alternate reference frame
 *
 * When this flag is set, the encoder will not update the alt ref frame with
 * the contents of the current frame.
 */
#define AOM_EFLAG_NO_UPD_ARF (1 << 25)
/*!\brief Disable entropy update
 *
 * When this flag is set, the encoder will not update its internal entropy
 * model based on the entropy of this frame.
 */
#define AOM_EFLAG_NO_UPD_ENTROPY (1 << 26)
/*!\brief Disable ref frame mvs
 *
 * When this flag is set, the encoder will not allow frames to
 * be encoded using mfmv.
 */
#define AOM_EFLAG_NO_REF_FRAME_MVS (1 << 27)
/*!\brief Enable error resilient frame
 *
 * When this flag is set, the encoder will code frames as error
 * resilient.
 */
#define AOM_EFLAG_ERROR_RESILIENT (1 << 28)
/*!\brief Enable s frame mode
 *
 * When this flag is set, the encoder will code frames as an
 * s frame.
 */
#define AOM_EFLAG_SET_S_FRAME (1 << 29)
/*!\brief Force primary_ref_frame to PRIMARY_REF_NONE
 *
 * When this flag is set, the encoder will set a frame's primary_ref_frame
 * to PRIMARY_REF_NONE
 */
#define AOM_EFLAG_SET_PRIMARY_REF_NONE (1 << 30)

/*!\brief AVx encoder control functions
 *
 * This set of macros define the control functions available for AVx
 * encoder interface.
 *
 * \sa #aom_codec_control
 */
enum aome_enc_control_id {
  /*!\brief Codec control function to set which reference frame encoder can use.
   */
  AOME_USE_REFERENCE = 7,

  /*!\brief Codec control function to pass an ROI map to encoder.
   */
  AOME_SET_ROI_MAP = 8,

  /*!\brief Codec control function to pass an Active map to encoder.
   */
  AOME_SET_ACTIVEMAP,

  /*!\brief Codec control function to set encoder scaling mode.
   */
  AOME_SET_SCALEMODE = 11,

  /*!\brief Codec control function to set encoder spatial layer id.
   */
  AOME_SET_SPATIAL_LAYER_ID = 12,

  /*!\brief Codec control function to set encoder internal speed settings.
   *
   * Changes in this value influences, among others, the encoder's selection
   * of motion estimation methods. Values greater than 0 will increase encoder
   * speed at the expense of quality.
   *
   * \note Valid range: 0..8
   */
  AOME_SET_CPUUSED = 13,

  /*!\brief Codec control function to enable automatic set and use alf frames.
   */
  AOME_SET_ENABLEAUTOALTREF,

  /*!\brief Codec control function to set sharpness.
   */
  AOME_SET_SHARPNESS = AOME_SET_ENABLEAUTOALTREF + 2,

  /*!\brief Codec control function to set the threshold for MBs treated static.
   */
  AOME_SET_STATIC_THRESHOLD,

  /*!\brief Codec control function to get last quantizer chosen by the encoder.
   *
   * Return value uses internal quantizer scale defined by the codec.
   */
  AOME_GET_LAST_QUANTIZER = AOME_SET_STATIC_THRESHOLD + 2,

  /*!\brief Codec control function to get last quantizer chosen by the encoder.
   *
   * Return value uses the 0..63 scale as used by the rc_*_quantizer config
   * parameters.
   */
  AOME_GET_LAST_QUANTIZER_64,

  /*!\brief Codec control function to set the max no of frames to create arf.
   */
  AOME_SET_ARNR_MAXFRAMES,

  /*!\brief Codec control function to set the filter strength for the arf.
   */
  AOME_SET_ARNR_STRENGTH,

  /*!\brief Codec control function to set visual tuning.
   */
  AOME_SET_TUNING = AOME_SET_ARNR_STRENGTH + 2,

  /*!\brief Codec control function to set constrained quality level.
   *
   * \attention For this value to be used aom_codec_enc_cfg_t::g_usage must be
   *            set to #AOM_CQ.
   * \note Valid range: 0..63
   */
  AOME_SET_CQ_LEVEL,

  /*!\brief Codec control function to set Max data rate for Intra frames.
   *
   * This value controls additional clamping on the maximum size of a
   * keyframe. It is expressed as a percentage of the average
   * per-frame bitrate, with the special (and default) value 0 meaning
   * unlimited, or no additional clamping beyond the codec's built-in
   * algorithm.
   *
   * For example, to allocate no more than 4.5 frames worth of bitrate
   * to a keyframe, set this to 450.
   */
  AOME_SET_MAX_INTRA_BITRATE_PCT,

  /*!\brief Codec control function to set number of spatial layers.
   */
  AOME_SET_NUMBER_SPATIAL_LAYERS,

  /*!\brief Codec control function to set max data rate for Inter frames.
   *
   * This value controls additional clamping on the maximum size of an
   * inter frame. It is expressed as a percentage of the average
   * per-frame bitrate, with the special (and default) value 0 meaning
   * unlimited, or no additional clamping beyond the codec's built-in
   * algorithm.
   *
   * For example, to allow no more than 4.5 frames worth of bitrate
   * to an inter frame, set this to 450.
   */
  AV1E_SET_MAX_INTER_BITRATE_PCT = AOME_SET_MAX_INTRA_BITRATE_PCT + 2,

  /*!\brief Boost percentage for Golden Frame in CBR mode.
   *
   * This value controls the amount of boost given to Golden Frame in
   * CBR mode. It is expressed as a percentage of the average
   * per-frame bitrate, with the special (and default) value 0 meaning
   * the feature is off, i.e., no golden frame boost in CBR mode and
   * average bitrate target is used.
   *
   * For example, to allow 100% more bits, i.e, 2X, in a golden frame
   * than average frame, set this to 100.
   */
  AV1E_SET_GF_CBR_BOOST_PCT,

  /*!\brief Codec control function to set lossless encoding mode.
   *
   * AV1 can operate in lossless encoding mode, in which the bitstream
   * produced will be able to decode and reconstruct a perfect copy of
   * input source. This control function provides a mean to switch encoder
   * into lossless coding mode(1) or normal coding mode(0) that may be lossy.
   *                          0 = lossy coding mode
   *                          1 = lossless coding mode
   *
   *  By default, encoder operates in normal coding mode (maybe lossy).
   */
  AV1E_SET_LOSSLESS = AV1E_SET_GF_CBR_BOOST_PCT + 2,

  /** control function to enable the row based multi-threading of encoder. A
   * value that is equal to 1 indicates that row based multi-threading is
   * enabled.
   */
  AV1E_SET_ROW_MT,

  /*!\brief Codec control function to set number of tile columns.
   *
   * In encoding and decoding, AV1 allows an input image frame be partitioned
   * into separate vertical tile columns, which can be encoded or decoded
   * independently. This enables easy implementation of parallel encoding and
   * decoding. The parameter for this control describes the number of tile
   * columns (in log2 units), which has a valid range of [0, 6]:
   *             0 = 1 tile column
   *             1 = 2 tile columns
   *             2 = 4 tile columns
   *             .....
   *             n = 2**n tile columns
   *
   * By default, the value is 0, i.e. one single column tile for entire image.
   */
  AV1E_SET_TILE_COLUMNS,

  /*!\brief Codec control function to set number of tile rows.
   *
   * In encoding and decoding, AV1 allows an input image frame be partitioned
   * into separate horizontal tile rows, which can be encoded or decoded
   * independently. The parameter for this control describes the number of tile
   * rows (in log2 units), which has a valid range of [0, 6]:
   *            0 = 1 tile row
   *            1 = 2 tile rows
   *            2 = 4 tile rows
   *            .....
   *            n = 2**n tile rows
   *
   * By default, the value is 0, i.e. one single row tile for entire image.
   */
  AV1E_SET_TILE_ROWS,

  /*!\brief Codec control function to enable RDO modulated by frame temporal
   * dependency.
   *
   * By default, this feature is off.
   */
  AV1E_SET_ENABLE_TPL_MODEL,

  /*!\brief Codec control function to enable frame parallel decoding feature.
   *
   * AV1 has a bitstream feature to reduce decoding dependency between frames
   * by turning off backward update of probability context used in encoding
   * and decoding. This allows staged parallel processing of more than one
   * video frames in the decoder. This control function provides a mean to
   * turn this feature on or off for bitstreams produced by encoder.
   *
   * By default, this feature is off.
   */
  AV1E_SET_FRAME_PARALLEL_DECODING,

  /*!\brief Codec control function to enable error_resilient_mode
   *
   * AV1 has a bitstream feature to guarantee parseability of a frame
   * by turning on the error_resilient_decoding mode, even though the
   * reference buffers are unreliable or not received.
   *
   * By default, this feature is off.
   */
  AV1E_SET_ERROR_RESILIENT_MODE,

  /*!\brief Codec control function to enable s_frame_mode
   *
   * AV1 has a bitstream feature to designate certain frames as S-frames,
   * from where we can switch to a different stream,
   * even though the reference buffers may not be exactly identical.
   *
   * By default, this feature is off.
   */
  AV1E_SET_S_FRAME_MODE,

  /*!\brief Codec control function to set adaptive quantization mode.
   *
   * AV1 has a segment based feature that allows encoder to adaptively change
   * quantization parameter for each segment within a frame to improve the
   * subjective quality. This control makes encoder operate in one of the
   * several AQ_modes supported.
   *
   * By default, encoder operates with AQ_Mode 0(adaptive quantization off).
   */
  AV1E_SET_AQ_MODE,

  /*!\brief Codec control function to enable/disable periodic Q boost.
   *
   * One AV1 encoder speed feature is to enable quality boost by lowering
   * frame level Q periodically. This control function provides a mean to
   * turn on/off this feature.
   *               0 = off
   *               1 = on
   *
   * By default, the encoder is allowed to use this feature for appropriate
   * encoding modes.
   */
  AV1E_SET_FRAME_PERIODIC_BOOST,

  /*!\brief Codec control function to set noise sensitivity.
   *
   *  0: off, 1: On(YOnly)
   */
  AV1E_SET_NOISE_SENSITIVITY,

  /*!\brief Codec control function to set content type.
   * \note Valid parameter range:
   *              AOM_CONTENT_DEFAULT = Regular video content (Default)
   *              AOM_CONTENT_SCREEN  = Screen capture content
   */
  AV1E_SET_TUNE_CONTENT,

  /*!\brief Codec control function to set CDF update mode.
   *
   *  0: no update          1: update on every frame
   *  2: selectively update
   */
  AV1E_SET_CDF_UPDATE_MODE,

  /*!\brief Codec control function to set color space info.
   * \note Valid ranges: 0..23, default is "Unspecified".
   *                     0 = For future use
   *                     1 = BT.709
   *                     2 = Unspecified
   *                     3 = For future use
   *                     4 = BT.470 System M (historical)
   *                     5 = BT.470 System B, G (historical)
   *                     6 = BT.601
   *                     7 = SMPTE 240
   *                     8 = Generic film (color filters using illuminant C)
   *                     9 = BT.2020, BT.2100
   *                     10 = SMPTE 428 (CIE 1921 XYZ)
   *                     11 = SMPTE RP 431-2
   *                     12 = SMPTE EG 432-1
   *                     13 = For future use (values 13 - 21)
   *                     22 = EBU Tech. 3213-E
   *                     23 = For future use
   *
   */
  AV1E_SET_COLOR_PRIMARIES,

  /*!\brief Codec control function to set transfer function info.
   * \note Valid ranges: 0..19, default is "Unspecified".
   *                     0 = For future use
   *                     1 = BT.709
   *                     2 = Unspecified
   *                     3 = For future use
   *                     4 = BT.470 System M (historical)
   *                     5 = BT.470 System B, G (historical)
   *                     6 = BT.601
   *                     7 = SMPTE 240 M
   *                     8 = Linear
   *                     9 = Logarithmic (100 : 1 range)
   *                     10 = Logarithmic (100 * Sqrt(10) : 1 range)
   *                     11 = IEC 61966-2-4
   *                     12 = BT.1361
   *                     13 = sRGB or sYCC
   *                     14 = BT.2020 10-bit systems
   *                     15 = BT.2020 12-bit systems
   *                     16 = SMPTE ST 2084, ITU BT.2100 PQ
   *                     17 = SMPTE ST 428
   *                     18 = BT.2100 HLG, ARIB STD-B67
   *                     19 = For future use
   *
   */
  AV1E_SET_TRANSFER_CHARACTERISTICS,

  /*!\brief Codec control function to set transfer function info.
   * \note Valid ranges: 0..15, default is "Unspecified".
   *                     0 = Identity matrix
   *                     1 = BT.709
   *                     2 = Unspecified
   *                     3 = For future use
   *                     4 = US FCC 73.628
   *                     5 = BT.470 System B, G (historical)
   *                     6 = BT.601
   *                     7 = SMPTE 240 M
   *                     8 = YCgCo
   *                     9 = BT.2020 non-constant luminance, BT.2100 YCbCr
   *                     10 = BT.2020 constant luminance
   *                     11 = SMPTE ST 2085 YDzDx
   *                     12 = Chromaticity-derived non-constant luminance
   *                     13 = Chromaticity-derived constant luminance
   *                     14 = BT.2100 ICtCp
   *                     15 = For future use
   *
   */
  AV1E_SET_MATRIX_COEFFICIENTS,

  /*!\brief Codec control function to set chroma 4:2:0 sample position info.
   * \note Valid ranges: 0..3, default is "UNKNOWN".
   *                     0 = UNKNOWN,
   *                     1 = VERTICAL
   *                     2 = COLOCATED
   *                     3 = RESERVED
   */
  AV1E_SET_CHROMA_SAMPLE_POSITION,

  /*!\brief Codec control function to set minimum interval between GF/ARF frames
   *
   * By default the value is set as 4.
   */
  AV1E_SET_MIN_GF_INTERVAL,

  /*!\brief Codec control function to set minimum interval between GF/ARF frames
   *
   * By default the value is set as 16.
   */
  AV1E_SET_MAX_GF_INTERVAL,

  /*!\brief Codec control function to get an Active map back from the encoder.
   */
  AV1E_GET_ACTIVEMAP,

  /*!\brief Codec control function to set color range bit.
   * \note Valid ranges: 0..1, default is 0
   *                     0 = Limited range (16..235 or HBD equivalent)
   *                     1 = Full range (0..255 or HBD equivalent)
   */
  AV1E_SET_COLOR_RANGE,

  /*!\brief Codec control function to set intended rendering image size.
   *
   * By default, this is identical to the image size in pixels.
   */
  AV1E_SET_RENDER_SIZE,

  /*!\brief Control to set target sequence level index for a certain operating
   * point(OP).
   * Possible values are in the form of "ABxy"(pad leading zeros if less than
   * 4 digits).
   *   AB: OP index.
   *   xy: Target level index for the OP. Can be values 0~23(corresponding to
   *   level 2.0 ~ 7.3) or 31(maximum level parameter, no level-based
   *   constraints).
   * E.g. "0" means target level index 0 for the 0th OP;
   *      "111" means target level index 11 for the 1st OP;
   *      "1021" means target level index 21 for the 10th OP.
   * If the target level is not specified for an OP, the maximum level parameter
   * of 31 is used as default.
   */
  AV1E_SET_TARGET_SEQ_LEVEL_IDX,

  /*!\brief Codec control function to get sequence level index.
   */
  AV1E_GET_SEQ_LEVEL_IDX,

  /*!\brief Codec control function to set intended superblock size.
   *
   * By default, the superblock size is determined separately for each
   * frame by the encoder.
   *
   * Experiment: EXT_PARTITION
   */
  AV1E_SET_SUPERBLOCK_SIZE,

  /*!\brief Codec control function to enable automatic set and use
   * bwd-pred frames.
   *
   */
  AOME_SET_ENABLEAUTOBWDREF,

  /*!\brief Codec control function to encode with CDEF.
   *
   * CDEF is the constrained directional enhancement filter which is an
   * in-loop filter aiming to remove coding artifacts
   *                          0 = do not apply CDEF
   *                          1 = apply CDEF
   *
   *  By default, the encoder applies CDEF.
   *
   * Experiment: AOM_CDEF
   */
  AV1E_SET_ENABLE_CDEF,

  /*!\brief Codec control function to encode with Loop Restoration Filter.
   *
   *                          0 = do not apply Restoration Filter
   *                          1 = apply Restoration Filter
   *
   *  By default, the encoder applies Restoration Filter.
   *
   */
  AV1E_SET_ENABLE_RESTORATION,

  /*!\brief Codec control function to predict with OBMC mode.
   *
   *                          0 = do not allow OBMC mode
   *                          1 = allow OBMC mode
   *
   *  By default, the encoder allows OBMC prediction mode.
   *
   */
  AV1E_SET_ENABLE_OBMC,

  /*!\brief Codec control function to encode without trellis quantization.
   *
   *                          0 = apply trellis quantization
   *                          1 = do not apply trellis quantization
   *                          2 = disable trellis quantization partially
   *
   *  By default, the encoder applies optimization on quantized
   *  coefficients.
   *
   */
  AV1E_SET_DISABLE_TRELLIS_QUANT,

  /*!\brief Codec control function to encode with quantisation matrices.
   *
   * AOM can operate with default quantisation matrices dependent on
   * quantisation level and block type.
   *                          0 = do not use quantisation matrices
   *                          1 = use quantisation matrices
   *
   *  By default, the encoder operates without quantisation matrices.
   *
   * Experiment: AOM_QM
   */

  AV1E_SET_ENABLE_QM,

  /*!\brief Codec control function to set the min quant matrix flatness.
   *
   * AOM can operate with different ranges of quantisation matrices.
   * As quantisation levels increase, the matrices get flatter. This
   * control sets the minimum level of flatness from which the matrices
   * are determined.
   *
   *  By default, the encoder sets this minimum at half the available
   *  range.
   *
   * Experiment: AOM_QM
   */
  AV1E_SET_QM_MIN,

  /*!\brief Codec control function to set the max quant matrix flatness.
   *
   * AOM can operate with different ranges of quantisation matrices.
   * As quantisation levels increase, the matrices get flatter. This
   * control sets the maximum level of flatness possible.
   *
   * By default, the encoder sets this maximum at the top of the
   * available range.
   *
   * Experiment: AOM_QM
   */
  AV1E_SET_QM_MAX,

  /*!\brief Codec control function to set the min quant matrix flatness.
   *
   * AOM can operate with different ranges of quantisation matrices.
   * As quantisation levels increase, the matrices get flatter. This
   * control sets the flatness for luma (Y).
   *
   *  By default, the encoder sets this minimum at half the available
   *  range.
   *
   * Experiment: AOM_QM
   */
  AV1E_SET_QM_Y,

  /*!\brief Codec control function to set the min quant matrix flatness.
   *
   * AOM can operate with different ranges of quantisation matrices.
   * As quantisation levels increase, the matrices get flatter. This
   * control sets the flatness for chroma (U).
   *
   *  By default, the encoder sets this minimum at half the available
   *  range.
   *
   * Experiment: AOM_QM
   */
  AV1E_SET_QM_U,

  /*!\brief Codec control function to set the min quant matrix flatness.
   *
   * AOM can operate with different ranges of quantisation matrices.
   * As quantisation levels increase, the matrices get flatter. This
   * control sets the flatness for chrome (V).
   *
   *  By default, the encoder sets this minimum at half the available
   *  range.
   *
   * Experiment: AOM_QM
   */
  AV1E_SET_QM_V,

  /*!\brief Codec control function to encode with dist_8x8.
   *
   *  The dist_8x8 is enabled automatically for model tuning parameters that
   *  require measuring distortion at the 8x8 level. This control also allows
   *  measuring distortion at the 8x8 level for other tuning options
   *  (e.g., PSNR), for testing purposes.
   *                          0 = do not use dist_8x8
   *                          1 = use dist_8x8
   *
   *  By default, the encoder does not use dist_8x8
   *
   * Experiment: DIST_8X8
   */
  AV1E_SET_ENABLE_DIST_8X8,

  /*!\brief Codec control function to set a maximum number of tile groups.
   *
   * This will set the maximum number of tile groups. This will be
   * overridden if an MTU size is set. The default value is 1.
   *
   * Experiment: TILE_GROUPS
   */
  AV1E_SET_NUM_TG,

  /*!\brief Codec control function to set an MTU size for a tile group.
   *
   * This will set the maximum number of bytes in a tile group. This can be
   * exceeded only if a single tile is larger than this amount.
   *
   * By default, the value is 0, in which case a fixed number of tile groups
   * is used.
   *
   * Experiment: TILE_GROUPS
   */
  AV1E_SET_MTU,

  /*!\brief Codec control function to set the number of symbols in an ANS data
   * window.
   *
   * The number of ANS symbols (both boolean and non-booleans alphabets) in an
   * ANS data window is set to 1 << value.
   *
   * \note Valid range: [8, 23]
   *
   * Experiment: ANS
   */
  AV1E_SET_ANS_WINDOW_SIZE_LOG2,

  /*!\brief Codec control function to enable/disable rectangular partitions.
   *
   * This will enable or disable usage of rectangular partitions. The default
   * value is 1.
   *
   */
  AV1E_SET_ENABLE_RECT_PARTITIONS,

  /*!\brief Codec control function to enable/disable AB partitions.
   *
   * This will enable or disable usage of AB partitions. The default
   * value is 1.
   *
   */
  AV1E_SET_ENABLE_AB_PARTITIONS,

  /*!\brief Codec control function to enable/disable 1:4 and 4:1 partitions.
   *
   * This will enable or disable usage of 1:4 and 4:1 partitions. The default
   * value is 1.
   *
   */
  AV1E_SET_ENABLE_1TO4_PARTITIONS,

  /*!\brief Codec control function to set min partition size.
   *
   * This will set min partition size. The default value is 4 for 4x4.
   * valid values are [4, 8, 16, 32, 64, 128]
   * min_partition_size is applied to both width and height of the partition.
   * i.e, both width and height of a partition can not be smaller than
   * the min_partition_size, except the partition at the picture boundary.
   *
   */
  AV1E_SET_MIN_PARTITION_SIZE,

  /*!\brief Codec control function to set max partition size.
   *
   * This will set max partition size. The default value is 128 for 128x128.
   * valid values are [4, 8, 16, 32, 64, 128]
   * max_partition_size is applied to both width and height of the partition.
   * i.e, both width and height of a partition can not be larger than
   * the max_partition_size.
   */
  AV1E_SET_MAX_PARTITION_SIZE,

  /*!\brief Codec control function to turn on / off intra edge filter
   * at sequence level.
   *
   * This will enable or disable usage of intra-edge filtering. The default
   * value is 1.
   *
   */
  AV1E_SET_ENABLE_INTRA_EDGE_FILTER,

  /*!\brief Codec control function to turn on / off frame order hint for a
   * few tools:
   *
   * joint compound mode
   * motion field motion vector
   * ref frame sign bias
   *
   * The default value is 1.
   *
   */
  AV1E_SET_ENABLE_ORDER_HINT,

  /*!\brief Codec control function to turn on / off 64-length transforms.
   *
   * This will enable or disable usage of length 64 transforms in any
   * direction. The default value is 1.
   *
   */
  AV1E_SET_ENABLE_TX64,

  /*!\brief Codec control function to turn on / off flip and identity
   * transforms.
   *
   * This will enable or disable usage of flip and identity transform
   * types in any direction. The default value is 1. Including:
   * FLIPADST_DCT, DCT_FLIPADST, FLIPADST_FLIPADST, ADST_FLIPADST,
   * FLIPADST_ADST, IDTX, V_DCT, H_DCT, V_ADST, H_ADST, V_FLIPADST,
   * H_FLIPADST
   */
  AV1E_SET_ENABLE_FLIP_IDTX,

  /*!\brief Codec control function to set transform block size search method.
   *
   * This will set the transform block size search method.
   * 0: use Full RD search, 1: use Fast RD search, 2: always use largest
   * allowed transform block size based on partition size.
   */
  AV1E_SET_TX_SIZE_SEARCH_METHOD,

  /*!\brief Codec control function to turn on / off dist-wtd compound mode
   * at sequence level.
   *
   * This will enable or disable distance-weighted compound mode. The default
   * value is 1. If AV1E_SET_ENABLE_ORDER_HINT is 0, then this flag is forced
   * to 0.
   *
   */
  AV1E_SET_ENABLE_DIST_WTD_COMP,

  /*!\brief Codec control function to turn on / off ref frame mvs (mfmv) usage
   * at sequence level.
   *
   * This will enable or disable usage of MFMV. The default value is 1.
   * If AV1E_SET_ENABLE_ORDER_HINT is 0, then this flag is forced to 0.
   *
   */
  AV1E_SET_ENABLE_REF_FRAME_MVS,

  /*!\brief Codec control function to set temporal mv prediction
   * enabling/disabling at frame level.
   *
   * This will enable or disable temporal mv predicton. The default value is 1.
   * If AV1E_SET_ENABLE_REF_FRAME_MVS is 0, then this flag is forced to 0.
   *
   */
  AV1E_SET_ALLOW_REF_FRAME_MVS,

  /*!\brief Codec control function to turn on / off dual filter usage
   * for a sequence.
   *
   * This will enable or disable use of dual interpolation filter.
   * The default value is 1.
   *
   */
  AV1E_SET_ENABLE_DUAL_FILTER,

  /*!\brief Codec control function to turn on / off masked compound usage
   * for a sequence.
   *
   * This will enable or disable usage of wedge and diff-wtd compound
   * modes. The default value is 1.
   *
   */
  AV1E_SET_ENABLE_MASKED_COMP,

  /*!\brief Codec control function to turn on / off one sided compound usage
   * for a sequence.
   *
   * This will enable or disable usage of one sided compound
   * modes. The default value is 1.
   *
   */
  AV1E_SET_ENABLE_ONESIDED_COMP,

  /*!\brief Codec control function to turn on / off interintra compound
   * for a sequence.
   *
   * This will enable or disable usage of inter-intra compound modes.
   * The default value is 1.
   *
   */
  AV1E_SET_ENABLE_INTERINTRA_COMP,

  /*!\brief Codec control function to turn on / off smooth inter-intra
   * mode for a sequence.
   *
   * This will enable or disable usage of smooth inter-intra mode.
   * The default value is 1.
   *
   */
  AV1E_SET_ENABLE_SMOOTH_INTERINTRA,

  /*!\brief Codec control function to turn on / off difference weighted
   * compound.
   *
   * This will enable or disable usage of difference weighted compound.
   * The default value is 1.
   *
   */
  AV1E_SET_ENABLE_DIFF_WTD_COMP,

  /*!\brief Codec control function to turn on / off interinter wedge
   * compound.
   *
   * This will enable or disable usage of interinter wedge compound.
   * The default value is 1.
   *
   */
  AV1E_SET_ENABLE_INTERINTER_WEDGE,

  /*!\brief Codec control function to turn on / off interintra wedge
   * compound.
   *
   * This will enable or disable usage of interintra wedge compound.
   * The default value is 1.
   *
   */
  AV1E_SET_ENABLE_INTERINTRA_WEDGE,

  /*!\brief Codec control function to turn on / off global motion usage
   * for a sequence.
   *
   * This will enable or disable usage of global motion. The default value is 1.
   *
   */
  AV1E_SET_ENABLE_GLOBAL_MOTION,

  /*!\brief Codec control function to turn on / off warped motion usage
   * at sequence level.
   *
   * This will enable or disable usage of warped motion. The default value is 1.
   *
   */
  AV1E_SET_ENABLE_WARPED_MOTION,

  /*!\brief Codec control function to turn on / off warped motion usage
   * at frame level.
   *
   * This will enable or disable usage of warped motion. The default value is 1.
   * If AV1E_SET_ENABLE_WARPED_MOTION is 0, then this flag is forced to 0.
   *
   */
  AV1E_SET_ALLOW_WARPED_MOTION,

  /*!\brief Codec control function to turn on / off filter intra usage at
   * sequence level.
   *
   * This will enable or disable usage of filter intra. The default value is 1.
   * If AV1E_SET_ENABLE_FILTER_INTRA is 0, then this flag is forced to 0.
   *
   */
  AV1E_SET_ENABLE_FILTER_INTRA,

  /*!\brief Codec control function to turn on / off smooth intra modes usage.
   *
   * This will enable or disable usage of smooth, smooth_h and smooth_v intra
   * modes. The default value is 1.
   *
   */
  AV1E_SET_ENABLE_SMOOTH_INTRA,

  /*!\brief Codec control function to turn on / off Paeth intra mode usage.
   *
   * This will enable or disable usage of Paeth intra mode. The default value
   * is 1.
   *
   */
  AV1E_SET_ENABLE_PAETH_INTRA,

  /*!\brief Codec control function to turn on / off CFL uv intra mode usage.
   *
   * This will enable or disable usage of chroma-from-luma intra mode. The
   * default value is 1.
   *
   */
  AV1E_SET_ENABLE_CFL_INTRA,

  /*!\brief Codec control function to turn on / off frame superresolution.
   *
   * This will enable or disable frame superresolution. The default value is 1
   * If AV1E_SET_ENABLE_SUPERRES is 0, then this flag is forced to 0.
   */
  AV1E_SET_ENABLE_SUPERRES,

  /*!\brief Codec control function to turn on/off palette mode */
  AV1E_SET_ENABLE_PALETTE,

  /*!\brief Codec control function to turn on/off intra block copy mode */
  AV1E_SET_ENABLE_INTRABC,

  /*!\brief Codec control function to turn on/off intra angle delta */
  AV1E_SET_ENABLE_ANGLE_DELTA,

  /*!\brief Codec control function to set the delta q mode
   *
   * AV1 has a segment based feature that allows encoder to adaptively change
   * quantization parameter for each segment within a frame to improve the
   * subjective quality. the delta q mode is added on top of segment based
   * feature, and allows control per 64x64 q and lf delta.This control makes
   * encoder operate in one of the several DELTA_Q_modes supported.
   *
   * By default, encoder operates with DELTAQ_Mode 0(deltaq signaling off).
   */
  AV1E_SET_DELTAQ_MODE,

  /*!\brief Codec control function to set the single tile decoding mode to 0 or
   * 1.
   *
   * 0 means that the single tile decoding is off, and 1 means that the single
   * tile decoding is on.
   *
   * Experiment: EXT_TILE
   */
  AV1E_SET_SINGLE_TILE_DECODING,

  /*!\brief Codec control function to enable the extreme motion vector unit test
   * in AV1. Please note that this is only used in motion vector unit test.
   *
   * 0 : off, 1 : MAX_EXTREME_MV, 2 : MIN_EXTREME_MV
   */
  AV1E_ENABLE_MOTION_VECTOR_UNIT_TEST,

  /*!\brief Codec control function to signal picture timing info in the
   * bitstream. \note Valid ranges: 0..1, default is "UNKNOWN". 0 = UNKNOWN, 1 =
   * EQUAL
   */
  AV1E_SET_TIMING_INFO_TYPE,

  /*!\brief Codec control function to add film grain parameters (one of several
   * preset types) info in the bitstream.
   * \note Valid ranges: 0..11, default is "0". 0 = UNKNOWN,
   * 1..16 = different test vectors for grain
   */
  AV1E_SET_FILM_GRAIN_TEST_VECTOR,

  /*!\brief Codec control function to set the path to the film grain parameters
   */
  AV1E_SET_FILM_GRAIN_TABLE,

  /*!\brief Sets the noise level */
  AV1E_SET_DENOISE_NOISE_LEVEL,

  /*!\brief Sets the denoisers block size */
  AV1E_SET_DENOISE_BLOCK_SIZE,

  /*!\brief Sets the chroma subsampling x value */
  AV1E_SET_CHROMA_SUBSAMPLING_X,

  /*!\brief Sets the chroma subsampling y value */
  AV1E_SET_CHROMA_SUBSAMPLING_Y,

  /*!\brief Control to use a reduced tx type set */
  AV1E_SET_REDUCED_TX_TYPE_SET,

  /*!\brief Control to use dct only for intra modes */
  AV1E_SET_INTRA_DCT_ONLY,

  /*!\brief Control to use dct only for inter modes */
  AV1E_SET_INTER_DCT_ONLY,

  /*!\brief Control to use default tx type only for intra modes */
  AV1E_SET_INTRA_DEFAULT_TX_ONLY,

  /*!\brief Control to use adaptive quantize_b */
  AV1E_SET_QUANT_B_ADAPT,

  /*!\brief Control to select maximum height for the GF group pyramid structure
   * (valid values: 0 - 4) */
  AV1E_SET_GF_MAX_PYRAMID_HEIGHT,

  /*!\brief Control to select maximum reference frames allowed per frame
   * (valid values: 3 - 7) */
  AV1E_SET_MAX_REFERENCE_FRAMES,

  /*!\brief Control to use reduced set of single and compound references. */
  AV1E_SET_REDUCED_REFERENCE_SET,

  /*!\brief Control to set frequency of the cost updates for coefficients
   * Possible values are:
   * 0: Update at SB level (default)
   * 1: Update at SB row level in tile
   * 2: Update at tile level
   */
  AV1E_SET_COEFF_COST_UPD_FREQ,

  /*!\brief Control to set frequency of the cost updates for mode
   * Possible values are:
   * 0: Update at SB level (default)
   * 1: Update at SB row level in tile
   * 2: Update at tile level
   */
  AV1E_SET_MODE_COST_UPD_FREQ,

  /*!\brief Control to set bit mask that specifies which tier each of the 32
   * possible operating points conforms to.
   * Bit value 0: Main Tier; 1: High Tier.
   */
  AV1E_SET_TIER_MASK,
};

/*!\brief aom 1-D scaling mode
 *
 * This set of constants define 1-D aom scaling modes
 */
typedef enum aom_scaling_mode_1d {
  AOME_NORMAL = 0,
  AOME_FOURFIVE = 1,
  AOME_THREEFIVE = 2,
  AOME_ONETWO = 3
} AOM_SCALING_MODE;

/*!\brief Max number of segments
 *
 * This is the limit of number of segments allowed within a frame.
 *
 * Currently same as "MAX_SEGMENTS" in AV1, the maximum that AV1 supports.
 *
 */
#define AOM_MAX_SEGMENTS 8

/*!\brief  aom region of interest map
 *
 * These defines the data structures for the region of interest map
 *
 * TODO(yaowu): create a unit test for ROI map related APIs
 *
 */
typedef struct aom_roi_map {
  /*! An id between 0 and 7 for each 8x8 region within a frame. */
  unsigned char *roi_map;
  unsigned int rows;              /**< Number of rows. */
  unsigned int cols;              /**< Number of columns. */
  int delta_q[AOM_MAX_SEGMENTS];  /**< Quantizer deltas. */
  int delta_lf[AOM_MAX_SEGMENTS]; /**< Loop filter deltas. */
  /*! Static breakout threshold for each segment. */
  unsigned int static_threshold[AOM_MAX_SEGMENTS];
} aom_roi_map_t;

/*!\brief  aom active region map
 *
 * These defines the data structures for active region map
 *
 */

typedef struct aom_active_map {
  /*!\brief specify an on (1) or off (0) each 16x16 region within a frame */
  unsigned char *active_map;
  unsigned int rows; /**< number of rows */
  unsigned int cols; /**< number of cols */
} aom_active_map_t;

/*!\brief  aom image scaling mode
 *
 * This defines the data structure for image scaling mode
 *
 */
typedef struct aom_scaling_mode {
  AOM_SCALING_MODE h_scaling_mode; /**< horizontal scaling mode */
  AOM_SCALING_MODE v_scaling_mode; /**< vertical scaling mode   */
} aom_scaling_mode_t;

/*!brief AV1 encoder content type */
typedef enum {
  AOM_CONTENT_DEFAULT,
  AOM_CONTENT_SCREEN,
  AOM_CONTENT_INVALID
} aom_tune_content;

/*!brief AV1 encoder timing info type signaling */
typedef enum {
  AOM_TIMING_UNSPECIFIED,
  AOM_TIMING_EQUAL,
  AOM_TIMING_DEC_MODEL
} aom_timing_info_type_t;

/*!\brief Model tuning parameters
 *
 * Changes the encoder to tune for certain types of input material.
 *
 */
typedef enum {
  AOM_TUNE_PSNR,
  AOM_TUNE_SSIM,
  AOM_TUNE_CDEF_DIST,
  AOM_TUNE_DAALA_DIST
} aom_tune_metric;

/*!\cond */
/*!\brief Encoder control function parameter type
 *
 * Defines the data types that AOME/AV1E control functions take. Note that
 * additional common controls are defined in aom.h
 *
 */

AOM_CTRL_USE_TYPE(AOME_USE_REFERENCE, int)
#define AOM_CTRL_AOME_USE_REFERENCE
AOM_CTRL_USE_TYPE(AOME_SET_ROI_MAP, aom_roi_map_t *)
#define AOM_CTRL_AOME_SET_ROI_MAP
AOM_CTRL_USE_TYPE(AOME_SET_ACTIVEMAP, aom_active_map_t *)
#define AOM_CTRL_AOME_SET_ACTIVEMAP
AOM_CTRL_USE_TYPE(AOME_SET_SCALEMODE, aom_scaling_mode_t *)
#define AOM_CTRL_AOME_SET_SCALEMODE

AOM_CTRL_USE_TYPE(AOME_SET_SPATIAL_LAYER_ID, unsigned int)
#define AOM_CTRL_AOME_SET_SPATIAL_LAYER_ID

AOM_CTRL_USE_TYPE(AOME_SET_CPUUSED, int)
#define AOM_CTRL_AOME_SET_CPUUSED
AOM_CTRL_USE_TYPE(AOME_SET_ENABLEAUTOALTREF, unsigned int)
#define AOM_CTRL_AOME_SET_ENABLEAUTOALTREF

AOM_CTRL_USE_TYPE(AOME_SET_ENABLEAUTOBWDREF, unsigned int)
#define AOM_CTRL_AOME_SET_ENABLEAUTOBWDREF

AOM_CTRL_USE_TYPE(AOME_SET_SHARPNESS, unsigned int)
#define AOM_CTRL_AOME_SET_SHARPNESS
AOM_CTRL_USE_TYPE(AOME_SET_STATIC_THRESHOLD, unsigned int)
#define AOM_CTRL_AOME_SET_STATIC_THRESHOLD

AOM_CTRL_USE_TYPE(AOME_SET_ARNR_MAXFRAMES, unsigned int)
#define AOM_CTRL_AOME_SET_ARNR_MAXFRAMES
AOM_CTRL_USE_TYPE(AOME_SET_ARNR_STRENGTH, unsigned int)
#define AOM_CTRL_AOME_SET_ARNR_STRENGTH
AOM_CTRL_USE_TYPE(AOME_SET_TUNING, int) /* aom_tune_metric */
#define AOM_CTRL_AOME_SET_TUNING
AOM_CTRL_USE_TYPE(AOME_SET_CQ_LEVEL, unsigned int)
#define AOM_CTRL_AOME_SET_CQ_LEVEL

AOM_CTRL_USE_TYPE(AV1E_SET_ROW_MT, unsigned int)
#define AOM_CTRL_AV1E_SET_ROW_MT

AOM_CTRL_USE_TYPE(AV1E_SET_TILE_COLUMNS, unsigned int)
#define AOM_CTRL_AV1E_SET_TILE_COLUMNS
AOM_CTRL_USE_TYPE(AV1E_SET_TILE_ROWS, unsigned int)
#define AOM_CTRL_AV1E_SET_TILE_ROWS

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_TPL_MODEL, unsigned int)
#define AOM_CTRL_AV1E_SET_ENABLE_TPL_MODEL

AOM_CTRL_USE_TYPE(AOME_GET_LAST_QUANTIZER, int *)
#define AOM_CTRL_AOME_GET_LAST_QUANTIZER
AOM_CTRL_USE_TYPE(AOME_GET_LAST_QUANTIZER_64, int *)
#define AOM_CTRL_AOME_GET_LAST_QUANTIZER_64

AOM_CTRL_USE_TYPE(AOME_SET_MAX_INTRA_BITRATE_PCT, unsigned int)
#define AOM_CTRL_AOME_SET_MAX_INTRA_BITRATE_PCT
AOM_CTRL_USE_TYPE(AOME_SET_MAX_INTER_BITRATE_PCT, unsigned int)
#define AOM_CTRL_AOME_SET_MAX_INTER_BITRATE_PCT

AOM_CTRL_USE_TYPE(AOME_SET_NUMBER_SPATIAL_LAYERS, int)
#define AOME_CTRL_AOME_SET_NUMBER_SPATIAL_LAYERS

AOM_CTRL_USE_TYPE(AV1E_SET_GF_CBR_BOOST_PCT, unsigned int)
#define AOM_CTRL_AV1E_SET_GF_CBR_BOOST_PCT

AOM_CTRL_USE_TYPE(AV1E_SET_LOSSLESS, unsigned int)
#define AOM_CTRL_AV1E_SET_LOSSLESS

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_CDEF, unsigned int)
#define AOM_CTRL_AV1E_SET_ENABLE_CDEF

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_RESTORATION, unsigned int)
#define AOM_CTRL_AV1E_SET_ENABLE_RESTORATION

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_OBMC, unsigned int)
#define AOM_CTRL_AV1E_SET_ENABLE_OBMC

AOM_CTRL_USE_TYPE(AV1E_SET_DISABLE_TRELLIS_QUANT, unsigned int)
#define AOM_CTRL_AV1E_SET_DISABLE_TRELLIS_QUANT

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_QM, unsigned int)
#define AOM_CTRL_AV1E_SET_ENABLE_QM

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_DIST_8X8, unsigned int)
#define AOM_CTRL_AV1E_SET_ENABLE_DIST_8X8

AOM_CTRL_USE_TYPE(AV1E_SET_QM_MIN, unsigned int)
#define AOM_CTRL_AV1E_SET_QM_MIN

AOM_CTRL_USE_TYPE(AV1E_SET_QM_MAX, unsigned int)
#define AOM_CTRL_AV1E_SET_QM_MAX

AOM_CTRL_USE_TYPE(AV1E_SET_QM_Y, unsigned int)
#define AOM_CTRL_AV1E_SET_QM_Y

AOM_CTRL_USE_TYPE(AV1E_SET_QM_U, unsigned int)
#define AOM_CTRL_AV1E_SET_QM_U

AOM_CTRL_USE_TYPE(AV1E_SET_QM_V, unsigned int)
#define AOM_CTRL_AV1E_SET_QM_V

AOM_CTRL_USE_TYPE(AV1E_SET_NUM_TG, unsigned int)
#define AOM_CTRL_AV1E_SET_NUM_TG
AOM_CTRL_USE_TYPE(AV1E_SET_MTU, unsigned int)
#define AOM_CTRL_AV1E_SET_MTU

AOM_CTRL_USE_TYPE(AV1E_SET_TIMING_INFO_TYPE, int) /* aom_timing_info_type_t */
#define AOM_CTRL_AV1E_SET_TIMING_INFO_TYPE

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_RECT_PARTITIONS, int)
#define AOM_CTRL_AV1E_SET_ENABLE_RECT_PARTITIONS

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_AB_PARTITIONS, int)
#define AOM_CTRL_AV1E_SET_ENABLE_AB_PARTITIONS

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_1TO4_PARTITIONS, int)
#define AOM_CTRL_AV1E_SET_ENABLE_1TO4_PARTITIONS

AOM_CTRL_USE_TYPE(AV1E_SET_MIN_PARTITION_SIZE, int)
#define AOM_CTRL_AV1E_SET_MIN_PARTITION_SIZE

AOM_CTRL_USE_TYPE(AV1E_SET_MAX_PARTITION_SIZE, int)
#define AOM_CTRL_AV1E_SET_MAX_PARTITION_SIZE

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_INTRA_EDGE_FILTER, int)
#define AOM_CTRL_AV1E_SET_ENABLE_INTRA_EDGE_FILTER

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_ORDER_HINT, int)
#define AOM_CTRL_AV1E_SET_ENABLE_ORDER_HINT

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_TX64, int)
#define AOM_CTRL_AV1E_SET_ENABLE_TX64

AOM_CTRL_USE_TYPE(AV1E_SET_TX_SIZE_SEARCH_METHOD, int)
#define AOM_CTRL_AV1E_SET_TXSIZE_SEARCH_METHOD

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_FLIP_IDTX, int)
#define AOM_CTRL_AV1E_SET_ENABLE_FLIP_IDTX

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_DIST_WTD_COMP, int)
#define AOM_CTRL_AV1E_SET_ENABLE_DIST_WTD_COMP

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_REF_FRAME_MVS, int)
#define AOM_CTRL_AV1E_SET_ENABLE_REF_FRAME_MVS

AOM_CTRL_USE_TYPE(AV1E_SET_ALLOW_REF_FRAME_MVS, int)
#define AOM_CTRL_AV1E_SET_ALLOW_REF_FRAME_MVS

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_DUAL_FILTER, int)
#define AOM_CTRL_AV1E_SET_ENABLE_DUAL_FILTER

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_MASKED_COMP, int)
#define AOM_CTRL_AV1E_SET_ENABLE_MASKED_COMP

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_ONESIDED_COMP, int)
#define AOM_CTRL_AV1E_SET_ENABLE_ONESIDED_COMP

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_INTERINTRA_COMP, int)
#define AOM_CTRL_AV1E_SET_ENABLE_INTERINTRA_COMP

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_SMOOTH_INTERINTRA, int)
#define AOM_CTRL_AV1E_SET_ENABLE_SMOOTH_INTERINTRA

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_DIFF_WTD_COMP, int)
#define AOM_CTRL_AV1E_SET_ENABLE_DIFF_WTD_COMP

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_INTERINTER_WEDGE, int)
#define AOM_CTRL_AV1E_SET_ENABLE_INTERINTER_WEDGE

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_INTERINTRA_WEDGE, int)
#define AOM_CTRL_AV1E_SET_ENABLE_INTERINTRA_WEDGE

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_GLOBAL_MOTION, int)
#define AOM_CTRL_AV1E_SET_ENABLE_GLOBAL_MOTION

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_WARPED_MOTION, int)
#define AOM_CTRL_AV1E_SET_ENABLE_WARPED_MOTION

AOM_CTRL_USE_TYPE(AV1E_SET_ALLOW_WARPED_MOTION, int)
#define AOM_CTRL_AV1E_SET_ALLOW_WARPED_MOTION

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_FILTER_INTRA, int)
#define AOM_CTRL_AV1E_SET_ENABLE_FILTER_INTRA

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_SMOOTH_INTRA, int)
#define AOM_CTRL_AV1E_SET_ENABLE_SMOOTH_INTRA

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_PAETH_INTRA, int)
#define AOM_CTRL_AV1E_SET_ENABLE_PAETH_INTRA

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_CFL_INTRA, int)
#define AOM_CTRL_AV1E_SET_ENABLE_CFL_INTRA

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_SUPERRES, int)
#define AOM_CTRL_AV1E_SET_ENABLE_SUPERRES

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_PALETTE, int)
#define AOM_CTRL_AV1E_SET_ENABLE_PALETTE

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_INTRABC, int)
#define AOM_CTRL_AV1E_SET_ENABLE_INTRABC

AOM_CTRL_USE_TYPE(AV1E_SET_ENABLE_ANGLE_DELTA, int)
#define AOM_CTRL_AV1E_SET_ENABLE_ANGLE_DELTA

AOM_CTRL_USE_TYPE(AV1E_SET_FRAME_PARALLEL_DECODING, unsigned int)
#define AOM_CTRL_AV1E_SET_FRAME_PARALLEL_DECODING

AOM_CTRL_USE_TYPE(AV1E_SET_ERROR_RESILIENT_MODE, int)
#define AOM_CTRL_AV1E_SET_ERROR_RESILIENT_MODE

AOM_CTRL_USE_TYPE(AV1E_SET_S_FRAME_MODE, int)
#define AOM_CTRL_AV1E_SET_S_FRAME_MODE

AOM_CTRL_USE_TYPE(AV1E_SET_AQ_MODE, unsigned int)
#define AOM_CTRL_AV1E_SET_AQ_MODE

AOM_CTRL_USE_TYPE(AV1E_SET_DELTAQ_MODE, unsigned int)
#define AOM_CTRL_AV1E_SET_DELTAQ_MODE

AOM_CTRL_USE_TYPE(AV1E_SET_FRAME_PERIODIC_BOOST, unsigned int)
#define AOM_CTRL_AV1E_SET_FRAME_PERIODIC_BOOST

AOM_CTRL_USE_TYPE(AV1E_SET_NOISE_SENSITIVITY, unsigned int)
#define AOM_CTRL_AV1E_SET_NOISE_SENSITIVITY

AOM_CTRL_USE_TYPE(AV1E_SET_TUNE_CONTENT, int) /* aom_tune_content */
#define AOM_CTRL_AV1E_SET_TUNE_CONTENT

AOM_CTRL_USE_TYPE(AV1E_SET_COLOR_PRIMARIES, int)
#define AOM_CTRL_AV1E_SET_COLOR_PRIMARIES

AOM_CTRL_USE_TYPE(AV1E_SET_TRANSFER_CHARACTERISTICS, int)
#define AOM_CTRL_AV1E_SET_TRANSFER_CHARACTERISTICS

AOM_CTRL_USE_TYPE(AV1E_SET_MATRIX_COEFFICIENTS, int)
#define AOM_CTRL_AV1E_SET_MATRIX_COEFFICIENTS

AOM_CTRL_USE_TYPE(AV1E_SET_CHROMA_SAMPLE_POSITION, int)
#define AOM_CTRL_AV1E_SET_CHROMA_SAMPLE_POSITION

AOM_CTRL_USE_TYPE(AV1E_SET_MIN_GF_INTERVAL, unsigned int)
#define AOM_CTRL_AV1E_SET_MIN_GF_INTERVAL

AOM_CTRL_USE_TYPE(AV1E_SET_MAX_GF_INTERVAL, unsigned int)
#define AOM_CTRL_AV1E_SET_MAX_GF_INTERVAL

AOM_CTRL_USE_TYPE(AV1E_GET_ACTIVEMAP, aom_active_map_t *)
#define AOM_CTRL_AV1E_GET_ACTIVEMAP

AOM_CTRL_USE_TYPE(AV1E_SET_COLOR_RANGE, int)
#define AOM_CTRL_AV1E_SET_COLOR_RANGE

#define AOM_CTRL_AV1E_SET_RENDER_SIZE
AOM_CTRL_USE_TYPE(AV1E_SET_RENDER_SIZE, int *)

AOM_CTRL_USE_TYPE(AV1E_SET_SUPERBLOCK_SIZE, unsigned int)
#define AOM_CTRL_AV1E_SET_SUPERBLOCK_SIZE

AOM_CTRL_USE_TYPE(AV1E_GET_SEQ_LEVEL_IDX, int *)
#define AOM_CTRL_AV1E_GET_SEQ_LEVEL_IDX

AOM_CTRL_USE_TYPE(AV1E_SET_SINGLE_TILE_DECODING, unsigned int)
#define AOM_CTRL_AV1E_SET_SINGLE_TILE_DECODING

AOM_CTRL_USE_TYPE(AV1E_ENABLE_MOTION_VECTOR_UNIT_TEST, unsigned int)
#define AOM_CTRL_AV1E_ENABLE_MOTION_VECTOR_UNIT_TEST

AOM_CTRL_USE_TYPE(AV1E_SET_FILM_GRAIN_TEST_VECTOR, int)
#define AOM_CTRL_AV1E_SET_FILM_GRAIN_TEST_VECTOR

AOM_CTRL_USE_TYPE(AV1E_SET_FILM_GRAIN_TABLE, const char *)
#define AOM_CTRL_AV1E_SET_FILM_GRAIN_TABLE

AOM_CTRL_USE_TYPE(AV1E_SET_CDF_UPDATE_MODE, unsigned int)
#define AOM_CTRL_AV1E_SET_CDF_UPDATE_MODE

#ifdef CONFIG_DENOISE
AOM_CTRL_USE_TYPE(AV1E_SET_DENOISE_NOISE_LEVEL, int);
#define AOM_CTRL_AV1E_SET_DENOISE_NOISE_LEVEL

AOM_CTRL_USE_TYPE(AV1E_SET_DENOISE_BLOCK_SIZE, unsigned int);
#define AOM_CTRL_AV1E_SET_DENOISE_BLOCK_SIZE
#endif

AOM_CTRL_USE_TYPE(AV1E_SET_CHROMA_SUBSAMPLING_X, unsigned int)
#define AOM_CTRL_AV1E_SET_CHROMA_SUBSAMPLING_X

AOM_CTRL_USE_TYPE(AV1E_SET_CHROMA_SUBSAMPLING_Y, unsigned int)
#define AOM_CTRL_AV1E_SET_CHROMA_SUBSAMPLING_Y

AOM_CTRL_USE_TYPE(AV1E_SET_REDUCED_TX_TYPE_SET, int)
#define AOM_CTRL_AV1E_SET_REDUCED_TX_TYPE_SET

AOM_CTRL_USE_TYPE(AV1E_SET_INTRA_DCT_ONLY, int)
#define AOM_CTRL_AV1E_SET_INTRA_DCT_ONLY

AOM_CTRL_USE_TYPE(AV1E_SET_INTER_DCT_ONLY, int)
#define AOM_CTRL_AV1E_SET_INTER_DCT_ONLY

AOM_CTRL_USE_TYPE(AV1E_SET_INTRA_DEFAULT_TX_ONLY, int)
#define AOM_CTRL_AV1E_SET_INTRA_DEFAULT_TX_ONLY

AOM_CTRL_USE_TYPE(AV1E_SET_QUANT_B_ADAPT, int)
#define AOM_CTRL_AV1E_SET_QUANT_B_ADAPT

AOM_CTRL_USE_TYPE(AV1E_SET_GF_MAX_PYRAMID_HEIGHT, unsigned int)
#define AOM_CTRL_AV1E_SET_GF_MAX_PYRAMID_HEIGHT

AOM_CTRL_USE_TYPE(AV1E_SET_MAX_REFERENCE_FRAMES, int)
#define AOM_CTRL_AV1E_SET_MAX_REFERENCE_FRAMES

AOM_CTRL_USE_TYPE(AV1E_SET_REDUCED_REFERENCE_SET, int)
#define AOM_CTRL_AV1E_SET_REDUCED_REFERENCE_SET

AOM_CTRL_USE_TYPE(AV1E_SET_COEFF_COST_UPD_FREQ, unsigned int)
#define AOM_CTRL_AV1E_SET_COEFF_COST_UPD_FREQ

AOM_CTRL_USE_TYPE(AV1E_SET_MODE_COST_UPD_FREQ, unsigned int)
#define AOM_CTRL_AV1E_SET_MODE_COST_UPD_FREQ

AOM_CTRL_USE_TYPE(AV1E_SET_TARGET_SEQ_LEVEL_IDX, int)
#define AOM_CTRL_AV1E_SET_TARGET_SEQ_LEVEL_IDX

AOM_CTRL_USE_TYPE(AV1E_SET_TIER_MASK, unsigned int)
#define AOM_CTRL_AV1E_SET_TIER_MASK

/*!\endcond */
/*! @} - end defgroup aom_encoder */
#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AOM_AOMCX_H_
