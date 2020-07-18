////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2019 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wavpack_local.h

#ifndef WAVPACK_LOCAL_H
#define WAVPACK_LOCAL_H

#include "wavpack.h"

#if defined(_WIN32)
#define strdup(x) _strdup(x)
#define FASTCALL __fastcall
#else
#define FASTCALL
#endif

#if defined(_WIN32) || \
    (defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && (BYTE_ORDER == LITTLE_ENDIAN)) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
#define BITSTREAM_SHORTS    // use 16-bit "shorts" for reading/writing bitstreams (instead of chars)
                            //  (only works on little-endian machines)
#endif

#include <sys/types.h>

// This header file contains all the definitions required by WavPack.

#if defined(_MSC_VER) && _MSC_VER < 1600
#include <stdlib.h>
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8  int8_t;
#else
#include <stdint.h>
#endif

// Because the C99 specification states that "The order of allocation of
// bit-fields within a unit (high-order to low-order or low-order to
// high-order) is implementation-defined" (6.7.2.1), I decided to change
// the representation of floating-point values from a structure of
// bit-fields to a 32-bit integer with access macros. Note that the WavPack
// library doesn't use any floating-point math to implement compression of
// floating-point data (although a little floating-point math is used in
// high-level functions unrelated to the codec).

typedef int32_t f32;

#define get_mantissa(f)     ((f) & 0x7fffff)
#define get_magnitude(f)    ((f) & 0x7fffffff)
#define get_exponent(f)     (((f) >> 23) & 0xff)
#define get_sign(f)         (((f) >> 31) & 0x1)

#define set_mantissa(f,v)   (f) ^= (((f) ^ (v)) & 0x7fffff)
#define set_exponent(f,v)   (f) ^= (((f) ^ ((uint32_t)(v) << 23)) & 0x7f800000)
#define set_sign(f,v)       (f) ^= (((f) ^ ((uint32_t)(v) << 31)) & 0x80000000)

#include <stdio.h>

#define FALSE 0
#define TRUE 1

// ID3v1 and APEv2 TAG formats (may occur at the end of WavPack files)

typedef struct {
    char tag_id [3], title [30], artist [30], album [30];
    char year [4], comment [30], genre;
} ID3_Tag;

typedef struct {
    char ID [8];
    int32_t version, length, item_count, flags;
    char res [8];
} APE_Tag_Hdr;

#define APE_Tag_Hdr_Format "8LLLL"

#define APE_TAG_TYPE_TEXT       0x0
#define APE_TAG_TYPE_BINARY     0x1
#define APE_TAG_THIS_IS_HEADER  0x20000000
#define APE_TAG_CONTAINS_HEADER 0x80000000
#define APE_TAG_MAX_LENGTH      (1024 * 1024 * 16)

typedef struct {
    int64_t tag_file_pos;
    int tag_begins_file;
    ID3_Tag id3_tag;
    APE_Tag_Hdr ape_tag_hdr;
    unsigned char *ape_tag_data;
} M_Tag;

// or-values for "flags"

#define CUR_STREAM_VERS     0x407       // universally compatible stream version


//////////////////////////// WavPack Metadata /////////////////////////////////

// This is an internal representation of metadata.

typedef struct {
    int32_t byte_length;
    void *data;
    unsigned char id;
} WavpackMetadata;


///////////////////////// WavPack Configuration ///////////////////////////////

/*
 * These config flags are not actually used for external configuration, which is
 * why they're not in the external wavpack.h file, but they are used internally
 * in the flags field of the WavpackConfig struct.
 */

#define CONFIG_MONO_FLAG        4       // not stereo
#define CONFIG_FLOAT_DATA       0x80    // ieee 32-bit floating point data

#define CONFIG_AUTO_SHAPING     0x4000  // automatic noise shaping
#define CONFIG_LOSSY_MODE       0x1000000 // obsolete (for information)

