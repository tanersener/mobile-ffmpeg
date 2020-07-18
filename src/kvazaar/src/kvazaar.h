#ifndef KVAZAAR_H_
#define KVAZAAR_H_
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

/** 
 * \ingroup Control
 * \file
 * This file defines the public API of Kvazaar when used as a library.
 */

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

#if defined(KVZ_DLL_EXPORTS)
  #if !defined(PIC)
    // Building static kvazaar library.
    #define KVZ_PUBLIC
  #elif defined(_WIN32) || defined(__CYGWIN__)
    // Building kvazaar DLL on Windows.
    #define KVZ_PUBLIC __declspec(dllexport)
  #elif defined(__GNUC__)
    // Building kvazaar shared library with GCC.
    #define KVZ_PUBLIC __attribute__ ((visibility ("default")))
  #else
    #define KVZ_PUBLIC
  #endif
#else
  #if defined(KVZ_STATIC_LIB)
    // Using static kvazaar library.
    #define KVZ_PUBLIC
  #elif defined(_WIN32) || defined(__CYGWIN__)
    // Using kvazaar DLL on Windows.
    #define KVZ_PUBLIC __declspec(dllimport)
  #else
    // Using kvazaar shared library and not on Windows.
    #define KVZ_PUBLIC
  #endif
#endif

/**
 * Maximum length of a GoP structure.
 */
#define KVZ_MAX_GOP_LENGTH 32

 /**
 * Maximum amount of GoP layers.
 */
#define KVZ_MAX_GOP_LAYERS 6

/**
 * Size of data chunks.
 */
#define KVZ_DATA_CHUNK_SIZE 4096

#define KVZ_BIT_DEPTH 8
#if KVZ_BIT_DEPTH == 8
typedef uint8_t kvz_pixel;
#else
typedef uint16_t kvz_pixel;
#endif

/**
 * \brief Opaque data structure representing one instance of the encoder.
 */
typedef struct kvz_encoder kvz_encoder;

/**
 * \brief Integer motion estimation algorithms.
 */
enum kvz_ime_algorithm {
  KVZ_IME_HEXBS = 0,
  KVZ_IME_TZ = 1,
  KVZ_IME_FULL = 2,
  KVZ_IME_FULL8 = 3, //! \since 3.6.0
  KVZ_IME_FULL16 = 4, //! \since 3.6.0
  KVZ_IME_FULL32 = 5, //! \since 3.6.0
  KVZ_IME_FULL64 = 6, //! \since 3.6.0
  KVZ_IME_DIA = 7, // Experimental. TODO: change into a proper doc comment
};

/**
 * \brief Interlacing methods.
 * \since 3.2.0
 */
enum kvz_interlacing
{
  KVZ_INTERLACING_NONE = 0,
  KVZ_INTERLACING_TFF = 1, // top field first
  KVZ_INTERLACING_BFF = 2, // bottom field first
};

/**
* \brief Constrain movement vectors.
* \since 3.3.0
*/
enum kvz_mv_constraint
{
  KVZ_MV_CONSTRAIN_NONE = 0,
  KVZ_MV_CONSTRAIN_FRAME = 1,  // Don't refer outside the frame.
  KVZ_MV_CONSTRAIN_TILE = 2,  // Don't refer to other tiles.
  KVZ_MV_CONSTRAIN_FRAME_AND_TILE = 3,  // Don't refer outside the tile.
  KVZ_MV_CONSTRAIN_FRAME_AND_TILE_MARGIN = 4,  // Keep enough margin for fractional pixel margins not to refer outside the tile.
};

/**
* \brief Constrain movement vectors.
* \since 3.5.0
*/
enum kvz_hash
{
  KVZ_HASH_NONE = 0,
  KVZ_HASH_CHECKSUM = 1,
  KVZ_HASH_MD5 = 2,
};

/**
* \brief cu split termination mode
* \since since 3.8.0
*/
enum kvz_cu_split_termination
{
  KVZ_CU_SPLIT_TERMINATION_ZERO = 0,
  KVZ_CU_SPLIT_TERMINATION_OFF = 1
};

/**
* \brief Enable and disable crypto features.
* \since 3.7.0
*/
enum kvz_crypto_features {
  KVZ_CRYPTO_OFF = 0,
  KVZ_CRYPTO_MVs = (1 << 0),
  KVZ_CRYPTO_MV_SIGNS = (1 << 1),
  KVZ_CRYPTO_TRANSF_COEFFS = (1 << 2),
  KVZ_CRYPTO_TRANSF_COEFF_SIGNS = (1 << 3),
  KVZ_CRYPTO_INTRA_MODE = (1 << 4),
  KVZ_CRYPTO_ON = (1 << 5) - 1,
};

