////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2017 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wavpack.c

// This is the main module for the WavPack command-line compressor.

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#if defined(__OS2__)
#define INCL_DOSPROCESS
#include <os2.h>
#include <io.h>
#endif
#include <sys/param.h>
#include <sys/stat.h>
#include <locale.h>
#include <iconv.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#include "wavpack.h"
#include "utils.h"
#include "md5.h"

#if (defined(__GNUC__) || defined(__sun)) && !defined(_WIN32)
#include <unistd.h>
#include <glob.h>
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif

#ifdef _WIN32
#include "win32_unicode_support.h"
#define fputs fputs_utf8
#define fprintf fprintf_utf8
#define remove(f) unlink_utf8(f)
#define rename(o,n) rename_utf8(o,n)
#define fopen(f,m) fopen_utf8(f,m)
#define strdup(x) _strdup(x)
#define stricmp(x,y) _stricmp(x,y)
#define strdup(x) _strdup(x)
#define exp2(e) pow(2.0,e)
#else
#define stricmp strcasecmp
#endif

///////////////////////////// local variable storage //////////////////////////

static const char *sign_on = "\n"
" WAVPACK  Hybrid Lossless Audio Compressor  %s Version %s\n"
" Copyright (c) 1998 - 2017 David Bryant.  All Rights Reserved.\n\n";

static const char *version_warning = "\n"
" WARNING: WAVPACK using libwavpack version %s, expected %s (see README)\n\n";

static const char *usage =
#if defined (_WIN32)
" Usage:   WAVPACK [-options] infile[.wav]|infile.ext|- [outfile[.wv]|outpath|-]\n"
"             (default is lossless; infile may contain wildcards: ?,*)\n\n"
#else
" Usage:   WAVPACK [-options] infile[.wav]|infile.ext|- [...] [-o outfile[.wv]|outpath|-]\n"
"             (default is lossless; multiple input files allowed)\n\n"
#endif
" Formats: .wav (default, bwf/rf64 okay)  .wv (transcode, with tags)\n"
"          .w64 (Sony Wave64)             .caf (Core Audio Format)\n"
"          .dff (Philips DSDIFF)          .dsf (Sony DSD stream)\n\n"
" Options: -bn = enable hybrid compression, n = 2.0 to 23.9 bits/sample, or\n"
"                                           n = 24-9600 kbits/second (kbps)\n"
"          -c  = create correction file (.wvc) for hybrid mode (=lossless)\n"
"          -f  = fast mode (fast, but some compromise in compression ratio)\n"
"          -h  = high quality (better compression ratio, but slower)\n"
"          -v  = verify output file integrity after write (no pipes)\n"
"          -x  = extra encode processing (no decoding speed penalty)\n"
"          --help = complete help\n\n"
" Web:     Visit www.wavpack.com for latest version and info\n";

static const char *help =
#if defined (_WIN32)
" Usage:\n"
"    WAVPACK [-options] infile[.wav]|infile.ext|- [outfile[.wv]|outpath|-]\n\n"
"    The default operation is lossless. Wildcard characters (*,?) may be included\n"
"    in the filename and the source file type is automatically determined (see\n"
"    accepted formats below). Raw PCM may also be used (see --raw-pcm option).\n\n"
#else
" Usage:\n"
"    WAVPACK [-options] infile[.wav]|infile.ext|- [...] [-o outfile[.wv]|outpath|-]\n\n"
"    The default operation is lossless. Multiple input files may be specified\n"
"    and the source file type is automatically determined (see accepted formats\n"
"    below). Raw PCM data may also be used (see --raw-pcm option).\n\n"
#endif
" Input Formats:             .wav (default, includes bwf/rf64 varients)\n"
"                            .wv  (transcode operation, tags copied)\n"
"                            .caf (Core Audio Format)\n"
"                            .w64 (Sony Wave64)\n"
"                            .dff (Philips DSDIFF)\n"
"                            .dsf (Sony DSD stream)\n\n"
" Options:\n"
"    -a                      Adobe Audition (CoolEdit) mode for 32-bit floats\n"
"    --allow-huge-tags       allow tag data up to 16 MB (embedding > 1 MB is not\n"
"                             recommended for portable devices and may not work\n"
"                             with some programs including WavPack pre-4.70)\n"
"    -bn                     enable hybrid compression\n"
"                              n = 2.0 to 23.9 bits/sample, or\n"
"                              n = 24-9600 kbits/second (kbps)\n"
"                              add -c to create correction file (.wvc)\n"
"    --blocksize=n           specify block size in samples (max = 131072 and\n"
"                               min = 16 with --merge-blocks, otherwise 128)\n"
"    -c                      hybrid lossless mode (use with -b to create\n"
"                             correction file (.wvc) in hybrid mode)\n"
"    -cc                     maximum hybrid lossless compression (but degrades\n"
"                             decode speed and may result in lower quality)\n"
"    --channel-order=<list>  specify (comma separated) channel order if not\n"
"                             Microsoft standard (which is FL,FR,FC,LFE,BL,BR,\n"
"                             FLC,FRC,BC,SL,SR,TC,TFL,TFC,TFR,TBL,TBC,TBR);\n"
"                             specify '...' to indicate that channels are not\n"
"                             assigned to specific speakers, or terminate list\n"
"                             with '...' to indicate that any channels beyond\n"
"                             those specified are unassigned\n"
"    --cross-decorr          use cross-channel correlation in hybrid mode (on by\n"
"                             default in lossless mode and with -cc option)\n"
"    -d                      delete source file if successful (use with caution!)\n"
"    -f                      fast mode (faster encode and decode, but some\n"
"                             compromise in compression ratio)\n"
"    -h                      high quality (better compression ratio, but slower\n"
"                             encode and decode than default mode)\n"
"    -hh                     very high quality (best compression, but slowest\n"
"                             and NOT recommended for portable hardware use)\n"
"    --help                  this extended help display\n"
"    -i                      ignore length in file header (no pipe output allowed)\n"
"    --import-id3            import ID3v2 tags from the trailer of DSF files only\n"
"    -jn                     joint-stereo override (0 = left/right, 1 = mid/side)\n"
#if defined (_WIN32) || defined (__OS2__)
"    -l                      run at lower priority for smoother multitasking\n"
#endif
"    -m                      compute & store MD5 signature of raw audio data\n"
"    --merge-blocks          merge consecutive blocks with equal redundancy\n"
"                             (used with --blocksize option and is useful for\n"
"                             files generated by the lossyWAV program or\n"
"                             decoded HDCD files)\n"
"    -n                      calculate average and peak quantization noise\n"
"                             (for hybrid mode only, reference fullscale sine)\n"
#ifdef _WIN32
"    --no-utf8-convert       assume tag values read from files are already UTF-8,\n"
"                             don't attempt to convert from local encoding\n"
#else
"    --no-utf8-convert       don't recode passed tags from local encoding to\n"
"                             UTF-8, assume they are in UTF-8 already\n"
"    -o FILENAME | PATH      specify output filename or path\n"
#endif
"    --pair-unassigned-chans encode unassigned channels into stereo pairs\n"
#ifdef _WIN32
"    --pause                 pause before exiting (if console window disappears)\n"
#endif
"    --pre-quantize=bits     pre-quantize samples to <bits> BEFORE encoding and MD5\n"
"                             (common use would be --pre-quantize=20 for 24-bit or\n"
"                             float material recorded with typical converters)\n"
"    -q                      quiet (keep console output to a minimum)\n"
"    -r                      remove file headers (file-appropriate headers\n"
"                             will be regenerated during unpacking)\n"
"    --raw-pcm               input data is raw pcm (default is 44100 Hz, 16-bit\n"
"                             signed, 2-channels, little-endian)\n"
"    --raw-pcm=sr,bps[f|s|u],nch,[le|be]\n"
"                            input data is raw pcm with specified sample rate,\n"
"                             sample bit depth (float or signed or unsigned), number\n"
"                             of channels, and little-endian or big-endian\n"
"                             (defaulted parameters may be omitted)\n"
"    --raw-pcm-skip=begin[,end]\n"
"                            skip <begin> bytes before encoding (i.e., a header)\n"
"                             and <end> bytes at the end-of-file (i.e., a trailer)\n"
"    -sn                     override default noise shaping where n is a float\n"
"                             value between -1.0 and 1.0; negative values move noise\n"
"                             lower in freq, positive values move noise higher\n"
"                             in freq, use '0' for no shaping (white noise)\n"
"    -t                      copy input file's time stamp to output file(s)\n"
"    --use-dns               force use of dynamic noise shaping (hybrid mode only)\n"
"    -v                      verify output file integrity after write (no pipes)\n"
"    --version               write the version to stdout\n"
"    -w Encoder              write actual \"Encoder\" information to APEv2 tag\n"
"    -w Settings             write actual \"Settings\" information to APEv2 tag\n"
"    -w \"Field=Value\"        write specified text metadata to APEv2 tag\n"
"    -w \"Field=@file.ext\"    write specified text metadata from file to APEv2\n"
"                             tag, normally used for embedded cuesheets and logs\n"
"                             (field names \"Cuesheet\" and \"Log\")\n"
"    --write-binary-tag \"Field=@file.ext\"\n"
"                            write the specified binary metadata file to APEv2\n"
"                             tag, normally used for cover art with the specified\n"
"                             field name \"Cover Art (Front)\"\n"
"    -x[n]                   extra encode processing (optional n = 1 to 6, 1=default)\n"
"                             -x1 to -x3 to choose best of predefined filters\n"
"                             -x4 to -x6 to generate custom filters (very slow!)\n"
"    -y                      yes to all warnings (use with caution!)\n"
#if defined (_WIN32)
"    -z                      don't set console title to indicate progress\n\n"
#else
"    -z1                     set console title to indicate progress\n\n"
#endif
" Web:\n"
"     Visit www.wavpack.com for latest version and complete information\n";

static const char *speakers [] = {
    "FL", "FR", "FC", "LFE", "BL", "BR", "FLC", "FRC", "BC",
    "SL", "SR", "TC", "TFL", "TFC", "TFR", "TBL", "TBC", "TBR"
};

#define NUM_SPEAKERS (sizeof (speakers) / sizeof (speakers [0]))

int ParseRiffHeaderConfig (FILE *infile, char *infilename, char *fourcc, WavpackContext *wpc, WavpackConfig *config);
int ParseWave64HeaderConfig (FILE *infile, char *infilename, char *fourcc, WavpackContext *wpc, WavpackConfig *config);
int ParseCaffHeaderConfig (FILE *infile, char *infilename, char *fourcc, WavpackContext *wpc, WavpackConfig *config);
int ParseDsdiffHeaderConfig (FILE *infile, char *infilename, char *fourcc, WavpackContext *wpc, WavpackConfig *config);
int ParseDsfHeaderConfig (FILE *infile, char *infilename, char *fourcc, WavpackContext *wpc, WavpackConfig *config);

static struct {
    unsigned char id;
    char *fourcc, *default_extension;
    int (* ParseHeader) (FILE *infile, char *infilename, char *fourcc, WavpackContext *wpc, WavpackConfig *config);
    int chunk_alignment;
} file_formats [] = {
    { WP_FORMAT_WAV,  "RIFF", "wav", ParseRiffHeaderConfig,   2 },
    { WP_FORMAT_WAV,  "RF64", "wav", ParseRiffHeaderConfig,   2 },
    { WP_FORMAT_W64,  "riff", "w64", ParseWave64HeaderConfig, 8 },
    { WP_FORMAT_CAF,  "caff", "caf", ParseCaffHeaderConfig,   1 },
    { WP_FORMAT_DFF,  "FRM8", "dff", ParseDsdiffHeaderConfig, 2 },
    { WP_FORMAT_DSF,  "DSD ", "dsf", ParseDsfHeaderConfig,    1 }
};

#define NUM_FILE_FORMATS (sizeof (file_formats) / sizeof (file_formats [0]))

// this global is used to indicate the special "debug" mode where extra debug messages
// are displayed and all messages are logged to the file \wavpack.log

int debug_logging_mode;

static int overwrite_all, num_files, file_index, copy_time, quiet_mode, verify_mode, delete_source,
    no_utf8_convert, set_console_title, allow_huge_tags, quantize_bits, quantize_round, import_id3,
    raw_pcm_skip_bytes_begin, raw_pcm_skip_bytes_end;

static int num_channels_order;
static unsigned char channel_order [18];
static double encode_time_percent;

// These two statics are used to keep track of tags that the user specifies on the
// command line. The "num_tag_strings" and "tag_strings" fields in the WavpackConfig
// structure are no longer used for anything (they should not have been there in
// the first place).

static int num_tag_items, total_tag_size;

static struct tag_item {
    char *item, *value, *ext;
    int vsize, binary;
} *tag_items;

#if defined (_WIN32)
static int pause_mode;
#endif

/////////////////////////// local function declarations ///////////////////////

static FILE *wild_fopen (char *filename, const char *mode);
static int pack_file (char *infilename, char *outfilename, char *out2filename, const WavpackConfig *config);
static int pack_audio (WavpackContext *wpc, FILE *infile, int qmode, unsigned char *new_order, unsigned char *md5_digest_source);
static int pack_dsd_audio (WavpackContext *wpc, FILE *infile, int qmode, unsigned char *new_order, unsigned char *md5_digest_source);
static int repack_file (char *infilename, char *outfilename, char *out2filename, const WavpackConfig *config);
static int repack_audio (WavpackContext *wpc, WavpackContext *infile, unsigned char *md5_digest_source);
static int verify_audio (char *infilename, unsigned char *md5_digest_source);
static void make_settings_string (char *settings, WavpackConfig *config);
static void display_progress (double file_progress);
static void TextToUTF8 (void *string, int len);

#define WAVPACK_NO_ERROR    0
#define WAVPACK_SOFT_ERROR  1
#define WAVPACK_HARD_ERROR  2

// The "main" function for the command-line WavPack compressor. Note that on Windows
// this is actually a static function that is called from the "real" main() defined
// immediately afterward that converts the wchar argument list into UTF-8 strings
// and sets the console to UTF-8 for better Unicode support.