/*
 * These config flags were never actually used, or are no longer used, or are
 * used for something else now. They may be used in the future for what they
 * say, or for something else. WavPack files in the wild *may* have some of
 * these bit set in their config flags (with these older meanings), but only
 * if the stream version is 0x410 or less than 0x407. Of course, this is not
 * very important because once the file has been encoded, the config bits are
 * just for information purposes (i.e., they do not affect decoding),
 *
#define CONFIG_ADOBE_MODE       0x100   // "adobe" mode for 32-bit floats
#define CONFIG_VERY_FAST_FLAG   0x400   // double fast
#define CONFIG_COPY_TIME        0x20000 // copy file-time from source
#define CONFIG_QUALITY_MODE     0x200000 // psychoacoustic quality mode
#define CONFIG_RAW_FLAG         0x400000 // raw mode (not implemented yet)
#define CONFIG_QUIET_MODE       0x10000000 // don't report progress %
#define CONFIG_IGNORE_LENGTH    0x20000000 // ignore length in wav header
#define CONFIG_NEW_RIFF_HEADER  0x40000000 // generate new RIFF wav header
 *
 */

#define EXTRA_SCAN_ONLY         1
#define EXTRA_STEREO_MODES      2
#define EXTRA_TRY_DELTAS        8
#define EXTRA_ADJUST_DELTAS     16
#define EXTRA_SORT_FIRST        32
#define EXTRA_BRANCHES          0x1c0
#define EXTRA_SKIP_8TO16        512
#define EXTRA_TERMS             0x3c00
#define EXTRA_DUMP_TERMS        16384
#define EXTRA_SORT_LAST         32768

//////////////////////////////// WavPack Stream ///////////////////////////////

// This internal structure contains everything required to handle a WavPack
// "stream", which is defined as a stereo or mono stream of audio samples. For
// multichannel audio several of these would be required. Each stream contains
// pointers to hold a complete allocated block of WavPack data, although it's
// possible to decode WavPack blocks without buffering an entire block.

typedef struct bs {
#ifdef BITSTREAM_SHORTS
    uint16_t *buf, *end, *ptr;
#else
    unsigned char *buf, *end, *ptr;
#endif
    void (*wrap)(struct bs *bs);
    int error, bc;
    uint32_t sr;
} Bitstream;

#define MAX_WRAPPER_BYTES 16777216
#define NEW_MAX_STREAMS 4096
#define OLD_MAX_STREAMS 8
#define MAX_NTERMS 16
#define MAX_TERM 8

// DSD-specific definitions

#define MAX_HISTORY_BITS    5       // maximum number of history bits in DSD "fast" mode
                                    // note that 5 history bits requires 32 history bins
#define MAX_BYTES_PER_BIN   1280    // maximum bytes for the value lookup array (per bin)
                                    //  such that the total storage per bin = 2K (also
                                    //  counting probabilities and summed_probabilities)

// Note that this structure is directly accessed in assembly files, so modify with care

struct decorr_pass {
    int32_t term, delta, weight_A, weight_B;
    int32_t samples_A [MAX_TERM], samples_B [MAX_TERM];
    int32_t aweight_A, aweight_B;
    int32_t sum_A, sum_B;
};

typedef struct {
    signed char joint_stereo, delta, terms [MAX_NTERMS+1];
} WavpackDecorrSpec;

struct entropy_data {
    uint32_t median [3], slow_level, error_limit;
};

struct words_data {
    uint32_t bitrate_delta [2], bitrate_acc [2];
    uint32_t pend_data, holding_one, zeros_acc;
    int holding_zero, pend_count;
    struct entropy_data c [2];
};

typedef struct {
    int32_t value, filter0, filter1, filter2, filter3, filter4, filter5, filter6, factor;
    unsigned int byte;
} DSDfilters;