/**
* \brief me early termination mode
* \since since 3.8.0
*/
enum kvz_me_early_termination
{
  KVZ_ME_EARLY_TERMINATION_OFF = 0,
  KVZ_ME_EARLY_TERMINATION_ON = 1,
  KVZ_ME_EARLY_TERMINATION_SENSITIVE = 2
};


/**
 * \brief Format the pixels are read in.
 * This is separate from chroma subsampling, because we might want to read
 * interleaved formats in the future.
 * \since 3.12.0
 */
enum kvz_input_format {
  KVZ_FORMAT_P400 = 0,
  KVZ_FORMAT_P420 = 1,
  KVZ_FORMAT_P422 = 2,
  KVZ_FORMAT_P444 = 3,
};

/**
* \brief Chroma subsampling format used for encoding.
* \since 3.12.0
*/
enum kvz_chroma_format {
  KVZ_CSP_400 = 0,
  KVZ_CSP_420 = 1,
  KVZ_CSP_422 = 2,
  KVZ_CSP_444 = 3,
};

/**
 * \brief Chroma subsampling format used for encoding.
 * \since 3.15.0
 */
enum kvz_slices {
  KVZ_SLICES_NONE,
  KVZ_SLICES_TILES = (1 << 0), /*!< \brief Put each tile in a slice. */
  KVZ_SLICES_WPP   = (1 << 1), /*!< \brief Put each row in a slice. */
};

enum kvz_sao {
  KVZ_SAO_OFF = 0,
  KVZ_SAO_EDGE = 1,
  KVZ_SAO_BAND = 2,
  KVZ_SAO_FULL = 3
};

enum kvz_scalinglist {
  KVZ_SCALING_LIST_OFF = 0,
  KVZ_SCALING_LIST_CUSTOM = 1,
  KVZ_SCALING_LIST_DEFAULT = 2,  
};

enum kvz_rc_algorithm
{
  KVZ_NO_RC = 0,
  KVZ_LAMBDA = 1,
  KVZ_OBA = 2,
};
// Map from input format to chroma format.
#define KVZ_FORMAT2CSP(format) ((enum kvz_chroma_format)"\0\1\2\3"[format])

/**
 * \brief GoP picture configuration.
 */
typedef struct kvz_gop_config {
  double qp_factor;
  int8_t qp_offset;    /*!< \brief QP offset */
  int8_t poc_offset;   /*!< \brief POC offset */
  int8_t layer;        /*!< \brief Current layer */
  int8_t is_ref;       /*!< \brief Flag if this picture is used as a reference */
  int8_t ref_pos_count;/*!< \brief Reference picture count */
  int8_t ref_pos[16];  /*!< \brief reference picture offset list */
  int8_t ref_neg_count;/*!< \brief Reference picture count */
  int8_t ref_neg[16];  /*!< \brief reference picture offset list */
  double qp_model_offset;
  double qp_model_scale;
} kvz_gop_config;

/**
 * \brief Struct which contains all configuration data
 *
 * Functions config_alloc, config_init and config_destroy must be used to
 * maintain ABI compatibility. Do not copy this struct, as the size might
 * change.
 */