#ifdef _WIN32
static int wavpack_main(int argc, char **argv)
#else
int main (int argc, char **argv)
#endif
{
#ifdef __EMX__ /* OS/2 */
    _wildcard (&argc, &argv);
#endif
    int error_count = 0, tag_next_arg = 0, output_spec = 0;
    char *outfilename = NULL, *out2filename = NULL;
    char **matches = NULL;
    WavpackConfig config;
    int result, i;

#if defined(_WIN32)
    char selfname [MAX_PATH];

    if (GetModuleFileName (NULL, selfname, sizeof (selfname)) && filespec_name (selfname) &&
        _strupr (filespec_name (selfname)) && strstr (filespec_name (selfname), "DEBUG")) {
            char **argv_t = argv;
            int argc_t = argc;

            debug_logging_mode = TRUE;

            while (--argc_t)
                error_line ("arg %d: %s", argc - argc_t, *++argv_t);
    }

    strcpy (selfname, *argv);
#else
    if (filespec_name (*argv))
        if (strstr (filespec_name (*argv), "ebug") || strstr (filespec_name (*argv), "DEBUG")) {
            char **argv_t = argv;
            int argc_t = argc;

            debug_logging_mode = TRUE;

            while (--argc_t)
                error_line ("arg %d: %s", argc - argc_t, *++argv_t);
    }
#endif

#if defined (_WIN32)
    set_console_title = 1;      // on Windows, we default to messing with the console title
#endif                          // on Linux, this is considered uncool to do by default

    CLEAR (config);

    // loop through command-line arguments

    while (--argc)
        if (**++argv == '-' && (*argv)[1] == '-' && (*argv)[2]) {
            char *long_option = *argv + 2, *long_param = long_option;

            while (*long_param)
                if (*long_param++ == '=')
                    break;

            if (!strcmp (long_option, "help")) {                        // --help
                printf ("%s", help);
                return 0;
            }
            else if (!strcmp (long_option, "version")) {                // --version
                printf ("wavpack %s\n", PACKAGE_VERSION);
                printf ("libwavpack %s\n", WavpackGetLibraryVersionString ());
                return 0;
            }
#ifdef _WIN32
            else if (!strcmp (long_option, "pause"))                    // --pause
                pause_mode = 1;
#endif
            else if (!strcmp (long_option, "optimize-mono"))            // --optimize-mono
                error_line ("warning: --optimize-mono deprecated, now enabled by default");
            else if (!strcmp (long_option, "dns")) {                    // --dns
                error_line ("warning: --dns deprecated, use --use-dns");
                ++error_count;
            }
            else if (!strcmp (long_option, "use-dns"))                  // --use-dns
                config.flags |= CONFIG_DYNAMIC_SHAPING;
            else if (!strcmp (long_option, "cross-decorr"))             // --cross-decorr
                config.flags |= CONFIG_CROSS_DECORR;
            else if (!strcmp (long_option, "merge-blocks"))             // --merge-blocks
                config.flags |= CONFIG_MERGE_BLOCKS;
            else if (!strcmp (long_option, "pair-unassigned-chans"))    // --pair-unassigned-chans
                config.flags |= CONFIG_PAIR_UNDEF_CHANS;
            else if (!strcmp (long_option, "import-id3"))               // --import-id3
                import_id3 = 1;
            else if (!strcmp (long_option, "no-utf8-convert"))          // --no-utf8-convert
                no_utf8_convert = 1;
            else if (!strcmp (long_option, "allow-huge-tags"))          // --allow-huge-tags
                allow_huge_tags = 1;
            else if (!strcmp (long_option, "write-binary-tag"))         // --write-binary-tag
                tag_next_arg = 2;
            else if (!strncmp (long_option, "raw-pcm-skip", 12)) {      // --raw-pcm-skip
                raw_pcm_skip_bytes_begin = strtol (long_param, &long_param, 10);

                if (*long_param == ',')
                    raw_pcm_skip_bytes_end = strtol (++long_param, &long_param, 10);

                if (*long_param || raw_pcm_skip_bytes_begin < 0 || raw_pcm_skip_bytes_end < 0) {
                    error_line ("syntax error in raw-pcm-skip specification!");
                    ++error_count;
                }

                error_line ("raw_pcm_skip = %d, %d bytes", raw_pcm_skip_bytes_begin, raw_pcm_skip_bytes_end);
            }
            else if (!strncmp (long_option, "raw-pcm", 7)) {            // --raw-pcm
                int params [] = { 44100, 16, 2 };
                int pi, fp = 0, be = 0, us = 0, s = 0;

                for (pi = 0; *long_param && pi < 3; ++pi) {
                    if (isdigit (*long_param))
                        params [pi] = strtol (long_param, &long_param, 10);

                    if (pi == 1) {
                        if (*long_param == 'f' || *long_param == 'F') {
                            long_param++;
                            fp = 1;
                        }
                        else if (*long_param == 'u' || *long_param == 'U') {
                            long_param++;
                            us = 1;
                        }
                        else if (*long_param == 's' || *long_param == 'S') {
                            long_param++;
                            s = 1;
                        }
                    }

                    if (*long_param == ',')
                        long_param++;
                    else
                        break;
                }

                if (*long_param && pi == 3) {
                    if (!stricmp (long_param, "be")) {
                        long_param += 2;
                        be = 1;
                    }
                    else if (!stricmp (long_param, "le"))
                        long_param += 2;
                }

                if (*long_param) {
                    error_line ("syntax error in raw PCM specification!");
                    ++error_count;
                }
                else if (params [0] < 1 || params [0] > 1000000000 ||
                    params [1] < 1 || params [1] > 32 || (fp && params [1] != 32) ||
                    params [2] < 1 || params [2] > 256) {
                        error_line ("argument range error in raw PCM specification!");
                        ++error_count;
                }
                else if (params [1] == 1) {
                    config.sample_rate = params [0] / 8;
                    config.bits_per_sample = params [1] * 8;
                    config.bytes_per_sample = 1;
                    config.num_channels = params [2];
                    config.qmode |= QMODE_DSD_MSB_FIRST | QMODE_RAW_PCM;
                }
                else {
                    config.sample_rate = params [0];
                    config.bits_per_sample = params [1];
                    config.bytes_per_sample = (params [1] + 7) / 8;
                    config.num_channels = params [2];
                    config.float_norm_exp = fp ? 127 : 0;
                    config.qmode |= QMODE_RAW_PCM;

                    if (params [1] > 8) {
                        if (us)
                            config.qmode |= QMODE_UNSIGNED_WORDS;

                        if (be)
                            config.qmode |= QMODE_BIG_ENDIAN;
                    }
                    else if (s)
                        config.qmode |= QMODE_SIGNED_BYTES;
                }
            }
            else if (!strncmp (long_option, "blocksize", 9)) {          // --blocksize
                config.block_samples = strtol (long_param, NULL, 10);

                if (config.block_samples < 16 || config.block_samples > 131072) {
                    error_line ("invalid blocksize!");
                    ++error_count;
                }
            }
            else if (!strncmp (long_option, "channel-order", 13)) {      // --channel-order
                char name [6], channel_error = 0;
                uint32_t mask = 0;
                int chan, ci, si;

                for (chan = 0; chan < sizeof (channel_order); ++chan) {

                    if (!*long_param)
                        break;

                    if (*long_param == '.') {
                        if (*++long_param == '.' && *++long_param == '.' && !*++long_param)
                            config.qmode |= QMODE_CHANS_UNASSIGNED;
                        else
                            channel_error = 1;

                        break;
                    }

                    for (ci = 0; isalpha (*long_param) && ci < sizeof (name) - 1; ci++)
                        name [ci] = *long_param++;

                    if (!ci) {
                        channel_error = 1;
                        break;
                    } 

                    name [ci] = 0;

                    for (si = 0; si < NUM_SPEAKERS; ++si)
                        if (!stricmp (name, speakers [si])) {
                            if (mask & (1L << si))
                                channel_error = 1;

                            channel_order [chan] = si;
                            mask |= (1L << si);
                            break;
                        }

                    if (channel_error || si == NUM_SPEAKERS) {
                        error_line ("unknown or repeated channel spec: %s!", name);
                        channel_error = 1;
                        break;
                    } 

                    if (*long_param && *long_param++ != ',') {
                        channel_error = 1;
                        break;
                    } 
                }

                if (channel_error) {
                    error_line ("syntax error in channel order specification!");
                    ++error_count;
                }
                else if (*long_param) {
                    error_line ("too many channels specified!");
                    ++error_count;
                }
                else {
                    config.channel_mask = mask;
                    num_channels_order = chan;
                }
            }
            else if (!strncmp (long_option, "pre-quantize-round", 18)) {    // --pre-quantize-round=
                quantize_round = quantize_bits = strtol(long_param, NULL, 10);

                if (quantize_bits < 4 || quantize_bits > 32) {
                    error_line ("invalid quantize bits!");
                    ++error_count;
                }
            }
            else if (!strncmp (long_option, "pre-quantize", 12)) {          // --pre-quantize=
                quantize_bits = strtol(long_param, NULL, 10);

                if (quantize_bits < 4 || quantize_bits > 32) {
                    error_line ("invalid quantize bits!");
                    ++error_count;
                }
            }
            else {
                error_line ("unknown option: %s !", long_option);
                ++error_count;
            }
        }
#if defined (_WIN32)
        else if ((**argv == '-' || **argv == '/') && (*argv)[1])
#else
        else if ((**argv == '-') && (*argv)[1])
#endif
            while (*++*argv)
                switch (**argv) {

                    case 'Y': case 'y':
                        overwrite_all = 1;
                        break;

                    case 'D': case 'd':
                        delete_source = 1;
                        break;

                    case 'C': case 'c':
                        if (config.flags & CONFIG_CREATE_WVC)
                            config.flags |= CONFIG_OPTIMIZE_WVC;
                        else
                            config.flags |= CONFIG_CREATE_WVC;

                        break;

                    case 'X': case 'x':
                        config.xmode = strtol (++*argv, argv, 10);

                        if (config.xmode < 0 || config.xmode > 6) {
                            error_line ("extra mode only goes from 1 to 6!");
                            ++error_count;
                        }
                        else
                            config.flags |= CONFIG_EXTRA_MODE;

                        --*argv;
                        break;

                    case 'F': case 'f':
                        config.flags |= CONFIG_FAST_FLAG;
                        break;

                    case 'H': case 'h':
                        if (config.flags & CONFIG_HIGH_FLAG)
                            config.flags |= CONFIG_VERY_HIGH_FLAG;
                        else
                            config.flags |= CONFIG_HIGH_FLAG;

                        break;

                    case 'N': case 'n':
                        config.flags |= CONFIG_CALC_NOISE;
                        break;

                    case 'A': case 'a':
                        config.qmode |= QMODE_ADOBE_MODE;
                        break;
#if defined (_WIN32)
                    case 'L': case 'l':
                        SetPriorityClass (GetCurrentProcess(), IDLE_PRIORITY_CLASS);
                        break;
#elif defined (__OS2__)
                    case 'L': case 'l':
                        DosSetPriority (0, PRTYC_IDLETIME, 0, 0);
                        break;
#endif
#if defined (_WIN32)
                    case 'O': case 'o':  // ignore -o in Windows to be Linux compatible
                        break;
#else
                    case 'O': case 'o':
                        output_spec = 1;
                        break;
#endif
                    case 'T': case 't':
                        copy_time = 1;
                        break;

                    case 'Q': case 'q':
                        quiet_mode = 1;
                        break;

                    case 'Z': case 'z':
                        set_console_title = strtol (++*argv, argv, 10);
                        --*argv;
                        break;

                    case 'M': case 'm':
                        config.flags |= CONFIG_MD5_CHECKSUM;
                        break;

                    case 'I': case 'i':
                        config.qmode |= QMODE_IGNORE_LENGTH;
                        break;

                    case 'R': case 'r':
                        config.qmode |= QMODE_NO_STORE_WRAPPER;
                        break;

                    case 'V': case 'v':
                        verify_mode = 1;
                        break;

                    case 'B': case 'b':
                        config.flags |= CONFIG_HYBRID_FLAG;
                        config.bitrate = (float) strtod (++*argv, argv);
                        --*argv;

                        if (config.bitrate < 2.0 || config.bitrate > 9600.0) {
                            error_line ("hybrid spec must be 2.0 to 9600!");
                            ++error_count;
                        }

                        if (config.bitrate >= 24.0)
                            config.flags |= CONFIG_BITRATE_KBPS;

                        break;

                    case 'J': case 'j':
                        switch (strtol (++*argv, argv, 10)) {

                            case 0:
                                config.flags |= CONFIG_JOINT_OVERRIDE;
                                config.flags &= ~CONFIG_JOINT_STEREO;
                                break;

                            case 1:
                                config.flags |= (CONFIG_JOINT_OVERRIDE | CONFIG_JOINT_STEREO);
                                break;

                            default:
                                error_line ("-j0 or -j1 only!");
                                ++error_count;
                        }

                        --*argv;
                        break;

                    case 'S': case 's':
                        config.shaping_weight = (float) strtod (++*argv, argv);

                        if (!config.shaping_weight) {
                            config.flags |= CONFIG_SHAPE_OVERRIDE;
                            config.flags &= ~CONFIG_HYBRID_SHAPE;
                        }
                        else if (config.shaping_weight >= -1.0 && config.shaping_weight <= 1.0)
                            config.flags |= (CONFIG_HYBRID_SHAPE | CONFIG_SHAPE_OVERRIDE);
                        else {
                            error_line ("-s-1.00 to -s1.00 only!");
                            ++error_count;
                        }

                        --*argv;
                        break;

                    case 'W': case 'w':
                        if (++tag_next_arg == 2) {
                            error_line ("warning: -ww deprecated, use --write-binary-tag");
                            ++error_count;
                        }

                        break;

                    default:
                        error_line ("illegal option: %c !", **argv);
                        ++error_count;
                }
        else if (tag_next_arg) {
            char *cp;

            // check for and allow "encoder" or "settings" without a value and create
            // an appropriate value for them (otherwise missing value is an error)

            if (!stricmp (*argv, "encoder")) {
                char *tag_arg = malloc (80);
                sprintf (tag_arg, "%s=WavPack %s", *argv, PACKAGE_VERSION);
                *argv = tag_arg;
            }
            else if (!stricmp (*argv, "settings")) {
                char settings [256], *tag_arg;

                make_settings_string (settings, &config);
                tag_arg = malloc (strlen (settings) + 16);
                sprintf (tag_arg, "%s=%s", *argv, settings);
                *argv = tag_arg;
            }

            cp = strchr (*argv, '=');

            if (cp && cp > *argv) {
                int i = num_tag_items;

                tag_items = realloc (tag_items, ++num_tag_items * sizeof (*tag_items));
                tag_items [i].item = malloc (cp - *argv + 1);
                memcpy (tag_items [i].item, *argv, cp - *argv);
                tag_items [i].item [cp - *argv] = 0;
                tag_items [i].vsize = (int) strlen (cp + 1);
                tag_items [i].value = malloc (tag_items [i].vsize + 1);
                strcpy (tag_items [i].value, cp + 1);
                tag_items [i].binary = (tag_next_arg == 2);
                tag_items [i].ext = NULL;
            }
            else {
                error_line ("error in tag spec: %s !", *argv);
                ++error_count;
            }

            tag_next_arg = 0;
        }
#if defined (_WIN32)
        else if (!num_files) {
            matches = realloc (matches, (num_files + 1) * sizeof (*matches));
            matches [num_files] = malloc (strlen (*argv) + 10);
            strcpy (matches [num_files], *argv);

            if (*(matches [num_files]) != '-' && *(matches [num_files]) != '@' &&
                !filespec_ext (matches [num_files]))
                    strcat (matches [num_files], (config.qmode & QMODE_RAW_PCM) ? ".raw" : ".wav");

            num_files++;
        }
        else if (!outfilename) {
            outfilename = malloc (strlen (*argv) + PATH_MAX);
            strcpy (outfilename, *argv);
        }
        else if (!out2filename) {
            out2filename = malloc (strlen (*argv) + PATH_MAX);
            strcpy (out2filename, *argv);
        }
        else {
            error_line ("extra unknown argument: %s !", *argv);
            ++error_count;
        }
#else
        else if (output_spec) {
            outfilename = malloc (strlen (*argv) + PATH_MAX);
            strcpy (outfilename, *argv);
            output_spec = 0;
        }
        else {
            matches = realloc (matches, (num_files + 1) * sizeof (*matches));
            matches [num_files] = malloc (strlen (*argv) + 10);
            strcpy (matches [num_files], *argv);

            if (*(matches [num_files]) != '-' && *(matches [num_files]) != '@' &&
                !filespec_ext (matches [num_files]))
                    strcat (matches [num_files], (config.qmode & QMODE_RAW_PCM) ? ".raw" : ".wav");

            num_files++;
        }
#endif

    setup_break ();     // set up console and detect ^C and ^Break

    // check for various command-line argument problems

    if (output_spec) {
        error_line ("no output filename or path specified with -o option!");
        ++error_count;
    }

    if (tag_next_arg) {
        error_line ("no tag specified with %s option!", tag_next_arg == 1 ? "-w" : "--write-binary-tag");
        ++error_count;
    }

    if (!(~config.flags & (CONFIG_HIGH_FLAG | CONFIG_FAST_FLAG))) {
        error_line ("high and fast modes are mutually exclusive!");
        ++error_count;
    }

    if ((config.qmode & QMODE_IGNORE_LENGTH) && outfilename && *outfilename == '-') {
        error_line ("can't ignore length in header when using stdout!");
        ++error_count;
    }

    if (verify_mode && outfilename && *outfilename == '-') {
        error_line ("can't verify output file when using stdout!");
        ++error_count;
    }

    if (config.flags & CONFIG_HYBRID_FLAG) {
        if ((config.flags & CONFIG_CREATE_WVC) && outfilename && *outfilename == '-') {
            error_line ("can't create correction file when using stdout!");
            ++error_count;
        }
        if (config.flags & CONFIG_MERGE_BLOCKS) {
            error_line ("--merge-blocks option is for lossless mode only!");
            ++error_count;
        }
        if ((config.flags & CONFIG_SHAPE_OVERRIDE) && (config.flags & CONFIG_DYNAMIC_SHAPING)) {
            error_line ("-s and --use-dns options are mutually exclusive!");
            ++error_count;
        }
    }
    else {
        if (config.flags & (CONFIG_CALC_NOISE | CONFIG_SHAPE_OVERRIDE | CONFIG_CREATE_WVC | CONFIG_DYNAMIC_SHAPING)) {
            error_line ("-c, -n, -s, and --use-dns options are for hybrid mode (-b) only!");
            ++error_count;
        }
    }

    if (config.flags & CONFIG_MERGE_BLOCKS) {
        if (!config.block_samples) {
            error_line ("--merge-blocks only makes sense when --blocksize is specified!");
            ++error_count;
        }
    }
    else if (config.block_samples && config.block_samples < 128) {
        error_line ("minimum blocksize is 128 when --merge-blocks is not specified!");
        ++error_count;
    }

    if (strcmp (WavpackGetLibraryVersionString (), PACKAGE_VERSION)) {
        fprintf (stderr, version_warning, WavpackGetLibraryVersionString (), PACKAGE_VERSION);
        fflush (stderr);
    }
    else if (!quiet_mode && !error_count) {
        fprintf (stderr, sign_on, VERSION_OS, WavpackGetLibraryVersionString ());
        fflush (stderr);
    }

    // Loop through any tag specification strings and check for file access, convert text
    // strings to UTF-8, and otherwise prepare for writing to APE tags. This is done here
    // rather than after encoding so that any errors can be reported to the user now.

    for (i = 0; i < num_tag_items; ++i) {
#ifdef _WIN32
        int tag_came_from_file = 0;
#endif
        if (*tag_items [i].value == '@') {
            char *fn = tag_items [i].value + 1, *new_value = NULL;
            FILE *file = wild_fopen (fn, "rb");

            // if the file is not found, try using any input and output directories that the
            // user may have specified on the command line

            if (!file && num_files && filespec_name (matches [0]) && *matches [0] != '-') {
                char *temp = malloc (strlen (matches [0]) + PATH_MAX);

                strcpy (temp, matches [0]);
                strcpy (filespec_name (temp), fn);
                file = wild_fopen (temp, "rb");
                free (temp);
            }

            if (!file && outfilename && filespec_name (outfilename) && *outfilename != '-') {
                char *temp = malloc (strlen (outfilename) + PATH_MAX);

                strcpy (temp, outfilename);
                strcpy (filespec_name (temp), fn);
                file = wild_fopen (temp, "rb");
                free (temp);
            }

            if (file) {
                uint32_t bcount;

                tag_items [i].vsize = (int) DoGetFileSize (file);

                if (filespec_ext (fn))
                    tag_items [i].ext = strdup (filespec_ext (fn));

                if (tag_items [i].vsize < 1048576 * (allow_huge_tags ? 16 : 1)) {
                    new_value = malloc (tag_items [i].vsize + 2);
                    memset (new_value, 0, tag_items [i].vsize + 2);

                    if (!DoReadFile (file, new_value, tag_items [i].vsize, &bcount) ||
                        bcount != tag_items [i].vsize) {
                            free (new_value);
                            new_value = NULL;
                        }
                }

                DoCloseHandle (file);
            }

            if (!new_value) {
                error_line ("error in tag spec: %s !", tag_items [i].value);
                ++error_count;
            }
            else {
                free (tag_items [i].value);
                tag_items [i].value = new_value;
#ifdef _WIN32
                tag_came_from_file = 1;
#endif
            }
        }
        else if (tag_items [i].binary) {
            error_line ("binary tags must be from files: %s !", tag_items [i].value);
            ++error_count;
        }

        if (tag_items [i].binary) {
            int isize = (int) strlen (tag_items [i].item);
            int esize = tag_items [i].ext ? (int) strlen (tag_items [i].ext) : 0;

            tag_items [i].value = realloc (tag_items [i].value, isize + esize + 1 + tag_items [i].vsize);
            memmove (tag_items [i].value + isize + esize + 1, tag_items [i].value, tag_items [i].vsize);
            strcpy (tag_items [i].value, tag_items [i].item);

            if (tag_items [i].ext)
                strcat (tag_items [i].value, tag_items [i].ext);

            tag_items [i].vsize += isize + esize + 1;
        }
        else if (tag_items [i].vsize) {
            tag_items [i].value = realloc (tag_items [i].value, tag_items [i].vsize * 2 + 1);

#ifdef _WIN32
            if (tag_came_from_file && !no_utf8_convert)
#else
            if (!no_utf8_convert)
#endif
                TextToUTF8 (tag_items [i].value, (int) tag_items [i].vsize * 2 + 1);

            // if a UTF8 BOM gets through to here, delete it now (redundant in APEv2 tags)

            if (tag_items [i].vsize >= 3 && (unsigned char) tag_items [i].value [0] == 0xEF &&
                (unsigned char) tag_items [i].value [1] == 0xBB && (unsigned char) tag_items [i].value [2] == 0xBF) {
                    memmove (tag_items [i].value, tag_items [i].value + 3, tag_items [i].vsize -= 3);
                    tag_items [i].value [tag_items [i].vsize] = 0;
            }

            tag_items [i].vsize = (int) strlen (tag_items [i].value);
        }

        if ((total_tag_size += tag_items [i].vsize) > 1048576 * (allow_huge_tags ? 16 : 1)) {
            error_line ("total APEv2 tag size exceeds %d MB !", allow_huge_tags ? 16 : 1);
            ++error_count;
            break;
        }
    }

    if (error_count) {
        fprintf (stderr, "\ntype 'wavpack' for short help or 'wavpack --help' for full help\n");
        fflush (stderr);
        return 1;
    }

    if (!num_files) {
        printf ("%s", usage);
        return 1;
    }

    for (file_index = 0; file_index < num_files; ++file_index) {
        char *infilename = matches [file_index];

        // If the single infile specification begins with a '@', then it
        // actually points to a file that contains the names of the files
        // to be converted. This was included for use by Wim Speekenbrink's
        // frontends, but could be used for other purposes.

        if (*infilename == '@') {
            FILE *list = fopen (infilename+1, "rb");
            char *listbuff = NULL, *cp;
            int listbytes = 0, di, c;

            for (di = file_index; di < num_files - 1; di++)
                matches [di] = matches [di + 1];

            file_index--;
            num_files--;

            if (list == NULL) {
                error_line ("file %s not found!", infilename+1);
                free (infilename);
                return 1;
            }

            while (1) {
                int bytes_read;

                listbuff = realloc (listbuff, listbytes + 1024);
                memset (listbuff + listbytes, 0, 1024);
                listbytes += bytes_read = (int) fread (listbuff + listbytes, 1, 1024, list);

                if (bytes_read < 1024)
                    break;
            }

#if defined (_WIN32)
            listbuff = realloc (listbuff, listbytes *= 2);
            TextToUTF8 (listbuff, listbytes);
#endif
            cp = listbuff;

            while ((c = *cp++)) {

                while (c == '\n' || c == '\r')
                    c = *cp++;

                if (c) {
                    char *fname = malloc (PATH_MAX);
                    int ci = 0;

                    do
                        fname [ci++] = c;
                    while ((c = *cp++) != '\n' && c != '\r' && c && ci < PATH_MAX);

                    fname [ci++] = '\0';
                    matches = realloc (matches, ++num_files * sizeof (*matches));

                    for (di = num_files - 1; di > file_index + 1; di--)
                        matches [di] = matches [di - 1];

                    matches [++file_index] = fname;
                }

                if (!c)
                    break;
            }

            fclose (list);
            free (listbuff);
            free (infilename);
        }
#if defined (_WIN32)
        else if (filespec_wild (infilename)) {
            wchar_t *winfilename = utf8_to_utf16(infilename);
            struct _wfinddata_t _wfinddata_t;
            intptr_t file;
            int di;

            for (di = file_index; di < num_files - 1; di++)
                matches [di] = matches [di + 1];

            file_index--;
            num_files--;

            if ((file = _wfindfirst (winfilename, &_wfinddata_t)) != (intptr_t) -1) {
                do {
                    char *name_utf8;

                    if (!(_wfinddata_t.attrib & _A_SUBDIR) && (name_utf8 = utf16_to_utf8(_wfinddata_t.name))) {
                        matches = realloc (matches, ++num_files * sizeof (*matches));

                        for (di = num_files - 1; di > file_index + 1; di--)
                            matches [di] = matches [di - 1];

                        matches [++file_index] = malloc (strlen (infilename) + strlen (name_utf8) + 10);
                        strcpy (matches [file_index], infilename);
                        *filespec_name (matches [file_index]) = '\0';
                        strcat (matches [file_index], name_utf8);
                        free (name_utf8);
                    }
                } while (_wfindnext (file, &_wfinddata_t) == 0);

                _findclose (file);
            }

            free (winfilename);
            free (infilename);
        }
#endif
    }

    // If the outfile specification begins with a '@', then it actually points
    // to a file that contains the output specification. This was included for
    // use by Wim Speekenbrink's frontends because certain filenames could not
    // be passed on the command-line, but could be used for other purposes.

    if (outfilename && outfilename [0] == '@') {
        char listbuff [PATH_MAX * 2], *lp = listbuff;
        FILE *list = fopen (outfilename+1, "rb");
        int c;

        if (list == NULL) {
            error_line ("file %s not found!", outfilename+1);
            free(outfilename);
            return 1;
        }

        memset (listbuff, 0, sizeof (listbuff));
        c = (int) fread (listbuff, 1, sizeof (listbuff) - 1, list);   // assign c only to suppress warning

#if defined (_WIN32)
        TextToUTF8 (listbuff, PATH_MAX * 2);
#endif

        while ((c = *lp++) == '\n' || c == '\r');

        if (c) {
            int ci = 0;

            do
                outfilename [ci++] = c;
            while ((c = *lp++) != '\n' && c != '\r' && c && ci < PATH_MAX);

            outfilename [ci] = '\0';
        }
        else {
            error_line ("output spec file is empty!");
            free(outfilename);
            fclose (list);
            return 1;
        }

        fclose (list);
    }

    if (out2filename && (num_files > 1 || !(config.flags & CONFIG_CREATE_WVC))) {
        error_line ("extra unknown argument: %s !", out2filename);
        return 1;
    }

    // if we found any files to process, this is where we start

    if (num_files) {
        char outpath, addext;

        // calculate an estimate for the percentage of the time that will be used for the encoding (as opposed
        // to the optional verification step) based on the "extra" mode processing; this is only used for
        // displaying the progress and so is not very critical

        if (verify_mode) {
            if (config.flags & CONFIG_EXTRA_MODE) {
                if (config.xmode)
                    encode_time_percent = 100.0 * (1.0 - (1.0 / ((1 << config.xmode) + 1)));
                else
                    encode_time_percent = 66.7;
            }
            else
                encode_time_percent = 50.0;
        }
        else
            encode_time_percent = 100.0;

        if (outfilename && *outfilename != '-') {
            outpath = (filespec_path (outfilename) != NULL);

            if (num_files > 1 && !outpath) {
                error_line ("%s is not a valid output path", outfilename);
                free(outfilename);
                return 1;
            }
        }
        else
            outpath = 0;

        addext = !outfilename || outpath || !filespec_ext (outfilename);

        // loop through and process files in list

        for (file_index = 0; file_index < num_files; ++file_index) {
            if (check_break ())
                break;

            // generate output filename

            if (outpath) {
                strcat (outfilename, filespec_name (matches [file_index]));

                if (filespec_ext (outfilename))
                    *filespec_ext (outfilename) = '\0';
            }
            else if (!outfilename) {
                outfilename = malloc (strlen (matches [file_index]) + 10);
                strcpy (outfilename, matches [file_index]);

                if (filespec_ext (outfilename))
                    *filespec_ext (outfilename) = '\0';
            }

            if (addext && *outfilename != '-')
                strcat (outfilename, ".wv");

            // if "correction" file is desired, generate name for that

            if (config.flags & CONFIG_CREATE_WVC) {
                if (!out2filename) {
                    out2filename = malloc (strlen (outfilename) + 10);
                    strcpy (out2filename, outfilename);
                }
                else {
                    char *temp = malloc (strlen (outfilename) + PATH_MAX);

                    strcpy (temp, outfilename);
                    strcpy (filespec_name (temp), filespec_name (out2filename));
                    strcpy (out2filename, temp);
                    free (temp);
                }

                if (filespec_ext (out2filename))
                    *filespec_ext (out2filename) = '\0';

                strcat (out2filename, ".wvc");
            }
            else
                out2filename = NULL;

            if (num_files > 1 && !quiet_mode) {
                fprintf (stderr, "\n%s:\n", matches [file_index]);
                fflush (stderr);
            }

            if (filespec_ext (matches [file_index]) && !stricmp (filespec_ext (matches [file_index]), ".wv"))
                result = repack_file (matches [file_index], outfilename, out2filename, &config);
            else
                result = pack_file (matches [file_index], outfilename, out2filename, &config);

            if (result != WAVPACK_NO_ERROR)
                ++error_count;

            if (result == WAVPACK_HARD_ERROR)
                break;

            // clean up in preparation for potentially another file

            if (outpath)
                *filespec_name (outfilename) = '\0';
            else if (*outfilename != '-') {
                free (outfilename);
                outfilename = NULL;
            }

            if (out2filename) {
                free (out2filename);
                out2filename = NULL;
            }

            free (matches [file_index]);
        }

        if (num_files > 1) {
            if (error_count) {
                fprintf (stderr, "\n **** warning: errors occurred in %d of %d files! ****\n", error_count, num_files);
                fflush (stderr);
            }
            else if (!quiet_mode) {
                fprintf (stderr, "\n **** %d files successfully processed ****\n", num_files);
                fflush (stderr);
            }
        }

        free (matches);
    }
    else {
        error_line ("nothing to do!");
        ++error_count;
    }

    if (outfilename)
        free (outfilename);

    if (set_console_title)
        DoSetConsoleTitle ("WavPack Completed");

    return error_count ? 1 : 0;
}