typedef struct {
    WavpackHeader wphdr;
    struct words_data w;

    unsigned char *blockbuff, *blockend;
    unsigned char *block2buff, *block2end;
    int32_t *sample_buffer;

    int64_t sample_index;
    int bits, num_terms, mute_error, joint_stereo, false_stereo, shift;
    int num_decorrs, num_passes, best_decorr, mask_decorr;
    uint32_t crc, crc_x, crc_wvx;
    Bitstream wvbits, wvcbits, wvxbits;
    int init_done, wvc_skip;
    float delta_decay;

    unsigned char int32_sent_bits, int32_zeros, int32_ones, int32_dups;
    unsigned char float_flags, float_shift, float_max_exp, float_norm_exp;

    struct {
        int32_t shaping_acc [2], shaping_delta [2], error [2];
        double noise_sum, noise_ave, noise_max;
        int16_t *shaping_data, *shaping_array;
        int32_t shaping_samples;
    } dc;

    struct decorr_pass decorr_passes [MAX_NTERMS], analysis_pass;
    const WavpackDecorrSpec *decorr_specs;

    struct {
        unsigned char *byteptr, *endptr, (*probabilities) [256], *lookup_buffer, **value_lookup, mode, ready;
        int history_bins, p0, p1;
        uint16_t (*summed_probabilities) [256];
        uint32_t low, high, value;
        DSDfilters filters [2];
        int32_t *ptable;
    } dsd;

} WavpackStream;

// flags for float_flags:

#define FLOAT_SHIFT_ONES 1      // bits left-shifted into float = '1'
#define FLOAT_SHIFT_SAME 2      // bits left-shifted into float are the same
#define FLOAT_SHIFT_SENT 4      // bits shifted into float are sent literally
#define FLOAT_ZEROS_SENT 8      // "zeros" are not all real zeros
#define FLOAT_NEG_ZEROS  0x10   // contains negative zeros
#define FLOAT_EXCEPTIONS 0x20   // contains exceptions (inf, nan, etc.)

/////////////////////////////// WavPack Context ///////////////////////////////

// This internal structure holds everything required to encode or decode WavPack
// files. It is recommended that direct access to this structure be minimized
// and the provided utilities used instead.

struct WavpackContext {
    WavpackConfig config;

    WavpackMetadata *metadata;
    uint32_t metabytes;
    int metacount;

    unsigned char *wrapper_data;
    uint32_t wrapper_bytes;

    WavpackBlockOutput blockout;
    void *wv_out, *wvc_out;

    WavpackStreamReader64 *reader;
    void *wv_in, *wvc_in;

    int64_t filelen, file2len, filepos, file2pos, total_samples, initial_index;
    uint32_t crc_errors, first_flags;
    int wvc_flag, open_flags, norm_offset, reduced_channels, lossy_blocks, version_five;
    uint32_t block_samples, ave_block_samples, block_boundary, max_samples, acc_samples, riff_trailer_bytes;
    int riff_header_added, riff_header_created;
    M_Tag m_tag;

    int current_stream, num_streams, max_streams, stream_version;
    WavpackStream **streams;
    void *stream3;

    // these items were added in 5.0 to support alternate file types (especially CAF & DSD)
    unsigned char file_format, *channel_reordering, *channel_identities;
    uint32_t channel_layout, dsd_multiplier;
    void *decimation_context;
    char file_extension [8];

    void (*close_callback)(void *wpc);
    char error_message [80];
};

//////////////////////// function prototypes and macros //////////////////////

#define CLEAR(destin) memset (&destin, 0, sizeof (destin));

//////////////////////////////// decorrelation //////////////////////////////
// modules: pack.c, unpack.c, unpack_floats.c, extra1.c, extra2.c

// #define SKIP_DECORRELATION   // experimental switch to disable all decorrelation on encode

// These macros implement the weight application and update operations
// that are at the heart of the decorrelation loops. Note that there are
// sometimes two and even three versions of each macro. Theses should be
// equivalent and produce identical results, but some may perform better
// or worse on a given architecture.