typedef struct kvz_config
{
  int32_t qp;        /*!< \brief Quantization parameter */
  int32_t intra_period; /*!< \brief the period of intra frames in stream */

  /** \brief How often the VPS, SPS and PPS are re-sent
   *
   * -1: never
   *  0: first frame only
   *  1: every intra frame
   *  2: every other intra frame
   *  3: every third intra frame
   *  and so on
   */
  int32_t vps_period;

  int32_t width;   /*!< \brief frame width, must be a multiple of 8 */
  int32_t height;  /*!< \brief frame height, must be a multiple of 8 */
  double framerate; /*!< \brief Deprecated, will be removed. */
  int32_t framerate_num; /*!< \brief Framerate numerator */
  int32_t framerate_denom; /*!< \brief Framerate denominator */
  int32_t deblock_enable; /*!< \brief Flag to enable deblocking filter */
  enum kvz_sao sao_type;     /*!< \brief Flag to enable sample adaptive offset filter */
  int32_t rdoq_enable;    /*!< \brief Flag to enable RD optimized quantization. */
  int32_t signhide_enable;   /*!< \brief Flag to enable sign hiding. */
  int32_t smp_enable;   /*!< \brief Flag to enable SMP blocks. */
  int32_t amp_enable;   /*!< \brief Flag to enable AMP blocks. */
  int32_t rdo;            /*!< \brief RD-calculation level (0..2) */
  int32_t full_intra_search; /*!< \brief If true, don't skip modes in intra search. */
  int32_t trskip_enable;    /*!< \brief Flag to enable transform skip (for 4x4 blocks). */
  int32_t tr_depth_intra; /*!< \brief Maximum transform depth for intra. */
  enum kvz_ime_algorithm ime_algorithm;  /*!< \brief Integer motion estimation algorithm. */
  int32_t fme_level;      /*!< \brief Fractional pixel motion estimation level (0: disabled, 1: enabled). */
  int8_t source_scan_type; /*!< \brief Source scan type (0: progressive, 1: top field first, 2: bottom field first).*/
  int32_t bipred;         /*!< \brief Bi-prediction (0: disabled, 1: enabled). */
  int32_t deblock_beta;   /*!< \brief (deblocking) beta offset (div 2), range -6...6 */
  int32_t deblock_tc;     /*!< \brief (deblocking) tc offset (div 2), range -6...6 */
  struct
  {
    int32_t sar_width;   /*!< \brief the horizontal size of the sample aspect ratio (in arbitrary units) */
    int32_t sar_height;  /*!< \brief the vertical size of the sample aspect ratio (in the same arbitrary units as sar_width). */
    int8_t overscan;     /*!< \brief Crop overscan setting */
    int8_t videoformat;  /*!< \brief Video format */
    int8_t fullrange;    /*!< \brief Flag to indicate full-range */
    int8_t colorprim;    /*!< \brief Color primaries */
    int8_t transfer;     /*!< \brief Transfer characteristics */
    int8_t colormatrix;  /*!< \brief Color matrix coefficients */
    int32_t chroma_loc;   /*!< \brief Chroma sample location */
  } vui;
  int32_t aud_enable;     /*!< \brief Flag to use access unit delimiters */
  int32_t ref_frames;     /*!< \brief number of reference frames to use */
  char * cqmfile;        /*!< \brief Pointer to custom quantization matrices filename */

  int32_t tiles_width_count;      /*!< \brief number of tiles separation in x direction */
  int32_t tiles_height_count;      /*!< \brief number of tiles separation in y direction */
  int32_t* tiles_width_split;      /*!< \brief tiles split x coordinates (dimension: tiles_width_count) */
  int32_t* tiles_height_split;      /*!< \brief tiles split y coordinates (dimension: tiles_height_count) */

  int wpp;
  int owf;

  int32_t slice_count;
  int32_t* slice_addresses_in_ts;

  int32_t threads;
  int32_t cpuid;

  struct {
    int32_t min[KVZ_MAX_GOP_LAYERS];
    int32_t max[KVZ_MAX_GOP_LAYERS];
  } pu_depth_inter, pu_depth_intra;

  int32_t add_encoder_info;
  int8_t gop_len;            /*!< \brief length of GOP for the video sequence */
  int8_t gop_lowdelay;       /*!< \brief specifies that the GOP does not use future pictures */
  kvz_gop_config gop[KVZ_MAX_GOP_LENGTH];  /*!< \brief Array of GOP settings */

  int32_t target_bitrate;

  int8_t mv_rdo;            /*!< \brief MV RDO calculation in search (0: estimation, 1: RDO). */
  int8_t calc_psnr;         /*!< \since 3.1.0 \brief Print PSNR in CLI. */

  enum kvz_mv_constraint mv_constraint;  /*!< \since 3.3.0 \brief Constrain movement vectors. */
  enum kvz_hash hash;  /*!< \since 3.5.0 \brief What hash algorithm to use. */

  enum kvz_cu_split_termination cu_split_termination; /*!< \since 3.8.0 \brief Mode of cu split termination. */

  enum kvz_crypto_features crypto_features; /*!< \since 3.7.0 */
  uint8_t *optional_key;

  enum kvz_me_early_termination me_early_termination; /*!< \since 3.8.0 \brief Mode of me early termination. */
  int32_t intra_rdo_et; /*!< \since 4.1.0 \brief Use early termination in intra rdo. */

  int32_t lossless; /*!< \brief Use lossless coding. */

  int32_t tmvp_enable; /*!> \brief Use Temporal Motion Vector Predictors. */

  int32_t rdoq_skip; /*!< \brief Mode of rdoq skip */

  enum kvz_input_format input_format; /*!< \brief Use Temporal Motion Vector Predictors. */
  int32_t input_bitdepth; /*!< \brief Use Temporal Motion Vector Predictors. */

  struct {
    unsigned d;  // depth
    unsigned t;  // temporal
  } gop_lp_definition;

  int32_t implicit_rdpcm; /*!< \brief Enable implicit residual DPCM. */

  struct {
    int32_t width;
    int32_t height;
    int8_t *dqps;
  } roi; /*!< \since 3.14.0 \brief Map of delta QPs for region of interest coding. */

  unsigned slices; /*!< \since 3.15.0 \brief How to map slices to frame. */

  /**
   * \brief Use adaptive QP for 360 video with equirectangular projection.
   */
  int32_t erp_aqp;

  /** \brief The HEVC level */
  uint8_t level;
  /** \brief Whether we ignore and just warn from all of the errors about the output not conforming to the level's requirements. */
  uint8_t force_level;
  /** \brief Whether we use the high tier bitrates. Requires the level to be 4 or higher. */
  uint8_t high_tier;
  /** \brief The maximum allowed bitrate for this level and tier. */
  uint32_t max_bitrate;

  /** \brief Maximum steps that hexagonal and diagonal motion estimation can use. -1 to disable */
  uint32_t me_max_steps;

  /** \brief Offset to add to QP for intra frames */
  int8_t intra_qp_offset;
  /** \brief Select intra QP Offset based on GOP length */
  uint8_t intra_qp_offset_auto;

  /** \brief Minimum QP that uses CABAC for residual cost instead of a fast estimate. */
  int8_t fast_residual_cost_limit;

  /** \brief Set QP at CU level keeping pic_init_qp_minus26 in PPS zero */
  int8_t set_qp_in_cu;

  /** \brief Flag to enable/disable open GOP configuration */
  int8_t open_gop;

	int32_t vaq; /** \brief Enable variance adaptive quantization*/

  /** \brief Type of scaling lists to use */
  int8_t scaling_list;

  /** \brief Maximum number of merge cadidates */
  uint8_t max_merge;

  /** \brief Enable Early Skip Mode Decision */
  uint8_t early_skip;

  /** \brief Enable Machine learning CU depth prediction for Intra encoding. */
  uint8_t ml_pu_depth_intra;  
  
  /** \brief Used for partial frame encoding*/
  struct {
    uint8_t startCTU_x;
    uint8_t startCTU_y;
    uint16_t fullWidth;
    uint16_t fullHeight;
  } partial_coding;

  /** \brief Always consider CU without any quantized residual */
  uint8_t zero_coeff_rdo;

  /** \brief Currently unused parameter for OBA rc */
  int8_t frame_allocation;

  /** \brief used rc scheme, 0 for QP */
  int8_t rc_algorithm;

  /** \brief whether to use hadamard based bit allocation for intra frames or not */
  uint8_t intra_bit_allocation;

  uint8_t clip_neighbour;
} kvz_config;