#ifdef _WIN32

// On Windows, this "real" main() acts as a shell to our static wavpack_main().
// Its purpose is to convert the wchar command-line arguments into UTF-8 encoded
// strings.

int main(int argc, char **argv)
{
    int ret = -1, argc_utf8 = -1;
    char **argv_utf8 = NULL;
    char **argv_copy = NULL;

    init_commandline_arguments_utf8(&argc_utf8, &argv_utf8);

    // we have to make a copy of the argv pointer array because the command parser
    // sometimes modifies them, which is problematic when it comes time to free them

    if (argc_utf8 && argv_utf8) {
        argv_copy = malloc (sizeof (char*) * argc_utf8);
        memcpy (argv_copy, argv_utf8, sizeof (char*) * argc_utf8);
    }

    ret = wavpack_main(argc_utf8, argv_copy);

    if (argv_copy)
        free (argv_copy);

    free_commandline_arguments_utf8(&argc_utf8, &argv_utf8);

    if (pause_mode) {
        fprintf (stderr, "\nPress any key to continue . . . ");
        fflush (stderr);
        while (!_kbhit ());
        _getch ();
        fprintf (stderr, "\n");
    }

    return ret;
}

#endif

// This structure and function are used to write completed WavPack blocks in
// a device independent way.

typedef struct {
    uint32_t bytes_written, first_block_size;
    FILE *file;
    int error;
} write_id;

static int write_block (void *id, void *data, int32_t length)
{
    write_id *wid = (write_id *) id;
    uint32_t bcount;

    if (wid->error)
        return FALSE;

    if (wid && wid->file && data && length) {
        if (!DoWriteFile (wid->file, data, length, &bcount) || bcount != length) {
            DoTruncateFile (wid->file);
            DoCloseHandle (wid->file);
            wid->file = NULL;
            wid->error = 1;
            return FALSE;
        }
        else {
            wid->bytes_written += length;

            if (!wid->first_block_size)
                wid->first_block_size = bcount;
        }
    }

    return TRUE;
}

// Special version of fopen() that allows a wildcard specification for the
// filename. If a wildcard is specified, then it must match 1 and only 1
// file to be acceptable (i.e. it won't match just the "first" file).

#if defined (_WIN32)

static FILE *wild_fopen (char *filename, const char *mode)
{
    struct _wfinddata_t _wfinddata_t;
    char *matchname = NULL;
    wchar_t *wfilename;
    FILE *res = NULL;
    intptr_t file;

    if (!filespec_wild (filename) || !filespec_name (filename))
        return fopen (filename, mode);

    wfilename = utf8_to_utf16(filename);

    if (!wfilename)
        return NULL;

    if ((file = _wfindfirst (wfilename, &_wfinddata_t)) != (intptr_t) -1) {
        do {
            if (!(_wfinddata_t.attrib & _A_SUBDIR)) {
                char *name_utf8;

                if (matchname) {
                    free (matchname);
                    matchname = NULL;
                    break;
                }
                else if ((name_utf8 = utf16_to_utf8(_wfinddata_t.name))) {
                    matchname = malloc (strlen (filename) + strlen(name_utf8));
                    strcpy (matchname, filename);
                    strcpy (filespec_name (matchname), name_utf8);
                    free (name_utf8);
                }
            }
        } while (_wfindnext (file, &_wfinddata_t) == 0);

        _findclose (file);
    }

    if (matchname) {
        res = fopen (matchname, mode);
        free (matchname);
    }

    free (wfilename);
    return res;
}

#else

static FILE *wild_fopen (char *filename, const char *mode)
{
    char *matchname = NULL;
    struct stat statbuf;
    FILE *res = NULL;
    glob_t globbuf;
    int i;

    glob (filename, 0, NULL, &globbuf);

    for (i = 0; i < globbuf.gl_pathc; ++i) {
        if (stat (globbuf.gl_pathv [i], &statbuf) == -1 || S_ISDIR (statbuf.st_mode))
            continue;

        if (matchname) {
            free (matchname);
            matchname = NULL;
            break;
        }
        else {
            matchname = malloc (strlen (globbuf.gl_pathv [i]) + 10);
            strcpy (matchname, globbuf.gl_pathv [i]);
        }
    }

    globfree (&globbuf);

    if (matchname) {
        res = fopen (matchname, mode);
        free (matchname);
    }

    return res;
}

#endif


// This function packs a single file "infilename" and stores the result at
// "outfilename". If "out2filename" is specified, then the "correction"
// file would go there. The files are opened and closed in this function
// and the "config" structure specifies the mode of compression.

int ImportID3v2 (WavpackContext *wpc, unsigned char *tag_data, int tag_size, char *error, int32_t *bytes_used); // import_id3.c