#if 1   // PERFCOND - apply decorrelation weight when no 32-bit overflow possible
#define apply_weight_i(weight, sample) ((weight * sample + 512) >> 10)
#else
#define apply_weight_i(weight, sample) ((((weight * sample) >> 8) + 2) >> 2)
#endif

#if 1   // PERFCOND - apply decorrelation weight when 32-bit overflow is possible
#define apply_weight_f(weight, sample) (((((sample & 0xffff) * weight) >> 9) + \
    (((sample & ~0xffff) >> 9) * weight) + 1) >> 1)
#elif 1
#define apply_weight_f(weight, sample) ((int32_t)((weight * (int64_t) sample + 512) >> 10))
#else
#define apply_weight_f(weight, sample) ((int32_t)floor(((double) weight * sample + 512.0) / 1024.0))
#endif

#if 1   // PERFCOND - universal version that checks input magnitude or always uses long version
#define apply_weight(weight, sample) (sample != (int16_t) sample ? \
    apply_weight_f (weight, sample) : apply_weight_i (weight, sample))
#else
#define apply_weight(weight, sample) (apply_weight_f (weight, sample))
#endif

#if 1   // PERFCOND
#define update_weight(weight, delta, source, result) \
    if (source && result) { int32_t s = (int32_t) (source ^ result) >> 31; weight = (delta ^ s) + (weight - s); }
#elif 1
#define update_weight(weight, delta, source, result) \
    if (source && result) weight += (((source ^ result) >> 30) | 1) * delta;
#else
#define update_weight(weight, delta, source, result) \
    if (source && result) (source ^ result) < 0 ? (weight -= delta) : (weight += delta);
#endif

#define update_weight_clip(weight, delta, source, result) \
    if (source && result) { \
        const int32_t s = (source ^ result) >> 31; \
        if ((weight = (weight ^ s) + (delta - s)) > 1024) weight = 1024; \
        weight = (weight ^ s) - s; \
    }

void pack_init (WavpackContext *wpc);
int pack_block (WavpackContext *wpc, int32_t *buffer);
void send_general_metadata (WavpackContext *wpc);
void free_metadata (WavpackMetadata *wpmd);
int copy_metadata (WavpackMetadata *wpmd, unsigned char *buffer_start, unsigned char *buffer_end);
double WavpackGetEncodedNoise (WavpackContext *wpc, double *peak);
int unpack_init (WavpackContext *wpc);
int read_decorr_terms (WavpackStream *wps, WavpackMetadata *wpmd);
int read_decorr_weights (WavpackStream *wps, WavpackMetadata *wpmd);
int read_decorr_samples (WavpackStream *wps, WavpackMetadata *wpmd);
int read_shaping_info (WavpackStream *wps, WavpackMetadata *wpmd);
int32_t unpack_samples (WavpackContext *wpc, int32_t *buffer, uint32_t sample_count);
int check_crc_error (WavpackContext *wpc);
int scan_float_data (WavpackStream *wps, f32 *values, int32_t num_values);
void send_float_data (WavpackStream *wps, f32 *values, int32_t num_values);
void float_values (WavpackStream *wps, int32_t *values, int32_t num_values);
void dynamic_noise_shaping (WavpackContext *wpc, int32_t *buffer, int shortening_allowed);
void execute_stereo (WavpackContext *wpc, int32_t *samples, int no_history, int do_samples);
void execute_mono (WavpackContext *wpc, int32_t *samples, int no_history, int do_samples);

////////////////////////// DSD related (including decimation) //////////////////////////
// modules: pack_dsd.c unpack_dsd.c

void pack_dsd_init (WavpackContext *wpc);
int pack_dsd_block (WavpackContext *wpc, int32_t *buffer);
int init_dsd_block (WavpackContext *wpc, WavpackMetadata *wpmd);
int32_t unpack_dsd_samples (WavpackContext *wpc, int32_t *buffer, uint32_t sample_count);