/**
 * \brief Struct which contains all picture data
 *
 * Function picture_alloc in kvz_api must be used for allocation.
 */
typedef struct kvz_picture {
  kvz_pixel *fulldata_buf;     //!< \brief Allocated buffer with padding (only used in the base_image)
  kvz_pixel *fulldata;         //!< \brief Allocated buffer portion that's actually used

  kvz_pixel *y;                //!< \brief Pointer to luma pixel array.
  kvz_pixel *u;                //!< \brief Pointer to chroma U pixel array.
  kvz_pixel *v;                //!< \brief Pointer to chroma V pixel array.
  kvz_pixel *data[3]; //!< \brief Alternate access method to same data.

  int32_t width;           //!< \brief Luma pixel array width.
  int32_t height;          //!< \brief Luma pixel array height.

  int32_t stride;          //!< \brief Luma pixel array width for the full picture (should be used as stride)

  struct kvz_picture *base_image; //!< \brief Pointer to the picture which owns the pixels
  int32_t refcount;        //!< \brief Number of references to the picture

  int64_t pts;             //!< \brief Presentation timestamp. Should be set for input frames.
  int64_t dts;             //!< \brief Decompression timestamp.

  enum kvz_interlacing interlacing; //!< \since 3.2.0 \brief Field order for interlaced pictures.
  enum kvz_chroma_format chroma_format;

  int32_t ref_pocs[16];
} kvz_picture;

