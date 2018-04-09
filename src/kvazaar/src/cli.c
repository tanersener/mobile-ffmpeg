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

/*
* \file
*
*/

#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>

static const char short_options[] = "i:o:d:w:h:n:q:p:r:";
static const struct option long_options[] = {
  { "input",              required_argument, NULL, 'i' },
  { "output",             required_argument, NULL, 'o' },
  { "debug",              required_argument, NULL, 'd' },
  { "width",              required_argument, NULL, 'w' },
  { "height",             required_argument, NULL, 'h' }, // deprecated
  { "frames",             required_argument, NULL, 'n' }, // deprecated
  { "qp",                 required_argument, NULL, 'q' },
  { "period",             required_argument, NULL, 'p' },
  { "ref",                required_argument, NULL, 'r' },
  { "vps-period",         required_argument, NULL, 0 },
  { "input-res",          required_argument, NULL, 0 },
  { "input-fps",          required_argument, NULL, 0 },
  { "deblock",            required_argument, NULL, 0 },
  { "no-deblock",               no_argument, NULL, 0 },
  { "sao",                optional_argument, NULL, 0 },
  { "no-sao",                   no_argument, NULL, 0 },
  { "rdoq",                     no_argument, NULL, 0 },
  { "no-rdoq",                  no_argument, NULL, 0 },
  { "signhide",                 no_argument, NULL, 0 },
  { "no-signhide",              no_argument, NULL, 0 },
  { "smp",                      no_argument, NULL, 0 },
  { "no-smp",                   no_argument, NULL, 0 },
  { "amp",                      no_argument, NULL, 0 },
  { "no-amp",                   no_argument, NULL, 0 },
  { "rd",                 required_argument, NULL, 0 },
  { "full-intra-search",        no_argument, NULL, 0 },
  { "no-full-intra-search",     no_argument, NULL, 0 },
  { "transform-skip",           no_argument, NULL, 0 },
  { "no-transform-skip",        no_argument, NULL, 0 },
  { "tr-depth-intra",     required_argument, NULL, 0 },
  { "me",                 required_argument, NULL, 0 },
  { "subme",              required_argument, NULL, 0 },
  { "source-scan-type",   required_argument, NULL, 0 },
  { "sar",                required_argument, NULL, 0 },
  { "overscan",           required_argument, NULL, 0 },
  { "videoformat",        required_argument, NULL, 0 },
  { "range",              required_argument, NULL, 0 },
  { "colorprim",          required_argument, NULL, 0 },
  { "transfer",           required_argument, NULL, 0 },
  { "colormatrix",        required_argument, NULL, 0 },
  { "chromaloc",          required_argument, NULL, 0 },
  { "aud",                      no_argument, NULL, 0 },
  { "no-aud",                   no_argument, NULL, 0 },
  { "cqmfile",            required_argument, NULL, 0 },
  { "seek",               required_argument, NULL, 0 },
  { "tiles",              required_argument, NULL, 0 },
  { "tiles-width-split",  required_argument, NULL, 0 },
  { "tiles-height-split", required_argument, NULL, 0 },
  { "wpp",                      no_argument, NULL, 0 },
  { "no-wpp",                   no_argument, NULL, 0 },
  { "owf",                required_argument, NULL, 0 },
  { "slices",             required_argument, NULL, 0 },
  { "threads",            required_argument, NULL, 0 },
  { "cpuid",              required_argument, NULL, 0 },
  { "pu-depth-inter",     required_argument, NULL, 0 },
  { "pu-depth-intra",     required_argument, NULL, 0 },
  { "info",                     no_argument, NULL, 0 },
  { "no-info",                  no_argument, NULL, 0 },
  { "gop",                required_argument, NULL, 0 },
  { "bipred",                   no_argument, NULL, 0 },
  { "no-bipred",                no_argument, NULL, 0 },
  { "bitrate",            required_argument, NULL, 0 },
  { "preset",             required_argument, NULL, 0 },
  { "mv-rdo",                   no_argument, NULL, 0 },
  { "no-mv-rdo",                no_argument, NULL, 0 },
  { "psnr",                     no_argument, NULL, 0 },
  { "no-psnr",                  no_argument, NULL, 0 },
  { "version",                  no_argument, NULL, 0 },
  { "help",                     no_argument, NULL, 0 },
  { "loop-input",               no_argument, NULL, 0 },
  { "mv-constraint",      required_argument, NULL, 0 },
  { "hash",               required_argument, NULL, 0 },
  {"cu-split-termination",required_argument, NULL, 0 },
  { "crypto",             required_argument, NULL, 0 },
  { "key",                required_argument, NULL, 0 },
  { "me-early-termination",required_argument, NULL, 0 },
  { "lossless",                 no_argument, NULL, 0 },
  { "no-lossless",              no_argument, NULL, 0 },
  { "tmvp",                     no_argument, NULL, 0 },
  { "no-tmvp",                  no_argument, NULL, 0 },
  { "rdoq-skip",                no_argument, NULL, 0 },
  { "no-rdoq-skip",             no_argument, NULL, 0 },
  { "input-bitdepth",     required_argument, NULL, 0 },
  { "input-format",       required_argument, NULL, 0 },
  { "implicit-rdpcm",           no_argument, NULL, 0 },
  { "no-implicit-rdpcm",        no_argument, NULL, 0 },
  { "roi",                required_argument, NULL, 0 },
  { "erp-aqp",                  no_argument, NULL, 0 },
  { "no-erp-aqp",               no_argument, NULL, 0 },
  {0, 0, 0, 0}
};