void *decimate_dsd_init (int num_channels);
void decimate_dsd_reset (void *decimate_context);
void decimate_dsd_run (void *decimate_context, int32_t *samples, int num_samples);
void decimate_dsd_destroy (void *decimate_context);

///////////////////////////////// CPU feature detection ////////////////////////////////

int unpack_cpu_has_feature_x86 (int findex), pack_cpu_has_feature_x86 (int findex);

#define CPU_FEATURE_MMX     23

///////////////////////////// pre-4.0 version decoding ////////////////////////////
// modules: unpack3.c, unpack3_open.c, unpack3_seek.c

WavpackContext *open_file3 (WavpackContext *wpc, char *error);
int32_t unpack_samples3 (WavpackContext *wpc, int32_t *buffer, uint32_t sample_count);
int seek_sample3 (WavpackContext *wpc, uint32_t desired_index);
uint32_t get_sample_index3 (WavpackContext *wpc);
void free_stream3 (WavpackContext *wpc);
int get_version3 (WavpackContext *wpc);

////////////////////////////// bitstream macros & functions /////////////////////////////

#define bs_is_open(bs) ((bs)->ptr != NULL)
uint32_t bs_close_read (Bitstream *bs);

#define getbit(bs) ( \
    (((bs)->bc) ? \
        ((bs)->bc--, (bs)->sr & 1) : \
            (((++((bs)->ptr) != (bs)->end) ? (void) 0 : (bs)->wrap (bs)), (bs)->bc = sizeof (*((bs)->ptr)) * 8 - 1, ((bs)->sr = *((bs)->ptr)) & 1) \
    ) ? \
        ((bs)->sr >>= 1, 1) : \
        ((bs)->sr >>= 1, 0) \
)

#define getbits(value, nbits, bs) do { \
    while ((nbits) > (bs)->bc) { \
        if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
        (bs)->sr |= (uint32_t)*((bs)->ptr) << (bs)->bc; \
        (bs)->bc += sizeof (*((bs)->ptr)) * 8; \
    } \
    *(value) = (bs)->sr; \
    if ((bs)->bc > 32) { \
        (bs)->bc -= (nbits); \
        (bs)->sr = *((bs)->ptr) >> (sizeof (*((bs)->ptr)) * 8 - (bs)->bc); \
    } \
    else { \
        (bs)->bc -= (nbits); \
        (bs)->sr >>= (nbits); \
    } \
} while (0)

#define putbit(bit, bs) do { if (bit) (bs)->sr |= (1U << (bs)->bc); \
    if (++((bs)->bc) == sizeof (*((bs)->ptr)) * 8) { \
        *((bs)->ptr) = (bs)->sr; \
        (bs)->sr = (bs)->bc = 0; \
        if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
    }} while (0)

#define putbit_0(bs) do { \
    if (++((bs)->bc) == sizeof (*((bs)->ptr)) * 8) { \
        *((bs)->ptr) = (bs)->sr; \
        (bs)->sr = (bs)->bc = 0; \
        if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
    }} while (0)

#define putbit_1(bs) do { (bs)->sr |= (1U << (bs)->bc); \
    if (++((bs)->bc) == sizeof (*((bs)->ptr)) * 8) { \
        *((bs)->ptr) = (bs)->sr; \
        (bs)->sr = (bs)->bc = 0; \
        if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
    }} while (0)

#define putbits(value, nbits, bs) do { \
    (bs)->sr |= (uint32_t)(value) << (bs)->bc; \
    if (((bs)->bc += (nbits)) >= sizeof (*((bs)->ptr)) * 8) \
        do { \
            *((bs)->ptr) = (bs)->sr; \
            (bs)->sr >>= sizeof (*((bs)->ptr)) * 8; \
            if (((bs)->bc -= sizeof (*((bs)->ptr)) * 8) > 32 - sizeof (*((bs)->ptr)) * 8) \
                (bs)->sr |= ((value) >> ((nbits) - (bs)->bc)); \
            if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
        } while ((bs)->bc >= sizeof (*((bs)->ptr)) * 8); \
} while (0)