/**
 * \brief NAL unit type codes.
 *
 * These are the nal_unit_type codes from Table 7-1 ITU-T H.265 v1.0.
 */
enum kvz_nal_unit_type {

  // Trailing pictures

  KVZ_NAL_TRAIL_N = 0,
  KVZ_NAL_TRAIL_R = 1,

  KVZ_NAL_TSA_N = 2,
  KVZ_NAL_TSA_R = 3,

  KVZ_NAL_STSA_N = 4,
  KVZ_NAL_STSA_R = 5,

  // Leading pictures

  KVZ_NAL_RADL_N = 6,
  KVZ_NAL_RADL_R = 7,

  KVZ_NAL_RASL_N = 8,
  KVZ_NAL_RASL_R = 9,

  // Reserved non-IRAP RSV_VCL_N/R 10-15

  // Intra random access point pictures

  KVZ_NAL_BLA_W_LP   = 16,
  KVZ_NAL_BLA_W_RADL = 17,
  KVZ_NAL_BLA_N_LP   = 18,

  KVZ_NAL_IDR_W_RADL = 19,
  KVZ_NAL_IDR_N_LP   = 20,

  KVZ_NAL_CRA_NUT    = 21,

  // Reserved IRAP

  KVZ_NAL_RSV_IRAP_VCL22 = 22,
  KVZ_NAL_RSV_IRAP_VCL23 = 23,

  // Reserved non-IRAP RSV_VCL 24-32

  // non-VCL

  KVZ_NAL_VPS_NUT = 32,
  KVZ_NAL_SPS_NUT = 33,
  KVZ_NAL_PPS_NUT = 34,

  KVZ_NAL_AUD_NUT = 35,
  KVZ_NAL_EOS_NUT = 36,
  KVZ_NAL_EOB_NUT = 37,
  KVZ_NAL_FD_NUT  = 38,

  KVZ_NAL_PREFIX_SEI_NUT = 39,
  KVZ_NAL_SUFFIX_SEI_NUT = 40,

  // Reserved RSV_NVCL 41-47
  // Unspecified UNSPEC 48-63
};

enum kvz_slice_type {
  KVZ_SLICE_B = 0,
  KVZ_SLICE_P = 1,
  KVZ_SLICE_I = 2,
};

/**
 * \brief Other information about an encoded frame
 */
typedef struct kvz_frame_info {

  /**
   * \brief Picture order count
   */
  int32_t poc;

  /**
   * \brief Quantization parameter
   */
  int8_t qp;

  /**
   * \brief Type of the NAL VCL unit
   */
  enum kvz_nal_unit_type nal_unit_type;

  /**
   * \brief Type of the slice
   */
  enum kvz_slice_type slice_type;

  /**
   * \brief Reference picture lists
   *
   * The first list contains the reference picture POCs that are less than the
   * POC of this frame and the second one contains those that are greater.
   */
  int ref_list[2][16];

  /**
   * \brief Lengths of the reference picture lists
   */
  int ref_list_len[2];

} kvz_frame_info;

/**
 * \brief A linked list of chunks of data.
 *
 * Used for returning the encoded data.
 */
typedef struct kvz_data_chunk {
  /// \brief Buffer for the data.
  uint8_t data[KVZ_DATA_CHUNK_SIZE];

  /// \brief Number of bytes filled in this chunk.
  uint32_t len;

  /// \brief Next chunk in the list.
  struct kvz_data_chunk *next;
} kvz_data_chunk;