static int pack_file (char *infilename, char *outfilename, char *out2filename, const WavpackConfig *config)
{
    char *outfilename_temp = NULL, *out2filename_temp = NULL, dummy;
    int use_tempfiles = (out2filename != NULL), chunk_alignment = 1;
    int imported_tag_items = 0;
    uint32_t bcount;
    WavpackConfig loc_config = *config;
    unsigned char *new_channel_order = NULL;
    unsigned char md5_digest [16];
    write_id wv_file, wvc_file;
    WavpackContext *wpc;
    double dtime;
    FILE *infile;
    int result;

#if defined(_WIN32)
    struct __timeb64 time1, time2;
#else
    struct timeval time1, time2;
    struct timezone timez;
#endif

    CLEAR (wv_file);
    CLEAR (wvc_file);
    wpc = WavpackOpenFileOutput (write_block, &wv_file, out2filename ? &wvc_file : NULL);

    // open the source file for reading

    if (*infilename == '-') {
        infile = stdin;
#if defined(_WIN32)
        _setmode (_fileno (stdin), O_BINARY);
#endif
#if defined(__OS2__)
        setmode (fileno (stdin), O_BINARY);
#endif
    }
    else if ((infile = fopen (infilename, "rb")) == NULL) {
        error_line ("can't open file %s!", infilename);
        WavpackCloseFile (wpc);
        return WAVPACK_SOFT_ERROR;
    }

    if (loc_config.qmode & QMODE_RAW_PCM) {
        int64_t infilesize = DoGetFileSize (infile), total_samples;

        if (infilesize) {
            int sample_size = loc_config.bytes_per_sample * loc_config.num_channels;

            infilesize -= raw_pcm_skip_bytes_begin + raw_pcm_skip_bytes_end;
            total_samples = infilesize / sample_size;

            if (total_samples <= 0) {
                error_line ("no raw PCM data to encode!");
                DoCloseHandle (infile);
                WavpackCloseFile (wpc);
                return WAVPACK_SOFT_ERROR;
            }

            if (infilesize % sample_size)
                error_line ("warning: raw PCM infile length does not divide evenly, %d bytes will be discarded",
                    (int)(infilesize % sample_size));
        }
        else {
            if (raw_pcm_skip_bytes_end) {
                error_line ("can't skip trailer in raw PCM read from stdin!");
                DoCloseHandle (infile);
                WavpackCloseFile (wpc);
                return WAVPACK_SOFT_ERROR;
            }

            loc_config.qmode |= QMODE_IGNORE_LENGTH;
            total_samples = -1;
        }

        if (!loc_config.channel_mask && !(loc_config.qmode & QMODE_CHANS_UNASSIGNED)) {
            if (loc_config.num_channels <= 2)
                loc_config.channel_mask = 0x5 - loc_config.num_channels;
            else if (loc_config.num_channels <= 18)
                loc_config.channel_mask = (1 << loc_config.num_channels) - 1;
            else
                loc_config.channel_mask = 0x3ffff;
        }

        if (!WavpackSetConfiguration64 (wpc, &loc_config, total_samples, NULL)) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            DoCloseHandle (infile);
            WavpackCloseFile (wpc);
            return WAVPACK_SOFT_ERROR;
        }
    }

    // check both output files for overwrite warning required

    // note that for a file to be considered "overwritable", it must both be openable for reading
    // and have at least 1 readable byte - this prevents us getting stuck on "nul" (Windows) 

    if (*outfilename != '-' && (wv_file.file = fopen (outfilename, "rb")) != NULL) {
        size_t res = fread (&dummy, 1, 1, wv_file.file);

        DoCloseHandle (wv_file.file);

        if (res == 1) {
            use_tempfiles = 1;

            if (!overwrite_all) {
                fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (outfilename));
                fflush (stderr);

                if (set_console_title)
                    DoSetConsoleTitle ("overwrite?");

                switch (yna ()) {
                    case 'n':
                        DoCloseHandle (infile);
                        WavpackCloseFile (wpc);
                        return WAVPACK_SOFT_ERROR;

                    case 'a':
                        overwrite_all = 1;
                }
            }
        }
    }

    if (out2filename && !overwrite_all && (wvc_file.file = fopen (out2filename, "rb")) != NULL) {
        size_t res = fread (&dummy, 1, 1, wvc_file.file);

        DoCloseHandle (wvc_file.file);

        if (res == 1) {
            fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (out2filename));
            fflush (stderr);

            if (set_console_title)
                DoSetConsoleTitle ("overwrite?");

            switch (yna ()) {

                case 'n':
                    DoCloseHandle (infile);
                    WavpackCloseFile (wpc);
                    return WAVPACK_SOFT_ERROR;

                case 'a':
                    overwrite_all = 1;
            }
        }
    }

    // if we are using temp files, either because the output filename already exists or we are creating a
    // "correction" file, search for and generate the corresponding names here

    if (use_tempfiles) {
        FILE *testfile;
        int count = 0;

        outfilename_temp = malloc (strlen (outfilename) + 16);

        if (out2filename)
            out2filename_temp = malloc (strlen (outfilename) + 16);

        while (1) {
            strcpy (outfilename_temp, outfilename);

            if (filespec_ext (outfilename_temp)) {
                if (count++)
                    sprintf (filespec_ext (outfilename_temp), ".tmp%d", count-1);
                else
                    strcpy (filespec_ext (outfilename_temp), ".tmp");

                strcat (outfilename_temp, filespec_ext (outfilename));
            }
            else {
                if (count++)
                    sprintf (outfilename_temp + strlen (outfilename_temp), ".tmp%d", count-1);
                else
                    strcat (outfilename_temp, ".tmp");
            }

            testfile = fopen (outfilename_temp, "rb");

            if (testfile) {
                int res = (int) fread (&dummy, 1, 1, testfile);

                fclose (testfile);

                if (res == 1)
                    continue;
            }

            if (out2filename) {
                strcpy (out2filename_temp, outfilename_temp);
                strcat (out2filename_temp, "c");

                testfile = fopen (out2filename_temp, "rb");

                if (testfile) {
                    int res = (int) fread (&dummy, 1, 1, testfile);

                    fclose (testfile);

                    if (res == 1)
                        continue;
                }
            }   

            break;
        }
    }

#if defined(_WIN32)
    _ftime64 (&time1);
#else
    gettimeofday(&time1,&timez);
#endif

    // open output file for writing

    if (*outfilename == '-') {
        wv_file.file = stdout;
#if defined(_WIN32)
        _setmode (_fileno (stdout), O_BINARY);
#endif
#if defined(__OS2__)
        setmode (fileno (stdout), O_BINARY);
#endif
    }
    else if ((wv_file.file = fopen (use_tempfiles ? outfilename_temp : outfilename, "w+b")) == NULL) {
        error_line ("can't create file %s!", use_tempfiles ? outfilename_temp : outfilename);
        DoCloseHandle (infile);
        WavpackCloseFile (wpc);
        return WAVPACK_SOFT_ERROR;
    }

    if (!quiet_mode) {
        if (*outfilename == '-')
            fprintf (stderr, "packing %s to stdout,", *infilename == '-' ? "stdin" : FN_FIT (infilename));
        else if (out2filename)
            fprintf (stderr, "creating %s (+%s),", FN_FIT (outfilename), filespec_ext (out2filename));
        else
            fprintf (stderr, "creating %s,", FN_FIT (outfilename));

        fflush (stderr);
    }

    // for now, raw 1-bit PCM is only DSDIFF format

    if (loc_config.qmode & QMODE_RAW_PCM)
        if (loc_config.qmode & QMODE_DSD_AUDIO)
            WavpackSetFileInformation (wpc, "dff", WP_FORMAT_DFF);

    // if not in "raw" mode, process RIFF form header and set configuration

    if (!(loc_config.qmode & QMODE_RAW_PCM)) {
        char fourcc [4];
        int i;

        if (!DoReadFile (infile, fourcc, sizeof (fourcc), &bcount) || bcount != sizeof (fourcc)) {
            error_line ("can't read file %s!", infilename);
            DoCloseHandle (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (wpc);
            return WAVPACK_SOFT_ERROR;
        }

        for (i = 0; i < NUM_FILE_FORMATS; ++i)
            if (!strncmp (fourcc, file_formats [i].fourcc, 4)) {

                WavpackSetFileInformation (wpc,
                    filespec_ext (infilename) ? filespec_ext (infilename) + 1 : file_formats [i].default_extension,
                    file_formats [i].id);

                if (file_formats [i].ParseHeader (infile, infilename, fourcc, wpc, &loc_config)) {
                    DoCloseHandle (infile);
                    DoCloseHandle (wv_file.file);
                    DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                    WavpackCloseFile (wpc);
                    return WAVPACK_SOFT_ERROR;
                }

                chunk_alignment = file_formats [i].chunk_alignment;
                break;
            }

        if (i == NUM_FILE_FORMATS)  {
            error_line ("%s is not a recognized file type!", infilename);
            DoCloseHandle (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (wpc);
            return WAVPACK_SOFT_ERROR;
        }
    }
    else if (raw_pcm_skip_bytes_begin) {          // if raw pcm mode and bytes to skip, do that here
        int bytes_to_skip = raw_pcm_skip_bytes_begin;
        char dummy [256];

        while (bytes_to_skip) {
            int requested_bytes = (bytes_to_skip >= sizeof (dummy)) ? sizeof (dummy) : bytes_to_skip;

            if (DoReadFile (infile, dummy, requested_bytes, &bcount) && bcount == requested_bytes)
                bytes_to_skip -= bcount;
            else
                break;
        }

        if (bytes_to_skip) {
            error_line ("can't read file %s!", infilename);
            DoCloseHandle (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (wpc);
            return WAVPACK_SOFT_ERROR;
        }
    }

    // handle case where the CAF header indicated a channel layout that requires reordering

    if (loc_config.qmode & QMODE_REORDERED_CHANS) {
        int layout = WavpackGetChannelLayout (wpc, NULL), i;

        if ((layout & 0xff) <= loc_config.num_channels) {
            new_channel_order = malloc (loc_config.num_channels);

            for (i = 0; i < loc_config.num_channels; ++i)
                new_channel_order [i] = i;

            WavpackGetChannelLayout (wpc, new_channel_order);
        }
    }

    // handle case where the user specified channel configuration on the command-line

    if (num_channels_order || (loc_config.qmode & QMODE_CHANS_UNASSIGNED)) {
        int i, j;

        if (loc_config.num_channels < num_channels_order ||
            (loc_config.num_channels > num_channels_order && !(loc_config.qmode & QMODE_CHANS_UNASSIGNED))) {
                error_line ("file does not have %d channel(s)!", num_channels_order);
                DoCloseHandle (infile);
                DoCloseHandle (wv_file.file);
                DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                WavpackCloseFile (wpc);
                return WAVPACK_SOFT_ERROR;
            }

        if (num_channels_order) {
            new_channel_order = malloc (loc_config.num_channels);

            for (i = 0; i < loc_config.num_channels; ++i)
                new_channel_order [i] = i;

            memcpy (new_channel_order, channel_order, num_channels_order);

            for (i = 0; i < num_channels_order;) {
                for (j = 0; j < num_channels_order; ++j)
                    if (new_channel_order [j] == i) {
                        i++;
                        break;
                    }

                if (j == num_channels_order)
                    for (j = 0; j < num_channels_order; ++j)
                        if (new_channel_order [j] > i)
                            new_channel_order [j]--;
            }
        }
    }

    // if we are creating a "correction" file, open it now for writing

    if (out2filename) {
        if ((wvc_file.file = fopen (use_tempfiles ? out2filename_temp : out2filename, "w+b")) == NULL) {
            error_line ("can't create correction file!");
            DoCloseHandle (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (wpc);
            return WAVPACK_SOFT_ERROR;
        }
    }

    // pack the audio portion of the file now; calculate md5 if we're writing it to the file or verify mode is active

    if (loc_config.qmode & QMODE_DSD_AUDIO)
        result = pack_dsd_audio (wpc, infile, loc_config.qmode, new_channel_order, ((loc_config.flags & CONFIG_MD5_CHECKSUM) || verify_mode) ? md5_digest : NULL);
    else
        result = pack_audio (wpc, infile, loc_config.qmode, new_channel_order, ((loc_config.flags & CONFIG_MD5_CHECKSUM) || verify_mode) ? md5_digest : NULL);

    if (new_channel_order)
        free (new_channel_order);

    // write the md5 sum if the user asked for it to be included

    if (result == WAVPACK_NO_ERROR && (loc_config.flags & CONFIG_MD5_CHECKSUM))
        WavpackStoreMD5Sum (wpc, md5_digest);

    // if everything went well, and we're not ignoring length or encoding raw
    // pcm, read past any required data chunk padding and then try to read anything
    // else that might be appended to the audio data and write that to the WavPack
    // metadata as "wrapper"

    if (result == WAVPACK_NO_ERROR && !(loc_config.qmode & (QMODE_IGNORE_LENGTH | QMODE_RAW_PCM))) {
        int wrapper_size = 0, buffer_size;
        unsigned char *buffer;

        // if this file format has chunk alignment padding, read past that here

        if (chunk_alignment != 1) {
            int64_t data_chunk_bytes = WavpackGetNumSamples64 (wpc) * WavpackGetNumChannels (wpc) * WavpackGetBytesPerSample (wpc);
            int bytes_over = (int)(data_chunk_bytes % chunk_alignment);
            int padding_bytes = bytes_over ? chunk_alignment - bytes_over : 0;
            unsigned char pad_byte;

            while (padding_bytes--) {
                if (!DoReadFile (infile, &pad_byte, 1, &bcount) || bcount != 1)
                    error_line ("warning: input file missing required padding byte!");
                else if (pad_byte)
                    error_line ("warning: input file has non-zero padding byte!");
            }
        }

        // now read everything remaining in the file into a new buffer

        buffer = malloc (buffer_size = 65536);

        while (DoReadFile (infile, buffer + wrapper_size, buffer_size - wrapper_size, &bcount) && bcount)
            if ((wrapper_size += bcount) == buffer_size)
                buffer = realloc (buffer, buffer_size += 65536);

        // if we got something and are storing wrapper, write it to the outfile file

        if (wrapper_size && !(loc_config.qmode & QMODE_NO_STORE_WRAPPER) && !WavpackAddWrapper (wpc, buffer, wrapper_size)) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            result = WAVPACK_HARD_ERROR;
        }

        // if we're supposed to try to import ID3 tags, check for and do that now
        // (but only error on a bad tag, not just a missing one or one with no applicable items)

        if (result == WAVPACK_NO_ERROR && import_id3 && wrapper_size > 10 && !strncmp ((char *) buffer, "ID3", 3)) {
            int32_t bytes_used, id3_res;
            char error [80];

            // first we do a "dry run" pass through the ID3 tag, and only if that passes do we try to write the tag items

            id3_res = ImportID3v2 (NULL, buffer, wrapper_size, error, &bytes_used);

            if (!allow_huge_tags && bytes_used > 1048576) {
                error_line ("imported tag items exceed 1 MB, use --allow-huge-tags to override");
                result = WAVPACK_SOFT_ERROR;
            }
            else if (bytes_used > 1048576 * 16) {
                error_line ("imported tag items exceed 16 MB");
                result = WAVPACK_SOFT_ERROR;
            }
            else {
                if (id3_res > 0)
                    id3_res = ImportID3v2 (wpc, buffer, wrapper_size, error, NULL);

                if (id3_res < 0) {
                    error_line ("ID3v2 import: %s", error);
                    result = WAVPACK_SOFT_ERROR;
                }
                else if (id3_res > 0)
                    imported_tag_items = id3_res;
            }
        }

        free (buffer);
    }

    DoCloseHandle (infile);     // we're now done with input file, so close

    // we're now done with any WavPack blocks, so flush any remaining data

    if (result == WAVPACK_NO_ERROR && !WavpackFlushSamples (wpc)) {
        error_line ("%s", WavpackGetErrorMessage (wpc));
        result = WAVPACK_HARD_ERROR;
    }

    // if still no errors, check to see if we need to create & write a tag
    // (which is NOT stored in regular WavPack blocks)

    if (result == WAVPACK_NO_ERROR && (num_tag_items || imported_tag_items)) {
        int i, res = TRUE;

        for (i = 0; i < num_tag_items && res; ++i)
            if (tag_items [i].vsize) {
                if (tag_items [i].binary) 
                    res = WavpackAppendBinaryTagItem (wpc, tag_items [i].item, tag_items [i].value, tag_items [i].vsize);
                else
                    res = WavpackAppendTagItem (wpc, tag_items [i].item, tag_items [i].value, tag_items [i].vsize);
            }

        if (!res || !WavpackWriteTag (wpc)) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            result = WAVPACK_HARD_ERROR;
        }
    }

    // At this point we're done writing to the output files. However, in some
    // situations we might have to back up and re-write the initial block. Currently
    // the only case is if we're ignoring length or reading raw pcm data from stdin
    // (which sets the ignore-length flag); otherwise it's an error.

    if (result == WAVPACK_NO_ERROR && WavpackGetNumSamples64 (wpc) != WavpackGetSampleIndex64 (wpc)) {
        if (loc_config.qmode & QMODE_IGNORE_LENGTH) {
            char *block_buff = malloc (wv_file.first_block_size);

            if (block_buff && !DoSetFilePositionAbsolute (wv_file.file, 0) &&
                DoReadFile (wv_file.file, block_buff, wv_file.first_block_size, &bcount) &&
                bcount == wv_file.first_block_size && !strncmp (block_buff, "wvpk", 4)) {

                    // this call will take care of the initial WavPack header and any RIFF header the library made

                    WavpackUpdateNumSamples (wpc, block_buff);

                    if (DoSetFilePositionAbsolute (wv_file.file, 0) ||
                        !DoWriteFile (wv_file.file, block_buff, wv_file.first_block_size, &bcount) ||
                        bcount != wv_file.first_block_size) {
                            error_line ("couldn't update WavPack header with actual length!!");
                            result = WAVPACK_SOFT_ERROR;
                    }
            }
            else {
                error_line ("couldn't update WavPack header with actual length!!");
                result = WAVPACK_SOFT_ERROR;
            }

            if (block_buff)
                free (block_buff);

            if (result == WAVPACK_NO_ERROR && wvc_file.file) {
                block_buff = malloc (wvc_file.first_block_size);

                if (block_buff && !DoSetFilePositionAbsolute (wvc_file.file, 0) &&
                    DoReadFile (wvc_file.file, block_buff, wvc_file.first_block_size, &bcount) &&
                    bcount == wvc_file.first_block_size && !strncmp (block_buff, "wvpk", 4)) {

                        WavpackUpdateNumSamples (wpc, block_buff);

                        if (DoSetFilePositionAbsolute (wvc_file.file, 0) ||
                            !DoWriteFile (wvc_file.file, block_buff, wvc_file.first_block_size, &bcount) ||
                            bcount != wvc_file.first_block_size) {
                                error_line ("couldn't update WavPack header with actual length!!");
                                result = WAVPACK_SOFT_ERROR;
                        }
                }
                else {
                    error_line ("couldn't update WavPack header with actual length!!");
                    result = WAVPACK_SOFT_ERROR;
                }

                if (block_buff)
                    free (block_buff);
            }
        }
        else {
            error_line ("couldn't read all samples, file may be corrupt!!");
            result = WAVPACK_SOFT_ERROR;
        }
    }

    // at this point we're completely done with the files, so close 'em whether there
    // were any other errors or not

    if (!DoCloseHandle (wv_file.file)) {
        error_line ("can't close WavPack file!");

        if (result == WAVPACK_NO_ERROR)
            result = WAVPACK_SOFT_ERROR;
    }

    if (out2filename && !DoCloseHandle (wvc_file.file)) {
        error_line ("can't close correction file!");

        if (result == WAVPACK_NO_ERROR)
            result = WAVPACK_SOFT_ERROR;
    }

    // if there have been no errors up to now, and verify mode is enabled, do that now; only pass in the md5 if this
    // was a lossless operation (either explicitly or because a high lossy bitrate resulted in lossless)

    if (result == WAVPACK_NO_ERROR && verify_mode)
        result = verify_audio (use_tempfiles ? outfilename_temp : outfilename, !WavpackLossyBlocks (wpc) ? md5_digest : NULL);

    // if there were any errors, delete the output files, close the context, and return the error

    if (result != WAVPACK_NO_ERROR) {
        DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);

        if (out2filename)
            DoDeleteFile (use_tempfiles ? out2filename_temp : out2filename);

        WavpackCloseFile (wpc);
        return result;
    }

    // if we were writing to a temp file because the target file already existed,
    // do the rename / overwrite now (and if that fails, return the error)

    if (use_tempfiles) {
#if defined(_WIN32)
        FILE *temp;

        if (remove (outfilename) && (temp = fopen (outfilename, "rb"))) {
            error_line ("can not remove file %s, result saved in %s!", outfilename, outfilename_temp);
            result = WAVPACK_SOFT_ERROR;
            fclose (temp);
        }
        else
#endif
        if (rename (outfilename_temp, outfilename)) {
            error_line ("can not rename temp file %s to %s!", outfilename_temp, outfilename);
            result = WAVPACK_SOFT_ERROR;
        }

        if (out2filename) {
#if defined(_WIN32)
            FILE *temp;

            if (remove (out2filename) && (temp = fopen (out2filename, "rb"))) {
                error_line ("can not remove file %s, result saved in %s!", out2filename, out2filename_temp);
                result = WAVPACK_SOFT_ERROR;
                fclose (temp);
            }
            else
#endif
            if (rename (out2filename_temp, out2filename)) {
                error_line ("can not rename temp file %s to %s!", out2filename_temp, out2filename);
                result = WAVPACK_SOFT_ERROR;
            }
        }

        free (outfilename_temp);
        if (out2filename) free (out2filename_temp);

        if (result != WAVPACK_NO_ERROR) {
            WavpackCloseFile (wpc);
            return result;
        }
    }

    if (result == WAVPACK_NO_ERROR && copy_time)
        if (!copy_timestamp (infilename, outfilename) ||
            (out2filename && !copy_timestamp (infilename, out2filename)))
                error_line ("failure copying time stamp!");

    // delete source file if that option is enabled

    if (result == WAVPACK_NO_ERROR && delete_source) {
        int res = DoDeleteFile (infilename);

        if (!quiet_mode || !res)
            error_line ("%s source file %s", res ?
                "deleted" : "can't delete", infilename);
    }

    // compute and display the time consumed along with some other details of
    // the packing operation, and then return WAVPACK_NO_ERROR

#if defined(_WIN32)
    _ftime64 (&time2);
    dtime = time2.time + time2.millitm / 1000.0;
    dtime -= time1.time + time1.millitm / 1000.0;
#else
    gettimeofday(&time2,&timez);
    dtime = time2.tv_sec + time2.tv_usec / 1000000.0;
    dtime -= time1.tv_sec + time1.tv_usec / 1000000.0;
#endif

    if ((loc_config.flags & CONFIG_CALC_NOISE) && WavpackGetEncodedNoise (wpc, NULL) > 0.0) {
        int full_scale_bits = WavpackGetBitsPerSample (wpc);
        double full_scale_rms = 0.5, sum, peak;

        while (full_scale_bits--)
            full_scale_rms *= 2.0;

        full_scale_rms = full_scale_rms * (full_scale_rms - 1.0) * 0.5;
        sum = WavpackGetEncodedNoise (wpc, &peak);

        error_line ("ave noise = %.2f dB, peak noise = %.2f dB",
            log10 (sum / WavpackGetNumSamples64 (wpc) / full_scale_rms) * 10,
            log10 (peak / full_scale_rms) * 10);
    }

    if (!quiet_mode) {
        char *file, *fext, *oper, *cmode, cratio [16] = "";

        if (imported_tag_items)
            error_line ("successfully imported %d items from ID3v2 tag", imported_tag_items);

        if (loc_config.flags & CONFIG_MD5_CHECKSUM) {
            char md5_string [] = "original md5 signature: 00000000000000000000000000000000";
            int i;

            for (i = 0; i < 16; ++i)
                sprintf (md5_string + 24 + (i * 2), "%02x", md5_digest [i]);

            error_line (md5_string);
        }

        if (outfilename && *outfilename != '-') {
            file = FN_FIT (outfilename);
            fext = wvc_file.bytes_written ? " (+.wvc)" : "";
            oper = verify_mode ? "created (and verified)" : "created";
        }
        else {
            file = (*infilename == '-') ? "stdin" : FN_FIT (infilename);
            fext = "";
            oper = "packed";
        }

        if (WavpackLossyBlocks (wpc)) {
            cmode = "lossy";

            if (WavpackGetAverageBitrate (wpc, TRUE) != 0.0)
                sprintf (cratio, ", %d kbps", (int) (WavpackGetAverageBitrate (wpc, TRUE) / 1000.0));
        }
        else {
            cmode = "lossless";

            if (WavpackGetRatio (wpc) != 0.0)
                sprintf (cratio, ", %.2f%%", 100.0 - WavpackGetRatio (wpc) * 100.0);
        }

        error_line ("%s %s%s in %.2f secs (%s%s)", oper, file, fext, dtime, cmode, cratio);
    }

    WavpackCloseFile (wpc);
    return WAVPACK_NO_ERROR;
}