///////////////////////////// entropy encoder / decoder ////////////////////////////
// modules: entropy_utils.c, read_words.c, write_words.c

// these control the time constant "slow_level" which is used for hybrid mode
// that controls bitrate as a function of residual level (HYBRID_BITRATE).
#define SLS 8
#define SLO ((1 << (SLS - 1)))

#define LIMIT_ONES 16   // maximum consecutive 1s sent for "div" data

// these control the time constant of the 3 median level breakpoints
#define DIV0 128        // 5/7 of samples
#define DIV1 64         // 10/49 of samples
#define DIV2 32         // 20/343 of samples

// this macro retrieves the specified median breakpoint (without frac; min = 1)
#define GET_MED(med) (((c->median [med]) >> 4) + 1)

// These macros update the specified median breakpoints. Note that the median
// is incremented when the sample is higher than the median, else decremented.
// They are designed so that the median will never drop below 1 and the value
// is essentially stationary if there are 2 increments for every 5 decrements.

#define INC_MED0() (c->median [0] += ((c->median [0] + DIV0) / DIV0) * 5)
#define DEC_MED0() (c->median [0] -= ((c->median [0] + (DIV0-2)) / DIV0) * 2)
#define INC_MED1() (c->median [1] += ((c->median [1] + DIV1) / DIV1) * 5)
#define DEC_MED1() (c->median [1] -= ((c->median [1] + (DIV1-2)) / DIV1) * 2)
#define INC_MED2() (c->median [2] += ((c->median [2] + DIV2) / DIV2) * 5)
#define DEC_MED2() (c->median [2] -= ((c->median [2] + (DIV2-2)) / DIV2) * 2)

#ifdef HAVE___BUILTIN_CLZ
#define count_bits(av) ((av) ? 32 - __builtin_clz (av) : 0)
#elif defined (_WIN64)
static __inline int count_bits (uint32_t av) { unsigned long res; return _BitScanReverse (&res, av) ? (int)(res + 1) : 0; }
#else
#define count_bits(av) ( \
 (av) < (1 << 8) ? nbits_table [av] : \
  ( \
   (av) < (1L << 16) ? nbits_table [(av) >> 8] + 8 : \
   ((av) < (1L << 24) ? nbits_table [(av) >> 16] + 16 : nbits_table [(av) >> 24] + 24) \
  ) \
)
#endif

void init_words (WavpackStream *wps);
void write_entropy_vars (WavpackStream *wps, WavpackMetadata *wpmd);
void write_hybrid_profile (WavpackStream *wps, WavpackMetadata *wpmd);
int read_entropy_vars (WavpackStream *wps, WavpackMetadata *wpmd);
int read_hybrid_profile (WavpackStream *wps, WavpackMetadata *wpmd);
int32_t FASTCALL send_word (WavpackStream *wps, int32_t value, int chan);
void send_words_lossless (WavpackStream *wps, int32_t *buffer, int32_t nsamples);
int32_t FASTCALL get_word (WavpackStream *wps, int chan, int32_t *correction);
int32_t get_words_lossless (WavpackStream *wps, int32_t *buffer, int32_t nsamples);
void flush_word (WavpackStream *wps);
int32_t nosend_word (WavpackStream *wps, int32_t value, int chan);
void scan_word (WavpackStream *wps, int32_t *samples, uint32_t num_samples, int dir);
void update_error_limit (WavpackStream *wps);

extern const uint32_t bitset [32];
extern const uint32_t bitmask [32];
extern const char nbits_table [256];

int wp_log2s (int32_t value);
int32_t wp_exp2s (int log);
int FASTCALL wp_log2 (uint32_t avalue);

#ifdef OPT_ASM_X86
#define LOG2BUFFER log2buffer_x86
#elif defined(OPT_ASM_X64) && (defined (_WIN64) || defined(__CYGWIN__) || defined(__MINGW64__) || defined(__midipix__))
#define LOG2BUFFER log2buffer_x64win
#elif defined(OPT_ASM_X64)
#define LOG2BUFFER log2buffer_x64
#else
#define LOG2BUFFER log2buffer
#endif