typedef struct kvz_api {

  /**
   * \brief Allocate a kvz_config structure.
   *
   * The returned structure should be deallocated by calling config_destroy.
   *
   * \return allocated config, or NULL if allocation failed.
   */
  kvz_config *  (*config_alloc)(void);

  /**
   * \brief Deallocate a kvz_config structure.
   *
   * If cfg is NULL, do nothing. Otherwise, the given structure must have been
   * returned from config_alloc.
   *
   * \param cfg   configuration
   * \return      1 on success, 0 on failure
   */
  int           (*config_destroy)(kvz_config *cfg);

  /**
   * \brief Initialize a config structure
   *
   * Set all fields in the given config to default values.
   *
   * \param cfg   configuration
   * \return      1 on success, 0 on failure
   */
  int           (*config_init)(kvz_config *cfg);

  /**
   * \brief Set an option.
   *
   * \param cfg   configuration
   * \param name  name of the option to set
   * \param value value to set
   * \return      1 on success, 0 on failure
   */
  int           (*config_parse)(kvz_config *cfg, const char *name, const char *value);

  /**
   * \brief Allocate a kvz_picture.
   *
   * The returned kvz_picture should be deallocated by calling picture_free.
   *
   * \param width   width of luma pixel array to allocate
   * \param height  height of luma pixel array to allocate
   * \return        allocated picture, or NULL if allocation failed.
   */
  kvz_picture * (*picture_alloc)(int32_t width, int32_t height);

  /**
   * \brief Deallocate a kvz_picture.
   *
   * If pic is NULL, do nothing. Otherwise, the picture must have been returned
   * from picture_alloc.
   */
  void          (*picture_free)(kvz_picture *pic);

  /**
   * \brief Deallocate a list of data chunks.
   *
   * Deallocates the given chunk and all chunks that follow it in the linked
   * list.
   */
  void          (*chunk_free)(kvz_data_chunk *chunk);

  /**
   * \brief Create an encoder.
   *
   * The returned encoder should be closed by calling encoder_close.
   *
   * Only one encoder may be open at a time.
   *
   * \param cfg   encoder configuration
   * \return      created encoder, or NULL if creation failed.
   */
  kvz_encoder * (*encoder_open)(const kvz_config *cfg);

  /**
   * \brief Deallocate an encoder.
   *
   * If encoder is NULL, do nothing. Otherwise, the encoder must have been
   * returned from encoder_open.
   */
  void          (*encoder_close)(kvz_encoder *encoder);

  /**
   * \brief Get parameter sets.
   *
   * Encode the VPS, SPS and PPS.
   *
   * If data_out is set to non-NULL values, the caller is responsible for
   * calling chunk_free on it.
   *
   * A null pointer may be passed in place of the parameter data_out or len_out
   * to skip returning the corresponding value.
   *
   * \param encoder   encoder
   * \param data_out  Returns the encoded parameter sets.
   * \param len_out   Returns number of bytes in the encoded data.
   * \return          1 on success, 0 on error.
   */
  int           (*encoder_headers)(kvz_encoder *encoder,
                                   kvz_data_chunk **data_out,
                                   uint32_t *len_out);

  /**
   * \brief Encode one frame.
   *
   * Add pic_in to the encoding pipeline. If an encoded frame is ready, return
   * the bitstream, length of the bitstream, the reconstructed frame, the
   * original frame and frame info in data_out, len_out, pic_out, src_out and
   * info_out, respectively. Otherwise, set the output parameters to NULL.
   *
   * After passing all of the input frames, the caller should keep calling this
   * function with pic_in set to NULL, until no more data is returned in the
   * output parameters.
   *
   * The caller must not modify pic_in after passing it to this function.
   *
   * If data_out, pic_out and src_out are set to non-NULL values, the caller is
   * responsible for calling chunk_free and picture_free on them.
   *
   * A null pointer may be passed in place of any of the parameters data_out,
   * len_out, pic_out, src_out or info_out to skip returning the corresponding
   * value.
   *
   * \param encoder   encoder
   * \param pic_in    input frame or NULL
   * \param data_out  Returns the encoded data.
   * \param len_out   Returns number of bytes in the encoded data.
   * \param pic_out   Returns the reconstructed picture.
   * \param src_out   Returns the original picture.
   * \param info_out  Returns information about the encoded picture.
   * \return          1 on success, 0 on error.
   */
  int           (*encoder_encode)(kvz_encoder *encoder,
                                  kvz_picture *pic_in,
                                  kvz_data_chunk **data_out,
                                  uint32_t *len_out,
                                  kvz_picture **pic_out,
                                  kvz_picture **src_out,
                                  kvz_frame_info *info_out);

  /**
   * \brief Allocate a kvz_picture.
   *
   * The returned kvz_picture should be deallocated by calling picture_free.
   *
   * \since 3.12.0
   * \param chroma_fomat  Chroma subsampling to use.
   * \param width   width of luma pixel array to allocate
   * \param height  height of luma pixel array to allocate
   * \return        allocated picture, or NULL if allocation failed.
   */
  kvz_picture * (*picture_alloc_csp)(enum kvz_chroma_format chroma_fomat, int32_t width, int32_t height);
} kvz_api;


KVZ_PUBLIC const kvz_api * kvz_api_get(int bit_depth);

#ifdef __cplusplus
}
#endif

#endif // KVAZAAR_H_