// This function handles the actual audio data compression. It assumes that the
// input file is positioned at the beginning of the audio data and that the
// WavPack configuration has been set. This is where the conversion from RIFF
// little-endian standard the executing processor's format is done and where
// (if selected) the MD5 sum is calculated and displayed.

static void reorder_channels (void *data, unsigned char *new_order, int num_chans,
    int num_samples, int bytes_per_sample);

static void load_samples (int32_t *dst, void *src, int qmode, int bps, int count);
static void *store_samples (void *dst, int32_t *src, int qmode, int bps, int count);
static void unreorder_channels (int32_t *data, unsigned char *order, int num_chans, int num_samples);

#define INPUT_SAMPLES 65536

static int pack_audio (WavpackContext *wpc, FILE *infile, int qmode, unsigned char *new_order, unsigned char *md5_digest_source)
{
    int64_t samples_remaining, input_samples = INPUT_SAMPLES;
    double progress = -1.0;
    int bytes_per_sample;
    int32_t *sample_buffer;
    unsigned char *input_buffer;
    MD5_CTX md5_context;
    int32_t quantize_bit_mask = 0;
    double fquantize_scale = 1.0, fquantize_iscale = 1.0;

    // don't use an absurd amount of memory just because we have an absurd number of channels

    while (input_samples * sizeof (int32_t) * WavpackGetNumChannels (wpc) > 2048*1024)
        input_samples >>= 1;

    if (md5_digest_source)
        MD5Init (&md5_context);

    WavpackPackInit (wpc);
    bytes_per_sample = WavpackGetBytesPerSample (wpc) * WavpackGetNumChannels (wpc);
    input_buffer = malloc ((uint32_t) input_samples * bytes_per_sample);
    sample_buffer = malloc ((uint32_t) input_samples * sizeof (int32_t) * WavpackGetNumChannels (wpc));
    samples_remaining = WavpackGetNumSamples64 (wpc);

    if (quantize_bits && quantize_bits < WavpackGetBytesPerSample (wpc) * 8) {
        quantize_bit_mask = ~((1<<(WavpackGetBytesPerSample (wpc)*8-quantize_bits))-1);
        if (MODE_FLOAT == (WavpackGetMode(wpc) & MODE_FLOAT)) {
            int float_norm_exp = WavpackGetFloatNormExp (wpc);
            fquantize_scale = exp2 (quantize_bits + 126 - float_norm_exp);
            fquantize_iscale = exp2 (float_norm_exp - 126 - quantize_bits);
        }
    }

    while (1) {
        uint32_t bytes_to_read, bytes_read = 0;
        int32_t sample_count;

        if ((qmode & QMODE_IGNORE_LENGTH) || samples_remaining > input_samples)
            bytes_to_read = (uint32_t) input_samples * bytes_per_sample;
        else
            bytes_to_read = (uint32_t) samples_remaining * bytes_per_sample;

        samples_remaining -= bytes_to_read / bytes_per_sample;
        DoReadFile (infile, input_buffer, bytes_to_read, &bytes_read);
        sample_count = bytes_read / bytes_per_sample;

        // if we have reordering to do because the user used the --channel-order option to define
        // an order that does not match the Microsoft order, then we do that BEFORE the MD5 because
        // this reordering is permanent (i.e., we will not unreorder on decode) and we want the
        // MD5 to match the new order

        if (new_order && !(qmode & QMODE_REORDERED_CHANS))
            reorder_channels (input_buffer, new_order, WavpackGetNumChannels (wpc),
                sample_count, WavpackGetBytesPerSample (wpc));

        if (md5_digest_source && quantize_bit_mask == 0)
            MD5Update (&md5_context, input_buffer, sample_count * bytes_per_sample);

        // if we have reordering to do because this is a CAF channel layout that is not in Microsoft
        // order, then we do the reordering AFTER the MD5 because we will be unreordering them at
        // decode time, and so we want the MD5 to match the orginal order

        if (new_order && (qmode & QMODE_REORDERED_CHANS))
            reorder_channels (input_buffer, new_order, WavpackGetNumChannels (wpc),
                sample_count, WavpackGetBytesPerSample (wpc));

        if (!sample_count)
            break;

        if (sample_count) {
            int bps = WavpackGetBytesPerSample (wpc);

            load_samples (sample_buffer, input_buffer, qmode, bps, sample_count * WavpackGetNumChannels (wpc));

            if (quantize_bit_mask) {
                unsigned int x,l = sample_count * WavpackGetNumChannels (wpc);
                if (0 == (WavpackGetMode(wpc) & MODE_FLOAT)) {
                    if (quantize_round) {
                        int32_t offset = (quantize_bit_mask >> 1) ^ quantize_bit_mask;
                        int shift = 32 - WavpackGetBytesPerSample (wpc) * 8;

                        for (x = 0; x < l; x ++)
                            if (sample_buffer[x] < 0 || ((sample_buffer[x] + offset) << shift) > 0)
                                sample_buffer[x] += offset;
                    }

                    for (x = 0; x < l; x ++) sample_buffer[x] &= quantize_bit_mask;
                }
                else {
                    for (x = 0; x < l; x ++) {
                        const float f = *(float *)&sample_buffer[x];
                        *(float *)&sample_buffer[x] = (float) (floor(f * fquantize_scale + 0.5) * fquantize_iscale);
                    }
                }

                if (md5_digest_source) {
                    store_samples (input_buffer, sample_buffer, qmode, bps, sample_count * WavpackGetNumChannels (wpc));
                    MD5Update (&md5_context, input_buffer, WavpackGetBytesPerSample (wpc) * l);
                }
            }
        }

        if (!WavpackPackSamples (wpc, sample_buffer, sample_count)) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            free (sample_buffer);
            free (input_buffer);
            return WAVPACK_HARD_ERROR;
        }

        if (check_break ()) {
#if defined(_WIN32)
            fprintf (stderr, "^C\n");
#else
            fprintf (stderr, "\n");
#endif
            fflush (stderr);
            free (sample_buffer);
            free (input_buffer);
            return WAVPACK_SOFT_ERROR;
        }

        if (WavpackGetProgress (wpc) != -1.0 &&
            progress != floor (WavpackGetProgress (wpc) * encode_time_percent + 0.5)) {
                int nobs = progress == -1.0;

                progress = floor (WavpackGetProgress (wpc) * encode_time_percent + 0.5);
                display_progress (progress / 100.0);

                if (!quiet_mode) {
                    fprintf (stderr, "%s%3d%% done...",
                        nobs ? " " : "\b\b\b\b\b\b\b\b\b\b\b\b", (int) progress);
                    fflush (stderr);
                }
        }
    }

    free (sample_buffer);
    free (input_buffer);

    if (!WavpackFlushSamples (wpc)) {
        error_line ("%s", WavpackGetErrorMessage (wpc));
        return WAVPACK_HARD_ERROR;
    }

    if (md5_digest_source)
        MD5Final (md5_digest_source, &md5_context);

    return WAVPACK_NO_ERROR;
}

static const unsigned char bit_reverse_table [] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

#define DSD_BLOCKSIZE 4096

static int pack_dsd_audio (WavpackContext *wpc, FILE *infile, int qmode, unsigned char *new_order, unsigned char *md5_digest_source)
{
    int64_t samples_remaining;
    double progress = -1.0;
    int num_channels;
    int32_t *sample_buffer;
    unsigned char *input_buffer;
    MD5_CTX md5_context;

    if (md5_digest_source)
        MD5Init (&md5_context);

    WavpackPackInit (wpc);
    num_channels = WavpackGetNumChannels (wpc);
    input_buffer = malloc (DSD_BLOCKSIZE * num_channels);
    sample_buffer = malloc (DSD_BLOCKSIZE * sizeof (int32_t) * num_channels);
    samples_remaining = WavpackGetNumSamples64 (wpc);

    while (samples_remaining) {
        uint32_t bytes_to_read, bytes_read = 0;
        int32_t sample_count;

        if ((qmode & QMODE_DSD_IN_BLOCKS) || samples_remaining > DSD_BLOCKSIZE)
            bytes_to_read = DSD_BLOCKSIZE * num_channels;
        else
            bytes_to_read = (uint32_t) samples_remaining * num_channels;

        DoReadFile (infile, input_buffer, bytes_to_read, &bytes_read);

        if (qmode & QMODE_DSD_IN_BLOCKS) {
            if (bytes_read != bytes_to_read) {
                error_line ("incomplete DSD block!");
                samples_remaining = sample_count = 0;
            }
            else if (samples_remaining < DSD_BLOCKSIZE)
                sample_count = (int32_t) samples_remaining;
            else
                sample_count = DSD_BLOCKSIZE;
        }
        else
            sample_count = bytes_read / num_channels;

        samples_remaining -= sample_count;

        // if we have reordering to do because the user used the --channel-order option to define
        // an order that does not match the Microsoft order, then we do that BEFORE the MD5 because
        // this reordering is permanent (i.e., we will not unreorder on decode) and we want the
        // MD5 to match the new order

        if (new_order && !(qmode & QMODE_REORDERED_CHANS)) {
            if (qmode & QMODE_DSD_IN_BLOCKS)
                reorder_channels (input_buffer, new_order, num_channels, 1, DSD_BLOCKSIZE);
            else
                reorder_channels (input_buffer, new_order, num_channels, sample_count, 1);
        }

        if (md5_digest_source)
            MD5Update (&md5_context, input_buffer, bytes_read);

        if (!sample_count)
            break;

        if (sample_count) {
            if (qmode & QMODE_DSD_IN_BLOCKS) {
                int32_t sindex, *sptr = sample_buffer, non_null = 0;

                for (sindex = 0; sindex < DSD_BLOCKSIZE; ++sindex) {
                    unsigned char *srcp = input_buffer + sindex;
                    int cc;

                    for (cc = num_channels; cc--; srcp += DSD_BLOCKSIZE)
                        if (sindex < sample_count)
                            *sptr++ = (qmode & QMODE_DSD_LSB_FIRST) ? bit_reverse_table [*srcp] : *srcp;
                        else if (*srcp)
                            non_null++;
                }

                if (non_null)
                    error_line ("blocks not padded with NULLs, MD5 will not match!");
            }
            else {
                int32_t scount = sample_count * num_channels, *sptr = sample_buffer;
                unsigned char *iptr = input_buffer;

                while (scount--)
                    *sptr++ = *iptr++;
            }
        }

        if (!WavpackPackSamples (wpc, sample_buffer, sample_count)) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            free (sample_buffer);
            free (input_buffer);
            return WAVPACK_HARD_ERROR;
        }

        if (check_break ()) {
#if defined(_WIN32)
            fprintf (stderr, "^C\n");
#else
            fprintf (stderr, "\n");
#endif
            fflush (stderr);
            free (sample_buffer);
            free (input_buffer);
            return WAVPACK_SOFT_ERROR;
        }

        if (WavpackGetProgress (wpc) != -1.0 &&
            progress != floor (WavpackGetProgress (wpc) * encode_time_percent + 0.5)) {
                int nobs = progress == -1.0;

                progress = floor (WavpackGetProgress (wpc) * encode_time_percent + 0.5);
                display_progress (progress / 100.0);

                if (!quiet_mode) {
                    fprintf (stderr, "%s%3d%% done...",
                        nobs ? " " : "\b\b\b\b\b\b\b\b\b\b\b\b", (int) progress);
                    fflush (stderr);
                }
        }
    }

    free (sample_buffer);
    free (input_buffer);

    if (!WavpackFlushSamples (wpc)) {
        error_line ("%s", WavpackGetErrorMessage (wpc));
        return WAVPACK_HARD_ERROR;
    }

    if (md5_digest_source)
        MD5Final (md5_digest_source, &md5_context);

    return WAVPACK_NO_ERROR;
}