uint32_t LOG2BUFFER (int32_t *samples, uint32_t num_samples, int limit);

signed char store_weight (int weight);
int restore_weight (signed char weight);

#define WORD_EOF ((int32_t)(1U << 31))

void WavpackFloatNormalize (int32_t *values, int32_t num_values, int delta_exp);

/////////////////////////// high-level unpacking API and support ////////////////////////////
// modules: open_utils.c, unpack_utils.c, unpack_seek.c, unpack_floats.c

WavpackContext *WavpackOpenFileInputEx64 (WavpackStreamReader64 *reader, void *wv_id, void *wvc_id, char *error, int flags, int norm_offset);
WavpackContext *WavpackOpenFileInputEx (WavpackStreamReader *reader, void *wv_id, void *wvc_id, char *error, int flags, int norm_offset);
WavpackContext *WavpackOpenFileInput (const char *infilename, char *error, int flags, int norm_offset);

#define OPEN_WVC        0x1     // open/read "correction" file
#define OPEN_TAGS       0x2     // read ID3v1 / APEv2 tags (seekable file)
#define OPEN_WRAPPER    0x4     // make audio wrapper available (i.e. RIFF)
#define OPEN_2CH_MAX    0x8     // open multichannel as stereo (no downmix)
#define OPEN_NORMALIZE  0x10    // normalize floating point data to +/- 1.0
#define OPEN_STREAMING  0x20    // "streaming" mode blindly unpacks blocks
                                // w/o regard to header file position info
#define OPEN_EDIT_TAGS  0x40    // allow editing of tags
#define OPEN_FILE_UTF8  0x80    // assume filenames are UTF-8 encoded, not ANSI (Windows only)

// new for version 5

#define OPEN_DSD_NATIVE 0x100   // open DSD files as bitstreams
                                // (returned as 8-bit "samples" stored in 32-bit words)
#define OPEN_DSD_AS_PCM 0x200   // open DSD files as 24-bit PCM (decimated 8x)
#define OPEN_ALT_TYPES  0x400   // application is aware of alternate file types & qmode
                                // (just affects retrieving wrappers & MD5 checksums)
#define OPEN_NO_CHECKSUM 0x800  // don't verify block checksums before decoding

int WavpackGetMode (WavpackContext *wpc);

int WavpackGetQualifyMode (WavpackContext *wpc);
int WavpackGetVersion (WavpackContext *wpc);
uint32_t WavpackUnpackSamples (WavpackContext *wpc, int32_t *buffer, uint32_t samples);
int WavpackSeekSample (WavpackContext *wpc, uint32_t sample);
int WavpackSeekSample64 (WavpackContext *wpc, int64_t sample);
int WavpackGetMD5Sum (WavpackContext *wpc, unsigned char data [16]);

int WavpackVerifySingleBlock (unsigned char *buffer, int verify_checksum);
uint32_t read_next_header (WavpackStreamReader64 *reader, void *id, WavpackHeader *wphdr);
int read_wvc_block (WavpackContext *wpc);

/////////////////////////// high-level packing API and support ////////////////////////////
// modules: pack_utils.c, pack_floats.c

WavpackContext *WavpackOpenFileOutput (WavpackBlockOutput blockout, void *wv_id, void *wvc_id);
int WavpackSetConfiguration (WavpackContext *wpc, WavpackConfig *config, uint32_t total_samples);
int WavpackSetConfiguration64 (WavpackContext *wpc, WavpackConfig *config, int64_t total_samples, const unsigned char *chan_ids);
int WavpackPackInit (WavpackContext *wpc);
int WavpackAddWrapper (WavpackContext *wpc, void *data, uint32_t bcount);
int WavpackPackSamples (WavpackContext *wpc, int32_t *sample_buffer, uint32_t sample_count);
int WavpackFlushSamples (WavpackContext *wpc);
int WavpackStoreMD5Sum (WavpackContext *wpc, unsigned char data [16]);
void WavpackSeekTrailingWrapper (WavpackContext *wpc);
void WavpackUpdateNumSamples (WavpackContext *wpc, void *first_block);
void *WavpackGetWrapperLocation (void *first_block, uint32_t *size);