/**
* \brief Try to detect resolution from file name automatically
*
* \param file_name    file name to get dimensions from
* \param out_width    detected width
* \param out_height   detected height
* \return      1 if the resolution is set, 0 on fail
*/
static int select_input_res_auto(const char *file_name, int32_t *out_width, int32_t *out_height)
{
  if (!file_name) return 0;

  // Find the last delimiter char ( / or \ ). Hope the other kind is not used in the name.
  // If delim is not found, set pointer to the beginning.
  unsigned char* sub_str = (unsigned char*)MAX(strrchr(file_name, '/'), strrchr(file_name, '\\'));
  if (!sub_str) sub_str = (unsigned char*)file_name;

  int success = 0;
  // Try if the substring starts with "<int>x<int>" without either of them being 0
  do {
    success = (sscanf((char*)sub_str, "%dx%d%*s", out_width, out_height) == 2);
    success &= (*out_width > 0 && *out_height > 0);
    // Move to the next char until a digit is found or the string ends
    do{
      ++sub_str;
    } while (*sub_str != 0 && !isdigit(*sub_str));
    // Continue until "<int>x<int>" is found or the string ends
  } while (*sub_str != 0 && !success);

  return success;
}

/**
 * \brief Parse command line arguments.
 * \param argc  Number of arguments
 * \param argv  Argument list
 * \return      Pointer to the parsed options, or NULL on failure.
 */
cmdline_opts_t* cmdline_opts_parse(const kvz_api *const api, int argc, char *argv[])
{
  int ok = 1;
  cmdline_opts_t *opts = calloc(1, sizeof(cmdline_opts_t));
  if (!opts) {
    ok = 0;
    goto done;
  }

  opts->config = api->config_alloc();
  if (!opts->config || !api->config_init(opts->config)) {
    ok = 0;
    goto done;
  }

  // Parse command line options
  for (optind = 0;;) {
    int long_options_index = -1;

    int c = getopt_long(argc, argv, short_options, long_options, &long_options_index);
    if (c == -1)
      break;

    if (long_options_index < 0) {
      int i;
      for (i = 0; long_options[i].name; i++)
        if (long_options[i].val == c) {
            long_options_index = i;
            break;
        }
      if (long_options_index < 0) {
        // getopt_long already printed an error message
        ok = 0;
        goto done;
      }
    }

    const char* name = long_options[long_options_index].name;
    if (!strcmp(name, "input")) {
      if (opts->input) {
        fprintf(stderr, "Input error: More than one input file given.\n");
        ok = 0;
        goto done;
      }
      opts->input = strdup(optarg);
    } else if (!strcmp(name, "output")) {
      if (opts->output) {
        fprintf(stderr, "Input error: More than one output file given.\n");
        ok = 0;
        goto done;
      }
      opts->output = strdup(optarg);
    } else if (!strcmp(name, "debug")) {
      if (opts->debug) {
        fprintf(stderr, "Input error: More than one debug output file given.\n");
        ok = 0;
        goto done;
      }
      opts->debug = strdup(optarg);
    } else if (!strcmp(name, "seek")) {
      opts->seek = atoi(optarg);
    } else if (!strcmp(name, "frames")) {
      opts->frames = atoi(optarg);
    } else if (!strcmp(name, "version")) {
      opts->version = true;
      goto done;
    } else if (!strcmp(name, "help")) {
      opts->help = true;
      goto done;
    } else if (!strcmp(name, "loop-input")) {
      opts->loop_input = true;
    } else if (!api->config_parse(opts->config, name, optarg)) {
      fprintf(stderr, "invalid argument: %s=%s\n", name, optarg);
      ok = 0;
      goto done;
    }
  }

  // Check for extra arguments.
  if (argc - optind > 0) {
    fprintf(stderr, "Input error: Extra argument found: \"%s\"\n", argv[optind]);
    ok = 0;
    goto done;
  }

  // Check that the required files were defined
  if (opts->input == NULL || opts->output == NULL) {
    ok = 0;
    goto done;
  }

  if (opts->config->vps_period < 0) {
    // Disabling parameter sets is only possible when using Kvazaar as
    // a library.
    fprintf(stderr, "Input error: vps_period must be non-negative\n");
    ok = 0;
    goto done;
  }

  // Set resolution automatically if necessary
  if (opts->config->width == 0 && opts->config->height == 0) {
    ok = select_input_res_auto(opts->input, &opts->config->width, &opts->config->height);
    goto done;
  }

done:
  if (!ok) {
    cmdline_opts_free(api, opts);
    opts = NULL;
  }

  return opts;
}