// This function transcodes a single WavPack file "infilename" and stores the resulting
// WavPack file at "outfilename". If "out2filename" is specified, then the "correction"
// file would go there. The files are opened and closed in this function and the "config"
// structure specifies the mode of compression. Note that lossy to lossless transcoding
// is not allowed (no technical reason, it's just dumb, and could result in files that
// fail their MD5 verification test).

static int repack_file (char *infilename, char *outfilename, char *out2filename, const WavpackConfig *config)
{
    int output_lossless = !(config->flags & CONFIG_HYBRID_FLAG) || (config->flags & CONFIG_CREATE_WVC);
    int flags = OPEN_WVC | OPEN_TAGS | OPEN_DSD_NATIVE | OPEN_ALT_TYPES, imported_tag_items = 0;
    char *outfilename_temp = NULL, *out2filename_temp = NULL;
    int use_tempfiles = (out2filename != NULL), input_mode;
    unsigned char md5_verify [16], md5_display [16];
    WavpackConfig loc_config = *config;
    WavpackContext *infile, *outfile;
    write_id wv_file, wvc_file;
    int64_t total_samples = 0;
    unsigned char *chan_ids;
    char error [80];
    double dtime;
    int result;

#if defined(_WIN32)
    struct __timeb64 time1, time2;
#else
    struct timeval time1, time2;
    struct timezone timez;
#endif

    if (!(loc_config.qmode & QMODE_NO_STORE_WRAPPER) || import_id3)
        flags |= OPEN_WRAPPER;

#if defined(_WIN32)
    flags |= OPEN_FILE_UTF8;
#endif

    // use library to open input WavPack file

    infile = WavpackOpenFileInput (infilename, error, flags, 0);

    if (!infile) {
        error_line (error);
        return WAVPACK_SOFT_ERROR;
    }

    input_mode = WavpackGetMode (infile);

    if (!(input_mode & MODE_LOSSLESS) && output_lossless) {
        error_line ("can't transcode lossy file %s to lossless...not allowed!", infilename);
        WavpackCloseFile (infile);
        return WAVPACK_SOFT_ERROR;
    }

    total_samples = WavpackGetNumSamples64 (infile);

    if (total_samples == -1) {
        error_line ("can't transcode file %s of unknown length!", infilename);
        WavpackCloseFile (infile);
        return WAVPACK_SOFT_ERROR;
    }

    // open an output context

    CLEAR (wv_file);
    CLEAR (wvc_file);
    outfile = WavpackOpenFileOutput (write_block, &wv_file, out2filename ? &wvc_file : NULL);

    // check both output files for overwrite warning required

    if (*outfilename != '-' && (wv_file.file = fopen (outfilename, "rb")) != NULL) {
        DoCloseHandle (wv_file.file);
        use_tempfiles = 1;

        if (!overwrite_all) {
            if (output_lossless)
                fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (outfilename));
            else
                fprintf (stderr, "overwrite %s with lossy transcode (yes/no/all)? ", FN_FIT (outfilename));

            fflush (stderr);

            if (set_console_title)
                DoSetConsoleTitle ("overwrite?");

            switch (yna ()) {
                case 'n':
                    WavpackCloseFile (infile);
                    WavpackCloseFile (outfile);
                    return WAVPACK_SOFT_ERROR;

                case 'a':
                    overwrite_all = 1;
            }
        }
    }

    if (out2filename && !overwrite_all && (wvc_file.file = fopen (out2filename, "rb")) != NULL) {
        DoCloseHandle (wvc_file.file);
        fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (out2filename));
        fflush (stderr);

        if (set_console_title)
            DoSetConsoleTitle ("overwrite?");

        switch (yna ()) {

            case 'n':
                WavpackCloseFile (infile);
                WavpackCloseFile (outfile);
                return WAVPACK_SOFT_ERROR;

            case 'a':
                overwrite_all = 1;
        }
    }

    // if we are using temp files, either because the output filename already exists or we are creating a
    // "correction" file, search for and generate the corresponding names here

    if (use_tempfiles) {
        FILE *testfile;
        int count = 0;

        outfilename_temp = malloc (strlen (outfilename) + 16);

        if (out2filename)
            out2filename_temp = malloc (strlen (outfilename) + 16);

        while (1) {
            strcpy (outfilename_temp, outfilename);

            if (filespec_ext (outfilename_temp)) {
                if (count++)
                    sprintf (filespec_ext (outfilename_temp), ".tmp%d", count-1);
                else
                    strcpy (filespec_ext (outfilename_temp), ".tmp");

                strcat (outfilename_temp, filespec_ext (outfilename));
            }
            else {
                if (count++)
                    sprintf (outfilename_temp + strlen (outfilename_temp), ".tmp%d", count-1);
                else
                    strcat (outfilename_temp, ".tmp");
            }

            testfile = fopen (outfilename_temp, "rb");

            if (testfile) {
                fclose (testfile);
                continue;
            }

            if (out2filename) {
                strcpy (out2filename_temp, outfilename_temp);
                strcat (out2filename_temp, "c");

                testfile = fopen (out2filename_temp, "rb");

                if (testfile) {
                    fclose (testfile);
                    continue;
                }
            }   

            break;
        }
    }

#if defined(_WIN32)
    _ftime64 (&time1);
#else
    gettimeofday(&time1,&timez);