/////////////////////////////////// common utilities ////////////////////////////////////
// module: common_utils.c

extern const uint32_t sample_rates [16];
uint32_t WavpackGetLibraryVersion (void);
const char *WavpackGetLibraryVersionString (void);
uint32_t WavpackGetSampleRate (WavpackContext *wpc);
int WavpackGetBitsPerSample (WavpackContext *wpc);
int WavpackGetBytesPerSample (WavpackContext *wpc);
int WavpackGetNumChannels (WavpackContext *wpc);
int WavpackGetChannelMask (WavpackContext *wpc);
int WavpackGetReducedChannels (WavpackContext *wpc);
int WavpackGetFloatNormExp (WavpackContext *wpc);
uint32_t WavpackGetNumSamples (WavpackContext *wpc);
int64_t WavpackGetNumSamples64 (WavpackContext *wpc);
uint32_t WavpackGetSampleIndex (WavpackContext *wpc);
int64_t WavpackGetSampleIndex64 (WavpackContext *wpc);
char *WavpackGetErrorMessage (WavpackContext *wpc);
int WavpackGetNumErrors (WavpackContext *wpc);
int WavpackLossyBlocks (WavpackContext *wpc);
uint32_t WavpackGetWrapperBytes (WavpackContext *wpc);
unsigned char *WavpackGetWrapperData (WavpackContext *wpc);
void WavpackFreeWrapper (WavpackContext *wpc);
double WavpackGetProgress (WavpackContext *wpc);
uint32_t WavpackGetFileSize (WavpackContext *wpc);
int64_t WavpackGetFileSize64 (WavpackContext *wpc);
double WavpackGetRatio (WavpackContext *wpc);
double WavpackGetAverageBitrate (WavpackContext *wpc, int count_wvc);
double WavpackGetInstantBitrate (WavpackContext *wpc);
WavpackContext *WavpackCloseFile (WavpackContext *wpc);
void WavpackLittleEndianToNative (void *data, char *format);
void WavpackNativeToLittleEndian (void *data, char *format);
void WavpackBigEndianToNative (void *data, char *format);
void WavpackNativeToBigEndian (void *data, char *format);

void install_close_callback (WavpackContext *wpc, void cb_func (void *wpc));
void free_dsd_tables (WavpackStream *wps);
void free_streams (WavpackContext *wpc);

/////////////////////////////////// tag utilities ////////////////////////////////////
// modules: tags.c, tag_utils.c

int WavpackGetNumTagItems (WavpackContext *wpc);
int WavpackGetTagItem (WavpackContext *wpc, const char *item, char *value, int size);
int WavpackGetTagItemIndexed (WavpackContext *wpc, int index, char *item, int size);
int WavpackGetNumBinaryTagItems (WavpackContext *wpc);
int WavpackGetBinaryTagItem (WavpackContext *wpc, const char *item, char *value, int size);
int WavpackGetBinaryTagItemIndexed (WavpackContext *wpc, int index, char *item, int size);
int WavpackAppendTagItem (WavpackContext *wpc, const char *item, const char *value, int vsize);
int WavpackAppendBinaryTagItem (WavpackContext *wpc, const char *item, const char *value, int vsize);
int WavpackDeleteTagItem (WavpackContext *wpc, const char *item);
int WavpackWriteTag (WavpackContext *wpc);
int load_tag (WavpackContext *wpc);
void free_tag (M_Tag *m_tag);
int valid_tag (M_Tag *m_tag);
int editable_tag (M_Tag *m_tag);

#endif