/**
 * \brief Deallocate a cmdline_opts_t structure.
 */
void cmdline_opts_free(const kvz_api *const api, cmdline_opts_t *opts)
{
  if (opts) {
    FREE_POINTER(opts->input);
    FREE_POINTER(opts->output);
    FREE_POINTER(opts->debug);
    api->config_destroy(opts->config);
    opts->config = NULL;
  }
  FREE_POINTER(opts);
}


void print_usage(void)
{
  fprintf(stdout,
    "Kvazaar usage: -i and --input-res to set input, -o to set output\n"
    "               --help for more information\n");
}


void print_version(void)
{
  fprintf(stdout,
    "Kvazaar " VERSION_STRING "\n"
    "Kvazaar license: LGPL version 2\n");
}


void print_help(void)
{
  fprintf(stdout,
    "Usage:\n"
    "kvazaar -i <input> --input-res <width>x<height> -o <output>\n"
    "\n"
    /* Word wrap to this width to stay under 80 characters (including ") ************/
    "Required:\n"
    "  -i, --input                : Input file\n"
    "      --input-res <res>      : Input resolution [auto]\n"
    "                               auto: detect from file name\n"
    "                               <int>x<int>: width times height\n"
    "  -o, --output               : Output file\n"
    "\n"
    /* Word wrap to this width to stay under 80 characters (including ") ************/
    "Presets:\n"
    "      --preset=<preset>      : Set options to a preset [medium]\n"
    "                                   - ultrafast, superfast, veryfast, faster,\n"
    "                                     fast, medium, slow, slower, veryslow\n"
    "                                     placebo\n"
    "\n"
    /* Word wrap to this width to stay under 80 characters (including ") ************/
    "Input:\n"
    "  -n, --frames <integer>     : Number of frames to code [all]\n"
    "      --seek <integer>       : First frame to code [0]\n"
    "      --input-fps <num>/<denom> : Framerate of the input video [25.0]\n"
    "      --source-scan-type <string> : Set source scan type [progressive].\n"
    "                                   - progressive: progressive scan\n"
    "                                   - tff: top field first\n"
    "                                   - bff: bottom field first\n"
    "      --input-format         : P420 or P400\n"
    "      --input-bitdepth       : 8-16\n"
    "      --loop-input           : Re-read input file forever\n"
    "\n"
    /* Word wrap to this width to stay under 80 characters (including ") ************/
    "Options:\n"
    "      --help                 : Print this help message and exit\n"
    "      --version              : Print version information and exit\n"
    "      --aud                  : Use access unit delimiters\n"
    "      --debug <string>       : Output encoders reconstruction.\n"
    "      --cpuid <integer>      : Disable runtime cpu optimizations with value 0.\n"
    "      --hash                 : Decoded picture hash [checksum]\n"
    "                                   - none: 0 bytes\n"
    "                                   - checksum: 18 bytes\n"
    "                                   - md5: 56 bytes\n"
    "      --no-psnr              : Don't calculate PSNR for frames\n"
    "      --no-info              : Don't add encoder info SEI.\n"
    "\n"
    /* Word wrap to this width to stay under 80 characters (including ") ************/
    "Video structure:\n"
    "  -q, --qp <integer>         : Quantization Parameter [32]\n"
    "  -p, --period <integer>     : Period of intra pictures [0]\n"
    "                               - 0: only first picture is intra\n"
    "                               - 1: all pictures are intra\n"
    "                               - 2-N: every Nth picture is intra\n"
    "      --vps-period <integer> : Specify how often the video parameter set is\n"
    "                               re-sent. [0]\n"
    "                                   - 0: only send VPS with the first frame\n"
    "                                   - N: send VPS with every Nth intra frame\n"
    "  -r, --ref <integer>        : Reference frames, range 1..15 [3]\n"
    "      --gop <string>         : Definition of GOP structure [0]\n"
    "                                   - 0: disabled\n"
    "                                   - 8: B-frame pyramid of length 8\n"
    "                                   - lp-<string>: lp-gop definition\n"
    "                                         (e.g. lp-g8d4t2, see README)\n"
    "      --cqmfile <string>     : Custom Quantization Matrices from a file\n"
    "      --bitrate <integer>    : Target bitrate. [0]\n"
    "                                   - 0: disable rate-control\n"
    "                                   - N: target N bits per second\n"
    "      --lossless             : Use lossless coding\n"
    "      --mv-constraint        : Constrain movement vectors\n"
    "                                   - none: no constraint\n"
    "                                   - frametile: constrain within the tile\n"
    "                                   - frametilemargin: constrain even more\n"
    "      --roi <string>         : Use a delta QP map for region of interest\n"
    "                                   Read an array of delta QP values from\n"
    "                                   a file, where the first two values are the\n"
    "                                   width and height, followed by width*height\n"
    "                                   delta QP values in raster order.\n"
    "                                   The delta QP map can be any size or aspect\n"
    "                                   ratio, and will be mapped to LCU's.\n"
    "      --(no-)erp-aqp         : Use adaptive QP for 360 video with\n"
    "                               equirectangular projection\n"
    "\n"
    /* Word wrap to this width to stay under 80 characters (including ") ************/
    "Compression tools:\n"
    "      --deblock [<beta:tc>]  : Deblocking\n"
    "                                     - beta: between -6 and 6\n"
    "                                     - tc: between -6 and 6\n"
    "      --(no-)sao             : Sample Adaptive Offset\n"
    "      --(no-)rdoq            : Rate-Distortion Optimized Quantization\n"
    "      --(no-)signhide        : Sign Hiding\n"
    "      --(no-)smp             : Symmetric Motion Partition\n"
    "      --(no-)amp             : Asymmetric Motion Partition\n"
    "      --rd <integer>         : Intra mode search complexity\n"
    "                                   - 0: skip intra if inter is good enough\n"
    "                                   - 1: rough intra mode search with SATD\n"
    "                                   - 2: refine intra mode search with SSE\n"
    "      --(no-)mv-rdo          : Rate-Distortion Optimized motion vector costs\n"
    "      --(no-)full-intra-search\n"
    "                             : Try all intra modes during rough search.\n"
    "      --(no-)transform-skip  : Transform skip\n"
    "      --me <string>          : Integer motion estimation\n"
    "                                   - hexbs: Hexagon Based Search\n"
    "                                   - tz:    Test Zone Search\n"
    "                                   - full:  Full Search\n"
    "                                   - full8, full16, full32, full64\n"
    "      --subme <integer>      : Set fractional pixel motion estimation level\n"
    "                                   - 0: only integer motion estimation\n"
    "                                   - 1: + 1/2-pixel horizontal and vertical\n"
    "                                   - 2: + 1/2-pixel diagonal\n"
    "                                   - 3: + 1/4-pixel horizontal and vertical\n"
    "                                   - 4: + 1/4-pixel diagonal\n"
    "      --pu-depth-inter <int>-<int>\n"
    "                             : Range for sizes for inter predictions\n"
    "                                   - 0, 1, 2, 3: from 64x64 to 8x8\n"
    "      --pu-depth-intra <int>-<int> : Range for sizes for intra predictions\n"
    "                                   - 0, 1, 2, 3, 4: from 64x64 to 4x4\n"
    "      --(no-)bipred          : Bi-prediction\n"
    "      --(no-)cu-split-termination\n"
    "                             : CU split search termination condition\n"
    "                                   - off: Never terminate cu-split search\n"
    "                                   - zero: Terminate with zero residual\n"
    "      --(no-)me-early-termination : ME early termination condition\n"
    "                                   - off: Don't terminate early\n"
    "                                   - on: Terminate early\n"
    "                                   - sensitive: Terminate even earlier\n"
    "      --(no-)implicit-rdpcm  : Implicit residual DPCM\n"
    "                               Currently only supported with lossless coding.\n"
    "      --(no-)tmvp            : Temporal Motion Vector Prediction\n"
    "      --(no-)rdoq-skip       : Skips RDOQ for 4x4 blocks\n"
    "\n"
    /* Word wrap to this width to stay under 80 characters (including ") ************/
    "Parallel processing:\n"
    "      --threads <integer>    : Number of threads to use [auto]\n"
    "                                   - 0: process everything with main thread\n"
    "                                   - N: use N threads for encoding\n"
    "                                   - auto: select based on number of cores\n"
    "      --owf <integer>        : Frame parallelism [auto]\n"
    "                                   - N: Process N-1 frames at a time\n"
    "                                   - auto: Select automatically\n"
    "      --(no-)wpp             : Wavefront parallel processing [enabled]\n"
    "                               Enabling tiles automatically disables WPP.\n"
    "                               To enable WPP with tiles, re-enable it after\n"
    "                               enabling tiles.\n"
    "      --tiles <int>x<int>    : Split picture into width x height uniform tiles.\n"
    "      --tiles-width-split <string>|u<int> :\n"
    "                               Specifies a comma separated list of pixel\n"
    "                               positions of tiles columns separation coordinates.\n"
    "                               Can also be u followed by and a single int n,\n"
    "                               in which case it produces columns of uniform width.\n"
    "      --tiles-height-split <string>|u<int> :\n"
    "                               Specifies a comma separated list of pixel\n"
    "                               positions of tiles rows separation coordinates.\n"
    "                               Can also be u followed by and a single int n,\n"
    "                               in which case it produces rows of uniform height.\n"
    "      --slices <string>      : Control how slices are used\n"
    "                                   - tiles: put tiles in independent slices\n"
    "                                   - wpp: put rows in dependent slices\n"
    "                                   - tiles+wpp: do both\n"
    "\n"
    /* Word wrap to this width to stay under 80 characters (including ") ************/
    "Video Usability Information:\n"
    "      --sar <width:height>   : Specify Sample Aspect Ratio\n"
    "      --overscan <string>    : Specify crop overscan setting [undef]\n"
    "                                   - undef, show, crop\n"
    "      --videoformat <string> : Specify video format [undef]\n"
    "                                   - component, pal, ntsc, secam, mac, undef\n"
    "      --range <string>       : Specify color range [tv]\n"
    "                                   - tv, pc\n"
    "      --colorprim <string>   : Specify color primaries [undef]\n"
    "                                   - undef, bt709, bt470m, bt470bg,\n"
    "                                     smpte170m, smpte240m, film, bt2020\n"
    "      --transfer <string>    : Specify transfer characteristics [undef]\n"
    "                                   - undef, bt709, bt470m, bt470bg,\n"
    "                                     smpte170m, smpte240m, linear, log100,\n"
    "                                     log316, iec61966-2-4, bt1361e,\n"
    "                                     iec61966-2-1, bt2020-10, bt2020-12\n"
    "      --colormatrix <string> : Specify color matrix setting [undef]\n"
    "                                   - undef, bt709, fcc, bt470bg, smpte170m,\n"
    "                                     smpte240m, GBR, YCgCo, bt2020nc, bt2020c\n"
    "      --chromaloc <integer>  : Specify chroma sample location (0 to 5) [0]\n"
    "\n"
    /* Word wrap to this width to stay under 80 characters (including ") ************/
    "Deprecated parameters: (might be removed at some point)\n"
    "  -w, --width                 : Use --input-res\n"
    "  -h, --height                : Use --input-res\n");
}


void print_frame_info(const kvz_frame_info *const info,
                      const double frame_psnr[3],
                      const uint32_t bytes,
                      const bool print_psnr)
{
  fprintf(stderr, "POC %4d QP %2d (%c-frame) %10d bits",
          info->poc,
          info->qp,
          "BPI"[info->slice_type % 3],
          bytes << 3);
  if (print_psnr) {
    fprintf(stderr, " PSNR Y %2.4f U %2.4f V %2.4f",
            frame_psnr[0], frame_psnr[1], frame_psnr[2]);
  }

  if (info->slice_type != KVZ_SLICE_I) {
    // Print reference picture lists
    fprintf(stderr, " [L0 ");
    for (int j = 0; j < info->ref_list_len[0]; j++) {
      fprintf(stderr, "%d ", info->ref_list[0][j]);
    }
    fprintf(stderr, "] [L1 ");
    for (int j = 0; j < info->ref_list_len[1]; j++) {
      fprintf(stderr, "%d ", info->ref_list[1][j]);
    }
    fprintf(stderr, "]");
  }

  fprintf(stderr, "\n");
}