#endif

    // open output file for writing

    if (*outfilename == '-') {
        wv_file.file = stdout;
#if defined(_WIN32)
        _setmode (_fileno (stdout), O_BINARY);
#endif
#if defined(__OS2__)
        setmode (fileno (stdout), O_BINARY);
#endif
    }
    else if ((wv_file.file = fopen (use_tempfiles ? outfilename_temp : outfilename, "w+b")) == NULL) {
        error_line ("can't create file %s!", use_tempfiles ? outfilename_temp : outfilename);
        WavpackCloseFile (infile);
        WavpackCloseFile (outfile);
        return WAVPACK_SOFT_ERROR;
    }

    if (!quiet_mode) {
        if (*outfilename == '-')
            fprintf (stderr, "packing %s to stdout,", *infilename == '-' ? "stdin" : FN_FIT (infilename));
        else if (out2filename)
            fprintf (stderr, "creating %s (+%s),", FN_FIT (outfilename), filespec_ext (out2filename));
        else
            fprintf (stderr, "creating %s,", FN_FIT (outfilename));

        fflush (stderr);
    }

    WavpackSetFileInformation (outfile, WavpackGetFileExtension (infile), WavpackGetFileFormat (infile));

    // unless we've been specifically told not to, copy RIFF header

    if (WavpackGetWrapperBytes (infile)) {
        if (!(loc_config.qmode & QMODE_NO_STORE_WRAPPER) && !WavpackAddWrapper (outfile, WavpackGetWrapperData (infile), WavpackGetWrapperBytes (infile))) {
            error_line ("%s", WavpackGetErrorMessage (outfile));
            WavpackCloseFile (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (outfile);
            return WAVPACK_SOFT_ERROR;
        }

        WavpackFreeWrapper (infile);
    }

    loc_config.bytes_per_sample = WavpackGetBytesPerSample (infile);
    loc_config.bits_per_sample = WavpackGetBitsPerSample (infile);
    loc_config.channel_mask = WavpackGetChannelMask (infile);
    loc_config.num_channels = WavpackGetNumChannels (infile);
    loc_config.sample_rate = WavpackGetSampleRate (infile);
    loc_config.qmode |= WavpackGetQualifyMode (infile);
    chan_ids = malloc (loc_config.num_channels + 1);
    WavpackGetChannelIdentities (infile, chan_ids);

    if (input_mode & MODE_FLOAT)
        loc_config.float_norm_exp = WavpackGetFloatNormExp (infile);

    if (input_mode & MODE_MD5)
        loc_config.flags |= CONFIG_MD5_CHECKSUM;

    if (!WavpackSetConfiguration64 (outfile, &loc_config, total_samples, chan_ids)) {
        error_line ("%s", WavpackGetErrorMessage (outfile));
        WavpackCloseFile (infile);
        DoCloseHandle (wv_file.file);
        DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
        WavpackCloseFile (outfile);
        return WAVPACK_SOFT_ERROR;
    }

    free (chan_ids);

    if (loc_config.qmode & QMODE_REORDERED_CHANS) {
        uint32_t layout = WavpackGetChannelLayout (infile, NULL);
        unsigned char order [256];

        if (layout & 0xff) {
            WavpackGetChannelLayout (infile, order);
            WavpackSetChannelLayout (outfile, layout, order);
        }
    }
    else
        WavpackSetChannelLayout (outfile, WavpackGetChannelLayout (infile, NULL), NULL);

    // if we are creating a "correction" file, open it now for writing

    if (out2filename) {
        if ((wvc_file.file = fopen (use_tempfiles ? out2filename_temp : out2filename, "w+b")) == NULL) {
            error_line ("can't create correction file!");
            WavpackCloseFile (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (outfile);
            return WAVPACK_SOFT_ERROR;
        }
    }

    // pack the audio portion of the file now; calculate md5 if we're writing it to the file or verify mode is active

    result = repack_audio (outfile, infile, md5_verify);

    // before anything else, make sure the source file was read without errors

    if (result == WAVPACK_NO_ERROR) {
        if (WavpackGetNumErrors (infile)) {
            error_line ("missing data or crc errors detected in %d block(s)!", WavpackGetNumErrors (infile));
            result = WAVPACK_SOFT_ERROR;
        }

        if (WavpackGetNumSamples64 (outfile) != total_samples) {
            error_line ("incorrect number of samples read from source file!");
            result = WAVPACK_SOFT_ERROR;
        }

        if ((input_mode & MODE_LOSSLESS) && !quantize_bits) {
            unsigned char md5_source [16];

            if (WavpackGetMD5Sum (infile, md5_source) && memcmp (md5_source, md5_verify, sizeof (md5_source))) {
                error_line ("MD5 signature in source should match, but does not!");
                result = WAVPACK_SOFT_ERROR;
            }
        }
    }

    // copy the md5 sum if present in source; if there's not one there and the user asked to add it,
    // store the one we just calculated

    if (result == WAVPACK_NO_ERROR) {
        if (WavpackGetMD5Sum (infile, md5_display)) {
            if ((input_mode & MODE_LOSSLESS) && quantize_bits)
                memcpy (md5_display, md5_verify, sizeof (md5_verify));
                
            WavpackStoreMD5Sum (outfile, md5_display);
        }
        else if (loc_config.flags & CONFIG_MD5_CHECKSUM) {
            memcpy (md5_display, md5_verify, sizeof (md5_display));
            WavpackStoreMD5Sum (outfile, md5_verify);
        }
    }

    // this is where we deal with a trailer (i.e., trailing wrapper) if there is one

    if (result == WAVPACK_NO_ERROR && WavpackGetWrapperBytes (infile)) {
        unsigned char *buffer = WavpackGetWrapperData (infile);
        int wrapper_size = WavpackGetWrapperBytes (infile);

        // unless we've been specifically told not to, copy RIFF trailer to output file

        if (!(loc_config.qmode & QMODE_NO_STORE_WRAPPER) && !WavpackAddWrapper (outfile, buffer, wrapper_size)) {
            error_line ("%s", WavpackGetErrorMessage (outfile));
            result = WAVPACK_SOFT_ERROR;
        }

        // if we're supposed to try to import ID3 tags, check for and do that now
        // (but only error on a bad tag, not just a missing one or one with no applicable items)

        if (result == WAVPACK_NO_ERROR && import_id3 && wrapper_size > 10 && !strncmp ((char *) buffer, "ID3", 3)) {
            int32_t bytes_used, id3_res;
            char error [80];

            // first we do a "dry run" pass through the ID3 tag, and only if that passes do we try to write the tag items

            id3_res = ImportID3v2 (NULL, buffer, wrapper_size, error, &bytes_used);

            if (!allow_huge_tags && bytes_used > 1048576) {
                error_line ("imported tag items exceed 1 MB, use --allow-huge-tags to override");
                result = WAVPACK_SOFT_ERROR;
            }
            else if (bytes_used > 1048576 * 16) {
                error_line ("imported tag items exceed 16 MB");
                result = WAVPACK_SOFT_ERROR;
            }
            else {
                if (id3_res > 0)
                    id3_res = ImportID3v2 (outfile, buffer, wrapper_size, error, NULL);

                if (id3_res < 0) {
                    error_line ("ID3v2 import: %s", error);
                    result = WAVPACK_SOFT_ERROR;
                }
                else if (id3_res > 0)
                    imported_tag_items = id3_res;
            }
        }

        WavpackFreeWrapper (infile);
    }

    // we're now done with any WavPack blocks, so flush any remaining data

    if (result == WAVPACK_NO_ERROR && !WavpackFlushSamples (outfile)) {
        error_line ("%s", WavpackGetErrorMessage (outfile));
        result = WAVPACK_HARD_ERROR;
    }

    // if still no errors, check to see if we need to create & write a tag
    // (which is NOT stored in regular WavPack blocks)

    if (result == WAVPACK_NO_ERROR && ((input_mode & MODE_VALID_TAG) || num_tag_items || imported_tag_items)) {
        int num_binary_items = WavpackGetNumBinaryTagItems (infile);
        int num_items = WavpackGetNumTagItems (infile), i;
        int item_len, value_len;
        char *item, *value;
        int res = TRUE;

        for (i = 0; i < num_items && res; ++i) {
            item_len = WavpackGetTagItemIndexed (infile, i, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetTagItemIndexed (infile, i, item, item_len + 1);

            // don't copy the values from the "encoder" or "settings" items because
            // these should be based on the current encoder and user settings

            if (!stricmp (item, "encoder")) {
                value = malloc (80);
                sprintf (value, "WavPack %s", PACKAGE_VERSION);
                value_len = (int) strlen (value);
            }
            else if (!stricmp (item, "settings")) {
                value = malloc (256);
                make_settings_string (value, &loc_config);
                value_len = (int) strlen (value);
            }
            else {
                value_len = WavpackGetTagItem (infile, item, NULL, 0);
                value = malloc (value_len + 1);
                WavpackGetTagItem (infile, item, value, value_len + 1);
            }

            res = WavpackAppendTagItem (outfile, item, value, value_len);
            free (value);
            free (item);
        }

        for (i = 0; i < num_binary_items && res; ++i) {
            item_len = WavpackGetBinaryTagItemIndexed (infile, i, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetBinaryTagItemIndexed (infile, i, item, item_len + 1);
            value_len = WavpackGetBinaryTagItem (infile, item, NULL, 0);
            value = malloc (value_len);
            value_len = WavpackGetBinaryTagItem (infile, item, value, value_len);
            res = WavpackAppendBinaryTagItem (outfile, item, value, value_len);
            free (value);
            free (item);
        }

        for (i = 0; i < num_tag_items && res; ++i)
            if (tag_items [i].vsize) {
                if (tag_items [i].binary) 
                    res = WavpackAppendBinaryTagItem (outfile, tag_items [i].item, tag_items [i].value, tag_items [i].vsize);
                else
                    res = WavpackAppendTagItem (outfile, tag_items [i].item, tag_items [i].value, tag_items [i].vsize);
            }
            else
                WavpackDeleteTagItem (outfile, tag_items [i].item);

        if (!res || !WavpackWriteTag (outfile)) {
            error_line ("%s", WavpackGetErrorMessage (outfile));
            result = WAVPACK_HARD_ERROR;
        }
    }

    WavpackCloseFile (infile);     // we're now done with input file, so close

    // at this point we're completely done with the files, so close 'em whether there
    // were any other errors or not

    if (!DoCloseHandle (wv_file.file)) {
        error_line ("can't close WavPack file!");

        if (result == WAVPACK_NO_ERROR)
            result = WAVPACK_SOFT_ERROR;
    }

    if (out2filename && !DoCloseHandle (wvc_file.file)) {
        error_line ("can't close correction file!");

        if (result == WAVPACK_NO_ERROR)
            result = WAVPACK_SOFT_ERROR;
    }

    // if there have been no errors up to now, and verify mode is enabled, do that now; only pass in the md5 if this
    // was a lossless operation (either explicitly or because a high lossy bitrate resulted in lossless)

    if (result == WAVPACK_NO_ERROR && verify_mode)
        result = verify_audio (use_tempfiles ? outfilename_temp : outfilename, !WavpackLossyBlocks (outfile) ? md5_verify : NULL);

    // if there were any errors, delete the output files, close the context, and return the error

    if (result != WAVPACK_NO_ERROR) {
        DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);

        if (out2filename)
            DoDeleteFile (use_tempfiles ? out2filename_temp : out2filename);

        WavpackCloseFile (outfile);
        return result;
    }

    if (result == WAVPACK_NO_ERROR && copy_time)
        if (!copy_timestamp (infilename, use_tempfiles ? outfilename_temp : outfilename) ||
            (out2filename && !copy_timestamp (infilename, use_tempfiles ? out2filename_temp : out2filename)))
                error_line ("failure copying time stamp!");

    // delete source file(s) if that option is enabled (this is done before temp file rename to make sure
    // we don't delete the file(s) we just created)

    if (result == WAVPACK_NO_ERROR && delete_source) {
        int res;

        if (stricmp (infilename, outfilename)) {
            res = DoDeleteFile (infilename);

            if (!quiet_mode || !res)
                error_line ("%s source file %s", res ?
                    "deleted" : "can't delete", infilename);
        }
    
        if (input_mode & MODE_WVC) {
            char in2filename [PATH_MAX];

            strcpy (in2filename, infilename);
            strcat (in2filename, "c");

            if (!out2filename || stricmp (in2filename, out2filename)) {
                res = DoDeleteFile (in2filename);

                if (!quiet_mode || !res)
                    error_line ("%s source file %s", res ?
                        "deleted" : "can't delete", in2filename);
            }
        }
    }

    // if we were writing to a temp file because the target file already existed,
    // do the rename / overwrite now (and if that fails, return the error)

    if (use_tempfiles) {
#if defined(_WIN32)
        FILE *temp;

        if (remove (outfilename) && (temp = fopen (outfilename, "rb"))) {
            error_line ("can not remove file %s, result saved in %s!", outfilename, outfilename_temp);
            result = WAVPACK_SOFT_ERROR;
            fclose (temp);
        }
        else
#endif
        if (rename (outfilename_temp, outfilename)) {
            error_line ("can not rename temp file %s to %s!", outfilename_temp, outfilename);
            result = WAVPACK_SOFT_ERROR;
        }

        if (out2filename) {
#if defined(_WIN32)
            FILE *temp;

            if (remove (out2filename) && (temp = fopen (out2filename, "rb"))) {
                error_line ("can not remove file %s, result saved in %s!", out2filename, out2filename_temp);
                result = WAVPACK_SOFT_ERROR;
                fclose (temp);
            }
            else
#endif
            if (rename (out2filename_temp, out2filename)) {
                error_line ("can not rename temp file %s to %s!", out2filename_temp, out2filename);
                result = WAVPACK_SOFT_ERROR;
            }
        }

        free (outfilename_temp);
        if (out2filename) free (out2filename_temp);

        if (result != WAVPACK_NO_ERROR) {
            WavpackCloseFile (outfile);
            return result;
        }
    }

    // compute and display the time consumed along with some other details of
    // the packing operation, and then return WAVPACK_NO_ERROR

#if defined(_WIN32)
    _ftime64 (&time2);
    dtime = time2.time + time2.millitm / 1000.0;
    dtime -= time1.time + time1.millitm / 1000.0;
#else
    gettimeofday(&time2,&timez);
    dtime = time2.tv_sec + time2.tv_usec / 1000000.0;
    dtime -= time1.tv_sec + time1.tv_usec / 1000000.0;
#endif

    if ((loc_config.flags & CONFIG_CALC_NOISE) && WavpackGetEncodedNoise (outfile, NULL) > 0.0) {
        int full_scale_bits = WavpackGetBitsPerSample (outfile);
        double full_scale_rms = 0.5, sum, peak;

        while (full_scale_bits--)
            full_scale_rms *= 2.0;

        full_scale_rms = full_scale_rms * (full_scale_rms - 1.0) * 0.5;
        sum = WavpackGetEncodedNoise (outfile, &peak);

        error_line ("ave noise = %.2f dB, peak noise = %.2f dB",
            log10 (sum / WavpackGetNumSamples (outfile) / full_scale_rms) * 10,
            log10 (peak / full_scale_rms) * 10);
    }

    if (!quiet_mode) {
        char *file, *fext, *oper, *cmode, cratio [16] = "";

        if (imported_tag_items)
            error_line ("successfully imported %d items from ID3v2 tag", imported_tag_items);

        if (config->flags & CONFIG_MD5_CHECKSUM) {
            char md5_string [] = "original md5 signature: 00000000000000000000000000000000";
            int i;

            for (i = 0; i < 16; ++i)
                sprintf (md5_string + 24 + (i * 2), "%02x", md5_display [i]);

            error_line (md5_string);
        }

        if (outfilename && *outfilename != '-') {
            file = FN_FIT (outfilename);
            fext = wvc_file.bytes_written ? " (+.wvc)" : "";
            oper = verify_mode ? "created (and verified)" : "created";
        }
        else {
            file = (*infilename == '-') ? "stdin" : FN_FIT (infilename);
            fext = "";
            oper = "packed";
        }

        if (WavpackLossyBlocks (outfile)) {
            cmode = "lossy";

            if (WavpackGetAverageBitrate (outfile, TRUE) != 0.0)
                sprintf (cratio, ", %d kbps", (int) (WavpackGetAverageBitrate (outfile, TRUE) / 1000.0));
        }
        else {
            cmode = "lossless";

            if (WavpackGetRatio (outfile) != 0.0)
                sprintf (cratio, ", %.2f%%", 100.0 - WavpackGetRatio (outfile) * 100.0);
        }

        error_line ("%s %s%s in %.2f secs (%s%s)", oper, file, fext, dtime, cmode, cratio);
    }

    WavpackCloseFile (outfile);
    return WAVPACK_NO_ERROR;
}

// This function handles the actual audio data transcoding. It assumes that the
// input file is positioned at the beginning of the audio data and that the
// WavPack configuration has been set. If the "md5_digest_source" pointer is not
// NULL, then a MD5 sum is calculated on the audio data during the transcoding
// and stored there at the completion. Note that the md5 requires a conversion
// to the native data format (endianness and bytes per sample) that is not
// required overwise.

static int repack_audio (WavpackContext *outfile, WavpackContext *infile, unsigned char *md5_digest_source)
{
    int bps = WavpackGetBytesPerSample (infile), num_channels = WavpackGetNumChannels (infile);
    int qmode = WavpackGetQualifyMode (infile);
    unsigned char *new_channel_order = NULL;
    uint32_t input_samples = INPUT_SAMPLES;
    unsigned char *format_buffer;
    int32_t *sample_buffer;
    double progress = -1.0;
    MD5_CTX md5_context;
    int32_t quantize_bit_mask = 0;
    double fquantize_scale = 1.0, fquantize_iscale = 1.0;

    // modify input_samples if we're doing blocked DSD, or we have a large number of channels

    if (qmode & QMODE_DSD_IN_BLOCKS)
        input_samples = DSD_BLOCKSIZE;
    else
        while (input_samples * sizeof (int32_t) * WavpackGetNumChannels (outfile) > 2048*1024)
            input_samples >>= 1;

    if (md5_digest_source) {
        format_buffer = malloc (input_samples * bps * WavpackGetNumChannels (outfile));
        MD5Init (&md5_context);

        if (qmode & QMODE_REORDERED_CHANS) {
            int layout = WavpackGetChannelLayout (infile, NULL), i;

            if ((layout & 0xff) <= num_channels) {
                new_channel_order = malloc (num_channels);

                for (i = 0; i < num_channels; ++i)
                    new_channel_order [i] = i;

                WavpackGetChannelLayout (infile, new_channel_order);
            }
        }
    }

    WavpackPackInit (outfile);
    sample_buffer = malloc (input_samples * sizeof (int32_t) * WavpackGetNumChannels (outfile));

    if (quantize_bits && quantize_bits < bps*8) {
        quantize_bit_mask = ~((1<<(bps*8-quantize_bits))-1);
        if (MODE_FLOAT == (WavpackGetMode(infile) & MODE_FLOAT)) {
            int float_norm_exp = WavpackGetFloatNormExp (infile);
            fquantize_scale = exp2 (quantize_bits + 126 - float_norm_exp);
            fquantize_iscale = exp2 (float_norm_exp - 126 - quantize_bits);
        }
    }

    while (1) {
        int32_t sample_count = WavpackUnpackSamples (infile, sample_buffer, input_samples);

        if (!sample_count)
            break;

        if (quantize_bit_mask) {
            unsigned int x,l = sample_count * num_channels;
            if (0 == (WavpackGetMode(infile) & MODE_FLOAT)) {
                if (quantize_round) {
                    int32_t offset = (quantize_bit_mask >> 1) ^ quantize_bit_mask;
                    int shift = 32 - bps * 8;

                    for (x = 0; x < l; x ++)
                        if (sample_buffer[x] < 0 || ((sample_buffer[x] + offset) << shift) > 0)
                            sample_buffer[x] += offset;
                }

                for (x = 0; x < l; x ++) sample_buffer[x] &= quantize_bit_mask;
            }
            else {
                for (x = 0; x < l; x ++) {
                    const float f = *(float *)&sample_buffer[x];
                    *(float *)&sample_buffer[x] = (float) (floor(f * fquantize_scale + 0.5) * fquantize_iscale);
                }
            }
        }

        if (!WavpackPackSamples (outfile, sample_buffer, sample_count)) {
            error_line ("%s", WavpackGetErrorMessage (outfile));
            free (sample_buffer);
            return WAVPACK_HARD_ERROR;
        }

        if (md5_digest_source) {
            if (new_channel_order)
                unreorder_channels (sample_buffer, new_channel_order, num_channels, sample_count);

            if (qmode & QMODE_DSD_AUDIO) {
                unsigned char *dptr = format_buffer;
                int32_t *sptr = sample_buffer;

                if (qmode & QMODE_DSD_IN_BLOCKS) {
                    int cc = num_channels;

                    while (cc--) {
                        int si;

                        for (si = 0; si < DSD_BLOCKSIZE; si++, sptr += num_channels)
                            if (si < sample_count)
                                *dptr++ = (qmode & QMODE_DSD_LSB_FIRST) ? bit_reverse_table [*sptr & 0xff] : *sptr;
                            else
                                *dptr++ = 0;

                        sptr -= (DSD_BLOCKSIZE * num_channels) - 1;
                    }

                    sample_count = DSD_BLOCKSIZE;
                }
                else {
                    int scount = sample_count * num_channels;

                    while (scount--)
                        *dptr++ = *sptr++;
                }
            }
            else
                store_samples (format_buffer, sample_buffer, qmode, bps, sample_count * num_channels);

            MD5Update (&md5_context, format_buffer, bps * sample_count * num_channels);
        }

        if (check_break ()) {
#if defined(_WIN32)
            fprintf (stderr, "^C\n");
#else
            fprintf (stderr, "\n");
#endif
            fflush (stderr);
            free (sample_buffer);
            return WAVPACK_SOFT_ERROR;
        }

        if (WavpackGetProgress (outfile) != -1.0 &&
            progress != floor (WavpackGetProgress (outfile) * encode_time_percent + 0.5)) {
                int nobs = progress == -1.0;

                progress = floor (WavpackGetProgress (outfile) * encode_time_percent + 0.5);
                display_progress (progress / 100.0);

                if (!quiet_mode) {
                    fprintf (stderr, "%s%3d%% done...",
                        nobs ? " " : "\b\b\b\b\b\b\b\b\b\b\b\b", (int) progress);
                    fflush (stderr);
                }
        }
    }

    if (new_channel_order)
        free (new_channel_order);

    free (sample_buffer);

    if (!WavpackFlushSamples (outfile)) {
        error_line ("%s", WavpackGetErrorMessage (outfile));
        return WAVPACK_HARD_ERROR;
    }

    if (md5_digest_source) {
        MD5Final (md5_digest_source, &md5_context);
        free (format_buffer);
    }

    return WAVPACK_NO_ERROR;
}

static void reorder_channels (void *data, unsigned char *order, int num_chans,
    int num_samples, int bytes_per_sample)
{
    char reorder_buffer [64], *temp = reorder_buffer;
    char *src = data;

    if (num_chans * bytes_per_sample > 64)
        temp = malloc (num_chans * bytes_per_sample);

    while (num_samples--) {
        char *start = src;
        int chan;

        for (chan = 0; chan < num_chans; ++chan) {
            char *dst = temp + (order [chan] * bytes_per_sample);
            int bc = bytes_per_sample;

            while (bc--)
                *dst++ = *src++;
        }

        memcpy (start, temp, num_chans * bytes_per_sample);
    }

    if (num_chans * bytes_per_sample > 64)
        free (temp);
}

static void unreorder_channels (int32_t *data, unsigned char *order, int num_chans, int num_samples)
{
    int32_t reorder_buffer [16], *temp = reorder_buffer;

    if (num_chans > 16)
        temp = malloc (num_chans * sizeof (*data));

    while (num_samples--) {
        int chan;

        for (chan = 0; chan < num_chans; ++chan)
            temp [chan] = data [order[chan]];

        memcpy (data, temp, num_chans * sizeof (*data));
        data += num_chans;
    }

    if (num_chans > 16)
        free (temp);
}

// Verify the specified WavPack input file. This function uses the library
// routines provided in wputils.c to do all unpacking. If an MD5 sum is provided
// by the caller, then this function will take care of reformatting the data
// (which is returned in native-endian longs) to the standard little-endian
// for a proper MD5 verification. Otherwise a lossy verification is assumed,
// and we only verify the exact number of samples and whether the decoding
// library detected CRC errors in any WavPack blocks.

#define VERIFY_BLOCKSIZE DSD_BLOCKSIZE

static int verify_audio (char *infilename, unsigned char *md5_digest_source)
{
    int num_channels, bps, qmode, result = WAVPACK_NO_ERROR;
    unsigned char *new_channel_order = NULL;
    int64_t total_unpacked_samples = 0;
    unsigned char md5_digest_result [16];
    double progress = -1.0;
    int32_t *temp_buffer;
    MD5_CTX md5_context;
    WavpackContext *wpc;
    char error [80];

    // use library to open WavPack file

#ifdef _WIN32
    wpc = WavpackOpenFileInput (infilename, error, OPEN_WVC | OPEN_FILE_UTF8 | OPEN_DSD_NATIVE | OPEN_ALT_TYPES, 0);
#else
    wpc = WavpackOpenFileInput (infilename, error, OPEN_WVC | OPEN_DSD_NATIVE | OPEN_ALT_TYPES, 0);
#endif

    if (!wpc) {
        error_line (error);
        return WAVPACK_SOFT_ERROR;
    }

    if (md5_digest_source)
        MD5Init (&md5_context);

    qmode = WavpackGetQualifyMode (wpc);
    num_channels = WavpackGetNumChannels (wpc);
    bps = WavpackGetBytesPerSample (wpc);
    temp_buffer = malloc (VERIFY_BLOCKSIZE * num_channels * 4);

    if (qmode & QMODE_REORDERED_CHANS) {
        int layout = WavpackGetChannelLayout (wpc, NULL), i;

        if ((layout & 0xff) <= num_channels) {
            new_channel_order = malloc (num_channels);

            for (i = 0; i < num_channels; ++i)
                new_channel_order [i] = i;

            WavpackGetChannelLayout (wpc, new_channel_order);
        }
    }

    while (result == WAVPACK_NO_ERROR) {
        int32_t samples_unpacked;

        samples_unpacked = WavpackUnpackSamples (wpc, temp_buffer, VERIFY_BLOCKSIZE);
        total_unpacked_samples += samples_unpacked;

        if (samples_unpacked) {
            if (md5_digest_source) {
                if (new_channel_order)
                    unreorder_channels (temp_buffer, new_channel_order, num_channels, samples_unpacked);

                if (qmode & QMODE_DSD_AUDIO) {
                    unsigned char *dsd_buffer = malloc (DSD_BLOCKSIZE * num_channels);
                    unsigned char *dptr = dsd_buffer;
                    int32_t *sptr = temp_buffer;

                    if (qmode & QMODE_DSD_IN_BLOCKS) {
                        int cc = num_channels;

                        while (cc--) {
                            int si;

                            for (si = 0; si < DSD_BLOCKSIZE; si++, sptr += num_channels)
                                if (si < samples_unpacked)
                                    *dptr++ = (qmode & QMODE_DSD_LSB_FIRST) ? bit_reverse_table [*sptr & 0xff] : *sptr;
                                else
                                    *dptr++ = 0;

                            sptr -= (DSD_BLOCKSIZE * num_channels) - 1;
                        }

                        samples_unpacked = DSD_BLOCKSIZE;   // count the entire block for MD5 (even if partial/last)
                    }
                    else {
                        int scount = samples_unpacked * num_channels;

                        while (scount--)
                            *dptr++ = *sptr++;
                    }

                    MD5Update (&md5_context, dsd_buffer, samples_unpacked * num_channels);
                    free (dsd_buffer);
                }
                else {
                    store_samples (temp_buffer, temp_buffer, qmode, bps, samples_unpacked * num_channels);
                    MD5Update (&md5_context, (unsigned char *) temp_buffer, bps * samples_unpacked * num_channels);
                }
            }
        }
        else
            break;

        if (check_break ()) {
#if defined(_WIN32)
            fprintf (stderr, "^C\n");
#else
            fprintf (stderr, "\n");
#endif
            fflush (stderr);
            result = WAVPACK_SOFT_ERROR;
            break;
        }

        if (WavpackGetProgress (wpc) != -1.0 &&
            progress != floor (WavpackGetProgress (wpc) * (100.0 - encode_time_percent) + encode_time_percent + 0.5)) {

                progress = floor (WavpackGetProgress (wpc) * (100.0 - encode_time_percent) + encode_time_percent + 0.5);
                display_progress (progress / 100.0);

                if (!quiet_mode) {
                    fprintf (stderr, "%s%3d%% done...",
                        "\b\b\b\b\b\b\b\b\b\b\b\b", (int) progress);
                    fflush (stderr);
                }
        }
    }

    free (temp_buffer);

    if (new_channel_order)
        free (new_channel_order);

    // If we have been provided an MD5 sum, then the assumption is that we are doing lossless compression (either explicitly
    // with lossless mode or having a high enough bitrate that the result is lossless) and we can use the MD5 sum as a pretty
    // definitive verification.

    if (result == WAVPACK_NO_ERROR && md5_digest_source) {
        MD5Final (md5_digest_result, &md5_context);

        if (memcmp (md5_digest_result, md5_digest_source, 16)) {
            char md5_string1 [] = "00000000000000000000000000000000";
            char md5_string2 [] = "00000000000000000000000000000000";
            int i;

            for (i = 0; i < 16; ++i) {
                sprintf (md5_string1 + (i * 2), "%02x", md5_digest_source [i]);
                sprintf (md5_string2 + (i * 2), "%02x", md5_digest_result [i]);
            }

            error_line ("original md5: %s", md5_string1);
            error_line ("verified md5: %s", md5_string2);
            error_line ("MD5 signatures should match, but do not!");
            result = WAVPACK_SOFT_ERROR;
        }
    }

    // If we have not been provided an MD5 sum, then the assumption is that we are doing lossy compression and cannot rely
    // (obviously) on that for verification. For these cases we make sure that the number of samples generated was exactly
    // correct and that the WavPack decoding library did not detect an error. There is a simple CRC on every WavPack block
    // that should catch any random corruption, although it's possible that this might miss some decoder bug that occurs
    // late in the decoding process (e.g., after the CRC).

    if (result == WAVPACK_NO_ERROR) {
        if (WavpackGetNumSamples64 (wpc) != -1) {
            if (total_unpacked_samples < WavpackGetNumSamples64 (wpc)) {
                error_line ("file is missing %llu samples!",
                    WavpackGetNumSamples64 (wpc) - total_unpacked_samples);
                result = WAVPACK_SOFT_ERROR;
            }
            else if (total_unpacked_samples > WavpackGetNumSamples64 (wpc)) {
                error_line ("file has %llu extra samples!",
                    total_unpacked_samples - WavpackGetNumSamples64 (wpc));
                result = WAVPACK_SOFT_ERROR;
            }
        }

        if (WavpackGetNumErrors (wpc)) {
            error_line ("missing data or crc errors detected in %d block(s)!", WavpackGetNumErrors (wpc));
            result = WAVPACK_SOFT_ERROR;
        }
    }

    WavpackCloseFile (wpc);
    return result;
}

// Create a string from the specified configuration that can be used for the "settings"
// tag. Note that the module globals allow_huge_tags and quantize_bits are also accessed.
// Room for 256 characters should be plenty.

static void make_settings_string (char *settings, WavpackConfig *config)
{
    strcpy (settings, "-");

    // basic settings

    if (config->flags & CONFIG_FAST_FLAG)
        strcat (settings, "f");
    else if (config->flags & CONFIG_VERY_HIGH_FLAG)
        strcat (settings, "hh");
    else if (config->flags & CONFIG_HIGH_FLAG)
        strcat (settings, "h");

    if (config->flags & CONFIG_HYBRID_FLAG) {
        sprintf (settings + strlen (settings), "b%g", config->bitrate);

        if (config->flags & CONFIG_OPTIMIZE_WVC)
            strcat (settings, "cc");
        else if (config->flags & CONFIG_CREATE_WVC)
            strcat (settings, "c");
    }

    if (config->flags & CONFIG_EXTRA_MODE)
        sprintf (settings + strlen (settings), "x%d", config->xmode ? config->xmode : 1);

    // override settings

    if (config->flags & CONFIG_JOINT_OVERRIDE) {
        if (config->flags & CONFIG_JOINT_STEREO)
            strcat (settings, "j1");
        else
            strcat (settings, "j0");
    }

    if (config->flags & CONFIG_SHAPE_OVERRIDE)
        sprintf (settings + strlen (settings), "s%g", config->shaping_weight);

    // long options

    if (quantize_bits)
        sprintf (settings + strlen (settings), " --pre-quantize%s=%d",
            quantize_round ? "-round" : "", quantize_bits);

    if (config->block_samples)
        sprintf (settings + strlen (settings), " --blocksize=%d", config->block_samples);

    if (config->flags & CONFIG_DYNAMIC_SHAPING)
        strcat (settings, " --use-dns");

    if (config->flags & CONFIG_CROSS_DECORR)
        strcat (settings, " --cross-decorr");

    if (config->flags & CONFIG_MERGE_BLOCKS)
        strcat (settings, " --merge-blocks");

    if (config->flags & CONFIG_PAIR_UNDEF_CHANS)
        strcat (settings, " --pair-unassigned-chans");

    if (allow_huge_tags)
        strcat (settings, " --allow-huge-tags");
}

// Code to load samples. Destination is an array of int32_t data (which is what WavPack uses
// internally), but the source can have from 1 to 4 bytes per sample. Also, the source data
// is assumed to be little-endian and signed, except for byte data which is unsigned (these
// are WAV file defaults). The endian and signedness can be overridden with the qmode flags
// to support other formats.

static void load_little_endian_unsigned_samples (int32_t *dst, void *src, int bps, int count);
static void load_little_endian_signed_samples (int32_t *dst, void *src, int bps, int count);
static void load_big_endian_unsigned_samples (int32_t *dst, void *src, int bps, int count);
static void load_big_endian_signed_samples (int32_t *dst, void *src, int bps, int count);

static void load_samples (int32_t *dst, void *src, int qmode, int bps, int count)
{
    if (qmode & QMODE_BIG_ENDIAN) {
        if ((qmode & QMODE_UNSIGNED_WORDS) || (bps == 1 && !(qmode & QMODE_SIGNED_BYTES)))
            load_big_endian_unsigned_samples (dst, src, bps, count);
        else
            load_big_endian_signed_samples (dst, src, bps, count);
    }
    else if ((qmode & QMODE_UNSIGNED_WORDS) || (bps == 1 && !(qmode & QMODE_SIGNED_BYTES)))
        load_little_endian_unsigned_samples (dst, src, bps, count);
    else
        load_little_endian_signed_samples (dst, src, bps, count);
}

static void load_little_endian_unsigned_samples (int32_t *dst, void *src, int bps, int count)
{
    unsigned char *sptr = src;

    switch (bps) {

        case 1:
            while (count--)
                *dst++ = *sptr++ - 0x80;

            break;

        case 2:
            while (count--) {
                *dst++ = (sptr [0] | ((int32_t) sptr [1] << 8)) - 0x8000;
                sptr += 2;
            }

            break;

        case 3:
            while (count--) {
                *dst++ = (sptr [0] | ((int32_t) sptr [1] << 8) | ((int32_t) sptr [2] << 16)) - 0x800000;
                sptr += 3;
            }

            break;

        case 4:
            while (count--) {
                *dst++ = (sptr [0] | ((int32_t) sptr [1] << 8) | ((int32_t) sptr [2] << 16) | ((int32_t) sptr [3] << 24)) - 0x80000000;
                sptr += 4;
            }

            break;
    }
}

static void load_little_endian_signed_samples (int32_t *dst, void *src, int bps, int count)
{
    unsigned char *sptr = src;

    switch (bps) {

        case 1:
            while (count--)
                *dst++ = (signed char) *sptr++;

            break;

        case 2:
            while (count--) {
                *dst++ = sptr [0] | ((int32_t)(signed char) sptr [1] << 8);
                sptr += 2;
            }

            break;

        case 3:
            while (count--) {
                *dst++ = sptr [0] | ((int32_t) sptr [1] << 8) | ((int32_t)(signed char) sptr [2] << 16);
                sptr += 3;
            }

            break;

        case 4:
            while (count--) {
                *dst++ = sptr [0] | ((int32_t) sptr [1] << 8) | ((int32_t) sptr [2] << 16) | ((int32_t)(signed char) sptr [3] << 24);
                sptr += 4;
            }

            break;
    }
}

static void load_big_endian_unsigned_samples (int32_t *dst, void *src, int bps, int count)
{
    unsigned char *sptr = src;

    switch (bps) {

        case 1:
            while (count--)
                *dst++ = *sptr++ - 0x80;

            break;

        case 2:
            while (count--) {
                *dst++ = (sptr [1] | ((int32_t) sptr [0] << 8)) - 0x8000;
                sptr += 2;
            }

            break;

        case 3:
            while (count--) {
                *dst++ = (sptr [2] | ((int32_t) sptr [1] << 8) | ((int32_t) sptr [0] << 16)) - 0x800000;
                sptr += 3;
            }

            break;

        case 4:
            while (count--) {
                *dst++ = (sptr [3] | ((int32_t) sptr [2] << 8) | ((int32_t) sptr [1] << 16) | ((int32_t) sptr [0] << 24)) - 0x80000000;
                sptr += 4;
            }

            break;
    }
}

static void load_big_endian_signed_samples (int32_t *dst, void *src, int bps, int count)
{
    unsigned char *sptr = src;

    switch (bps) {

        case 1:
            while (count--)
                *dst++ = (signed char) *sptr++;

            break;

        case 2:
            while (count--) {
                *dst++ = sptr [1] | ((int32_t)(signed char) sptr [0] << 8);
                sptr += 2;
            }

            break;

        case 3:
            while (count--) {
                *dst++ = sptr [2] | ((int32_t) sptr [1] << 8) | ((int32_t)(signed char) sptr [0] << 16);
                sptr += 3;
            }

            break;

        case 4:
            while (count--) {
                *dst++ = sptr [3] | ((int32_t) sptr [2] << 8) | ((int32_t) sptr [1] << 16) | ((int32_t)(signed char) sptr [0] << 24);
                sptr += 4;
            }

            break;
    }
}

// Code to store samples. Source is an array of int32_t data (which is what WavPack uses
// internally), but the destination can have from 1 to 4 bytes per sample. Also, the destination
// data is assumed to be little-endian and signed, except for byte data which is unsigned (these
// are WAV file defaults). The endian and signedness can be overridden with the qmode flags
// to support other formats.

static void *store_little_endian_unsigned_samples (void *dst, int32_t *src, int bps, int count);
static void *store_little_endian_signed_samples (void *dst, int32_t *src, int bps, int count);
static void *store_big_endian_unsigned_samples (void *dst, int32_t *src, int bps, int count);
static void *store_big_endian_signed_samples (void *dst, int32_t *src, int bps, int count);

static void *store_samples (void *dst, int32_t *src, int qmode, int bps, int count)
{
    if (qmode & QMODE_BIG_ENDIAN) {
        if ((qmode & QMODE_UNSIGNED_WORDS) || (bps == 1 && !(qmode & QMODE_SIGNED_BYTES)))
            return store_big_endian_unsigned_samples (dst, src, bps, count);
        else
            return store_big_endian_signed_samples (dst, src, bps, count);
    }
    else if ((qmode & QMODE_UNSIGNED_WORDS) || (bps == 1 && !(qmode & QMODE_SIGNED_BYTES)))
        return store_little_endian_unsigned_samples (dst, src, bps, count);
    else
        return store_little_endian_signed_samples (dst, src, bps, count);
}

static void *store_little_endian_unsigned_samples (void *dst, int32_t *src, int bps, int count)
{
    unsigned char *dptr = dst;
    int32_t temp;

    switch (bps) {

        case 1:
            while (count--)
                *dptr++ = *src++ + 0x80;

            break;

        case 2:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++ + 0x8000);
                *dptr++ = (unsigned char) (temp >> 8);
            }

            break;

        case 3:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++ + 0x800000);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) (temp >> 16);
            }

            break;

        case 4:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++ + 0x80000000);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) (temp >> 16);
                *dptr++ = (unsigned char) (temp >> 24);
            }

            break;
    }

    return dptr;
}

static void *store_little_endian_signed_samples (void *dst, int32_t *src, int bps, int count)
{
    unsigned char *dptr = dst;
    int32_t temp;

    switch (bps) {

        case 1:
            while (count--)
                *dptr++ = *src++;

            break;

        case 2:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++);
                *dptr++ = (unsigned char) (temp >> 8);
            }

            break;

        case 3:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) (temp >> 16);
            }

            break;

        case 4:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) (temp >> 16);
                *dptr++ = (unsigned char) (temp >> 24);
            }

            break;
    }

    return dptr;
}

static void *store_big_endian_unsigned_samples (void *dst, int32_t *src, int bps, int count)
{
    unsigned char *dptr = dst;
    int32_t temp;

    switch (bps) {

        case 1:
            while (count--)
                *dptr++ = *src++ + 0x80;

            break;

        case 2:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++ + 0x8000) >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;

        case 3:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++ + 0x800000) >> 16);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;

        case 4:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++ + 0x80000000) >> 24);
                *dptr++ = (unsigned char) (temp >> 16);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;
    }

    return dptr;
}

static void *store_big_endian_signed_samples (void *dst, int32_t *src, int bps, int count)
{
    unsigned char *dptr = dst;
    int32_t temp;

    switch (bps) {

        case 1:
            while (count--)
                *dptr++ = *src++;

            break;

        case 2:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++) >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;

        case 3:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++) >> 16);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;

        case 4:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++) >> 24);
                *dptr++ = (unsigned char) (temp >> 16);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;
    }

    return dptr;
}

#if defined(_WIN32)

// Convert the Unicode wide-format string into a UTF-8 string using no more
// than the specified buffer length. The wide-format string must be NULL
// terminated and the resulting string will be NULL terminated. The actual
// number of characters converted (not counting terminator) is returned, which
// may be less than the number of characters in the wide string if the buffer
// length is exceeded.

static int WideCharToUTF8 (const wchar_t *Wide, unsigned char *pUTF8, int len)
{
    const wchar_t *pWide = Wide;
    int outndx = 0;

    while (*pWide) {
        if (*pWide < 0x80 && outndx + 1 < len)
            pUTF8 [outndx++] = (unsigned char) *pWide++;
        else if (*pWide < 0x800 && outndx + 2 < len) {
            pUTF8 [outndx++] = (unsigned char) (0xc0 | ((*pWide >> 6) & 0x1f));
            pUTF8 [outndx++] = (unsigned char) (0x80 | (*pWide++ & 0x3f));
        }
        else if (outndx + 3 < len) {
            pUTF8 [outndx++] = (unsigned char) (0xe0 | ((*pWide >> 12) & 0xf));
            pUTF8 [outndx++] = (unsigned char) (0x80 | ((*pWide >> 6) & 0x3f));
            pUTF8 [outndx++] = (unsigned char) (0x80 | (*pWide++ & 0x3f));
        }
        else
            break;
    }

    pUTF8 [outndx] = 0;
    return (int)(pWide - Wide);
}

// Convert a text string into its Unicode UTF-8 format equivalent. The
// conversion is done in-place so the maximum length of the string buffer must
// be specified because the string may become longer or shorter. If the
// resulting string will not fit in the specified buffer size then it is
// truncated.

static void TextToUTF8 (void *string, int len)
{
    unsigned char *inp = string;

    // simple case: test for UTF8 BOM and if so, simply delete the BOM

    if (len > 3 && inp [0] == 0xEF && inp [1] == 0xBB && inp [2] == 0xBF) {
        memmove (inp, inp + 3, len - 3);
        inp [len - 3] = 0;
    }
    else if (* (wchar_t *) string == 0xFEFF) {
        wchar_t *temp = _wcsdup (string);

        WideCharToUTF8 (temp + 1, (unsigned char *) string, len);
        free (temp);
    }
    else {
        int max_chars = (int) strlen (string);
        wchar_t *temp = (wchar_t *) malloc ((max_chars + 1) * 2);

        MultiByteToWideChar (CP_ACP, 0, string, -1, temp, max_chars + 1);
        WideCharToUTF8 (temp, (unsigned char *) string, len);
        free (temp);
    }
}

#else

static void TextToUTF8 (void *string, int len)
{
    char *temp = malloc (len);
    char *outp = temp;
    char *inp = string;
    size_t insize = 0;
    size_t outsize = len - 1;
    int err = 0;
    char *old_locale;
    iconv_t converter;

    // simple case: test for UTF8 BOM and if so, simply delete the BOM and return

    if (len > 3 && (unsigned char) inp [0] == 0xEF && (unsigned char) inp [1] == 0xBB &&
        (unsigned char) inp [2] == 0xBF) {
            memmove (inp, inp + 3, len - 3);
            inp [len - 3] = 0;
            return;
    }

    memset(temp, 0, len);
    old_locale = setlocale (LC_CTYPE, "");

    if ((unsigned char) inp [0] == 0xFF && (unsigned char) inp [1] == 0xFE) {
        uint16_t *utf16p = (uint16_t *) (inp += 2);

        while (*utf16p++)
            insize += 2;

        converter = iconv_open ("UTF-8", "UTF-16LE");
    }
    else {
        insize = strlen (string);
        converter = iconv_open ("UTF-8", "");
    }

    if (converter != (iconv_t) -1) {
        err = iconv (converter, &inp, &insize, &outp, &outsize);
        iconv_close (converter);
    }
    else
        err = -1;

    setlocale (LC_CTYPE, old_locale);

    if (err == -1) {
        free(temp);
        return;
    }

    memmove (string, temp, len);
    free (temp);
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function displays the progress status on the title bar of the DOS   //
// window that WavPack is running in. The "file_progress" argument is for   //
// the current file only and ranges from 0 - 1; this function takes into    //
// account the total number of files to generate a batch progress number.   //
//////////////////////////////////////////////////////////////////////////////

static void display_progress (double file_progress)
{
    char title [40];

    if (set_console_title) {
        file_progress = (file_index + file_progress) / num_files;
        sprintf (title, "%d%% (WavPack)", (int) ((file_progress * 100.0) + 0.5));
        DoSetConsoleTitle (title);
    }
}
