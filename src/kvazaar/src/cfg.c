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

#include "cfg.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


kvz_config *kvz_config_alloc(void)
{
  return calloc(1, sizeof(kvz_config));
}

int kvz_config_init(kvz_config *cfg)
{
  cfg->width           = 0;
  cfg->height          = 0;
  cfg->framerate       = 25; // deprecated and will be removed.
  cfg->framerate_num   = 0;
  cfg->framerate_denom = 1;
  cfg->qp              = 22;
  cfg->intra_period    = 64;
  cfg->vps_period      = 0;
  cfg->deblock_enable  = 1;
  cfg->deblock_beta    = 0;
  cfg->deblock_tc      = 0;
  cfg->sao_type        = 3;
  cfg->rdoq_enable     = 1;
  cfg->rdoq_skip       = 1;
  cfg->signhide_enable = true;
  cfg->smp_enable      = false;
  cfg->amp_enable      = false;
  cfg->rdo             = 1;
  cfg->mv_rdo          = 0;
  cfg->full_intra_search = 0;
  cfg->trskip_enable   = 0;
  cfg->tr_depth_intra  = 0;
  cfg->ime_algorithm   = 0; /* hexbs */
  cfg->fme_level       = 4;
  cfg->source_scan_type = 0; /* progressive */
  cfg->vui.sar_width   = 0;
  cfg->vui.sar_height  = 0;
  cfg->vui.overscan    = 0; /* undef */
  cfg->vui.videoformat = 5; /* undef */
  cfg->vui.fullrange   = 0; /* limited range */
  cfg->vui.colorprim   = 2; /* undef */
  cfg->vui.transfer    = 2; /* undef */
  cfg->vui.colormatrix = 2; /* undef */
  cfg->vui.chroma_loc  = 0; /* left center */
  cfg->aud_enable      = 0;
  cfg->cqmfile         = NULL;
  cfg->ref_frames      = 1;
  cfg->gop_len         = 4;
  cfg->gop_lowdelay    = true;
  cfg->bipred          = 0;
  cfg->target_bitrate  = 0;
  cfg->hash            = KVZ_HASH_CHECKSUM;
  cfg->lossless        = false;
  cfg->tmvp_enable     = true;
  cfg->implicit_rdpcm  = false;

  cfg->cu_split_termination = KVZ_CU_SPLIT_TERMINATION_ZERO;

  cfg->tiles_width_count  = 1;
  cfg->tiles_height_count = 1;
  cfg->tiles_width_split  = NULL;
  cfg->tiles_height_split = NULL;
  
  cfg->wpp = 1;
  cfg->owf = -1;
  cfg->slice_count = 1;
  cfg->slice_addresses_in_ts = MALLOC(int32_t, 1);
  cfg->slice_addresses_in_ts[0] = 0;
  
  cfg->threads = -1;
  cfg->cpuid = 1;

  // Defaults for what sizes of PUs are tried.
  cfg->pu_depth_inter.min = 2; // 0-3
  cfg->pu_depth_inter.max = 3; // 0-3
  cfg->pu_depth_intra.min = 2; // 0-4
  cfg->pu_depth_intra.max = 3; // 0-4

  cfg->add_encoder_info = true;
  cfg->calc_psnr = true;

  cfg->mv_constraint = KVZ_MV_CONSTRAIN_NONE;
  cfg->crypto_features = KVZ_CRYPTO_OFF;

  cfg->me_early_termination = 1;

  cfg->input_format = KVZ_FORMAT_P420;
  cfg->input_bitdepth = 8;

  cfg->gop_lp_definition.d = 3;
  cfg->gop_lp_definition.t = 1;

  cfg->roi.width = 0;
  cfg->roi.height = 0;
  cfg->roi.dqps = NULL;

  cfg->erp_aqp = false;

  cfg->slices = KVZ_SLICES_NONE;

  cfg->optional_key = NULL;

  return 1;
}

int kvz_config_destroy(kvz_config *cfg)
{
  if (cfg) {
    FREE_POINTER(cfg->cqmfile);
    FREE_POINTER(cfg->tiles_width_split);
    FREE_POINTER(cfg->tiles_height_split);
    FREE_POINTER(cfg->slice_addresses_in_ts);
    FREE_POINTER(cfg->roi.dqps);
    FREE_POINTER(cfg->optional_key);
  }
  free(cfg);

  return 1;
}

static int atobool(const char *str)
{
  if (!strcmp(str, "1")    ||
      !strcmp(str, "true") ||
      !strcmp(str, "yes"))
    return 1;
  if (!strcmp(str, "0")     ||
      !strcmp(str, "false") ||
      !strcmp(str, "no"))
    return 0;
  return 0;
}

static int parse_enum_n(const char *arg, unsigned num_chars, const char * const *names, int8_t *dst)
{
  int8_t i;
  for (i = 0; names[i]; i++) {
    if (!strncmp(arg, names[i], num_chars)) {
      *dst = i;
      return 1;
    }
  }

  return 0;
}

static int parse_enum(const char *arg, const char * const *names, int8_t *dst)
{
  return parse_enum_n(arg, 255, names, dst);
}

static int parse_tiles_specification(const char* const arg, int32_t * const ntiles, int32_t** const array) {
  const char* current_arg = NULL;
  int32_t current_value;
  int32_t values[MAX_TILES_PER_DIM];
  
  int i;
  
  //Free pointer in any case
  if (*array) {
    FREE_POINTER(*array);
  }
  
  //If the arg starts with u, we want an uniform split
  if (arg[0]=='u') {
    *ntiles = atoi(arg + 1);
    if (MAX_TILES_PER_DIM <= *ntiles || 1 >= *ntiles) {
      fprintf(stderr, "Invalid number of tiles (1 <= %d <= %d = MAX_TILES_PER_DIM)!\n", *ntiles, MAX_TILES_PER_DIM);
      return 0;
    }
    //Done with parsing
    return 1;
  }
  
  //We have a comma-separated list of int for the split...
  current_arg = arg;
  *ntiles = 1;
  do {
    int ret = sscanf(current_arg, "%d", &current_value);
    if (ret != 1) {
      fprintf(stderr, "Could not parse integer \"%s\"!\n", current_arg);
      return 0;
    }
    current_arg = strchr(current_arg, ',');
    //Skip the , if we found one
    if (current_arg) ++current_arg;
    values[*ntiles - 1] = current_value;
    ++(*ntiles);
    if (MAX_TILES_PER_DIM <= *ntiles) break;
  } while (current_arg);
  
  if (MAX_TILES_PER_DIM <= *ntiles || 1 >= *ntiles) {
    fprintf(stderr, "Invalid number of tiles (1 <= %d <= %d = MAX_TILES_PER_DIM)!\n", *ntiles, MAX_TILES_PER_DIM);
    return 0;
  }
  
  *array = MALLOC(int32_t, *ntiles - 1);
  if (!*array) {
    fprintf(stderr, "Could not allocate array for tiles\n");
    return 0;
  }
  
  //TODO: memcpy?
  for (i = 0; i < *ntiles - 1; ++i) {
    (*array)[i] = values[i];
  }
  
  return 1;
}

static int parse_uint8(const char *numstr,uint8_t* number,int min, int max)                               
{
  char *tail;
  int d = strtol(numstr, &tail, 10);
  if (*tail || d < min || d > max){
    fprintf(stderr, "Expected number between %d and %d\n", min, max);
    if(number)
      *number = 0;
    return 0;
  } else{
    if (number)
      *number = (uint8_t) d;
    return 1;
  }
}

static int parse_array(const char *array, uint8_t *coeff_key, int size,
                            int min, int max)
{
  char *key = strdup(array);
  const char delim[] = ",;:";
  char *token;
  int i = 0;

  token = strtok(key, delim);
  while(token!=NULL&&i<size){
    if (!parse_uint8(token, &coeff_key[i], min, max))
    {
      free(key);
      return 0;
    }
    i++;
    token = strtok(NULL, delim);
  }
  if(i>=size && (token != NULL)){
    fprintf(stderr, "parsing failed : too many members.\n");
    free(key);
    return 0;
  }
  else if (i<size){
    fprintf(stderr, "parsing failed : too few members.\n");
    free(key);
    return 0;
  }
  free(key);
  return 1;
}

static int parse_slice_specification(const char* const arg, int32_t * const nslices, int32_t** const array) {
  const char* current_arg = NULL;
  int32_t current_value;
  int32_t values[MAX_SLICES];
  
  int i;
  
  //Free pointer in any case
  if (*array) {
    FREE_POINTER(*array);
  }
  
  //If the arg starts with u, we want an uniform split
  if (arg[0]=='u') {
    *nslices = atoi(arg+1);
    if (MAX_SLICES <= *nslices || 0 >= *nslices) {
      fprintf(stderr, "Invalid number of tiles (0 < %d <= %d = MAX_SLICES)!\n", *nslices + 1, MAX_SLICES);
      return 0;
    }
    //Done with parsing
    return 1;
  }
  
  //We have a comma-separated list of int for the split...
  current_arg = arg;
  //We always have a slice starting at 0
  values[0] = 0;
  *nslices = 1;
  do {
    int ret = sscanf(current_arg, "%d", &current_value);
    if (ret != 1) {
      fprintf(stderr, "Could not parse integer \"%s\"!\n", current_arg);
      return 0;
    }
    current_arg = strchr(current_arg, ',');
    //Skip the , if we found one
    if (current_arg) ++current_arg;
    values[*nslices] = current_value;
    ++(*nslices);
    if (MAX_SLICES <= *nslices) break;
  } while (current_arg);
  
  if (MAX_SLICES <= *nslices || 0 >= *nslices) {
    fprintf(stderr, "Invalid number of slices (0 < %d <= %d = MAX_SLICES)!\n", *nslices, MAX_SLICES);
    return 0;
  }
  
  *array = MALLOC(int32_t, *nslices);
  if (!*array) {
    fprintf(stderr, "Could not allocate array for slices\n");
    return 0;
  }
  
  //TODO: memcpy?
  for (i = 0; i < *nslices; ++i) {
    (*array)[i] = values[i];
  }
  
  return 1;
}

int kvz_config_parse(kvz_config *cfg, const char *name, const char *value)
{
  static const char * const me_names[]          = { "hexbs", "tz", "full", "full8", "full16", "full32", "full64", NULL };
  static const char * const source_scan_type_names[] = { "progressive", "tff", "bff", NULL };

  static const char * const overscan_names[]    = { "undef", "show", "crop", NULL };
  static const char * const videoformat_names[] = { "component", "pal", "ntsc", "secam", "mac", "undef", NULL };
  static const char * const range_names[]       = { "tv", "pc", NULL };
  static const char * const colorprim_names[]   = { "", "bt709", "undef", "", "bt470m", "bt470bg", "smpte170m",
                                                    "smpte240m", "film", "bt2020", NULL };
  static const char * const transfer_names[]    = { "", "bt709", "undef", "", "bt470m", "bt470bg", "smpte170m",
                                                    "smpte240m", "linear", "log100", "log316", "iec61966-2-4",
                                                    "bt1361e", "iec61966-2-1", "bt2020-10", "bt2020-12", NULL };
  static const char * const colormatrix_names[] = { "GBR", "bt709", "undef", "", "fcc", "bt470bg", "smpte170m",
                                                    "smpte240m", "YCgCo", "bt2020nc", "bt2020c", NULL };
  static const char * const mv_constraint_names[] = { "none", "frame", "tile", "frametile", "frametilemargin", NULL };
  static const char * const hash_names[] = { "none", "checksum", "md5", NULL };

  static const char * const cu_split_termination_names[] = { "zero", "off", NULL };
  static const char * const crypto_toggle_names[] = { "off", "on", NULL };
  static const char * const crypto_feature_names[] = { "mvs", "mv_signs", "trans_coeffs", "trans_coeff_signs", "intra_pred_modes", NULL };

  static const char * const me_early_termination_names[] = { "off", "on", "sensitive", NULL };

  static const char * const sao_names[] = { "off", "edge", "band", "full", NULL };

  static const char * const preset_values[11][20*2] = {
      { 
        "ultrafast", 
        "pu-depth-intra", "2-3",
        "pu-depth-inter", "2-3",
        "rd", "0",
        "me", "hexbs",
        "ref", "1",
        "deblock", "0:0",
        "signhide", "0",
        "subme", "0",
        "sao", "off",
        "rdoq", "0",
        "rdoq-skip", "1",
        "transform-skip", "0", 
        "full-intra-search", "0",
        "mv-rdo", "0",
        "smp", "0",
        "amp", "0",
        "cu-split-termination", "zero",
        "me-early-termination", "sensitive",
        "gop", "lp-g4d3t1",
        NULL 
      },
      { 
        "superfast",
        "pu-depth-intra", "2-3",
        "pu-depth-inter", "2-3",
        "rd", "0",
        "me", "hexbs",
        "ref", "1",
        "deblock", "0:0",
        "signhide", "0",
        "subme", "0",
        "sao", "full",
        "rdoq", "0",
        "rdoq-skip", "1",
        "transform-skip", "0",
        "full-intra-search", "0",
        "mv-rdo", "0",
        "smp", "0",
        "amp", "0",
        "cu-split-termination", "zero",
        "me-early-termination", "sensitive",
        "gop", "lp-g4d3t1",
        NULL
      },
      {
        "veryfast",
        "pu-depth-intra", "2-3",
        "pu-depth-inter", "2-3",
        "rd", "0",
        "me", "hexbs",
        "ref", "1",
        "deblock", "0:0",
        "signhide", "0",
        "subme", "2",
        "sao", "full",
        "rdoq", "0",
        "rdoq-skip", "1",
        "transform-skip", "0",
        "full-intra-search", "0",
        "mv-rdo", "0",
        "smp", "0",
        "amp", "0",
        "cu-split-termination", "zero",
        "me-early-termination", "sensitive",
        "gop", "lp-g4d3t1",
        NULL
      },
      {
        "faster",
        "pu-depth-intra", "2-3",
        "pu-depth-inter", "1-3",
        "rd", "1",
        "me", "hexbs",
        "ref", "1",
        "deblock", "0:0",
        "signhide", "0",
        "subme", "2",
        "sao", "full",
        "rdoq", "0",
        "rdoq-skip", "1",
        "transform-skip", "0",
        "full-intra-search", "0",
        "mv-rdo", "0",
        "smp", "0",
        "amp", "0",
        "cu-split-termination", "zero",
        "me-early-termination", "sensitive",
        "gop", "lp-g4d3t1",
        NULL
      },
      {
        "fast",
        "pu-depth-intra", "2-3",
        "pu-depth-inter", "1-3",
        "rd", "1",
        "me", "hexbs",
        "ref", "1",
        "deblock", "0:0",
        "signhide", "0",
        "subme", "4",
        "sao", "full",
        "rdoq", "0",
        "rdoq-skip", "1",
        "transform-skip", "0",
        "full-intra-search", "0",
        "mv-rdo", "0",
        "smp", "0",
        "amp", "0",
        "cu-split-termination", "zero",
        "me-early-termination", "on",
        "gop", "lp-g4d3t1",
        NULL
      },
      {
        "medium",
        "pu-depth-intra", "1-3",
        "pu-depth-inter", "1-3",
        "rd", "1",
        "me", "hexbs",
        "ref", "1",
        "deblock", "0:0",
        "signhide", "0",
        "subme", "4",
        "sao", "full",
        "rdoq", "1",
        "rdoq-skip", "1",
        "transform-skip", "0",
        "full-intra-search", "0",
        "mv-rdo", "0",
        "smp", "0",
        "amp", "0",
        "cu-split-termination", "zero",
        "me-early-termination", "on",
        "gop", "lp-g4d3t1",
        NULL
      },
      {
        "slow",
        "pu-depth-intra", "1-3",
        "pu-depth-inter", "1-3",
        "rd", "1",
        "me", "hexbs",
        "ref", "2",
        "deblock", "0:0",
        "signhide", "1",
        "subme", "4",
        "sao", "full",
        "rdoq", "1",
        "rdoq-skip", "1",
        "transform-skip", "0",
        "full-intra-search", "0",
        "mv-rdo", "0",
        "smp", "0",
        "amp", "0",
        "cu-split-termination", "zero",
        "me-early-termination", "on",
        "gop", "lp-g4d2t1",
        NULL
      },
      {
        "slower",
        "pu-depth-intra", "1-3",
        "pu-depth-inter", "0-3",
        "rd", "1",
        "me", "hexbs",
        "ref", "2",
        "deblock", "0:0",
        "signhide", "1",
        "subme", "4",
        "sao", "full",
        "rdoq", "1",
        "rdoq-skip", "1",
        "transform-skip", "0",
        "full-intra-search", "0",
        "mv-rdo", "0",
        "smp", "0",
        "amp", "0",
        "cu-split-termination", "zero",
        "me-early-termination", "on",
        "gop", "lp-g4d2t1",
        NULL
      },
      {
        "veryslow",
        "pu-depth-intra", "1-4",
        "pu-depth-inter", "0-3",
        "rd", "1",
        "me", "hexbs",
        "ref", "3",
        "deblock", "0:0",
        "signhide", "1",
        "subme", "4",
        "sao", "full",
        "rdoq", "1",
        "rdoq-skip", "1",
        "transform-skip", "0",
        "full-intra-search", "0",
        "mv-rdo", "0",
        "smp", "0",
        "amp", "0",
        "cu-split-termination", "zero",
        "me-early-termination", "on",
        "gop", "lp-g4d2t1",
        NULL
      },
      {
        "placebo",
        "pu-depth-intra", "1-4",
        "pu-depth-inter", "0-3",
        "rd", "1",
        "me", "tz",
        "ref", "4",
        "deblock", "0:0",
        "signhide", "1",
        "subme", "4",
        "sao", "full",
        "rdoq", "1",
        "rdoq-skip", "0",
        "transform-skip", "1",
        "full-intra-search", "0",
        "mv-rdo", "1",
        "smp", "1",
        "amp", "1",
        "cu-split-termination", "off",
        "me-early-termination", "off",
        "gop", "lp-g4d2t1",
        NULL
      },
      { NULL }
  };

  if (!name)
    return 0;

  if (!value)
    value = "true";

  // Treat "--no-param" as --param 0
  if ((!strncmp(name, "no-", 3))) {
    name += 3;
    value = atobool(value) ? "false" : "true";
  }

#define OPT(STR) (!strcmp(name, STR))
  if OPT("width")
    cfg->width = atoi(value);
  else if OPT("height")
    cfg->height = atoi(value);
  else if OPT("input-res")
    if (!strcmp(value, "auto")) {
      return 1;
    } else {
      return (sscanf(value, "%dx%d", &cfg->width, &cfg->height) == 2);
    }
  else if OPT("input-fps") {
    int32_t fps_num, fps_denom;
    if (sscanf(value, "%d/%d", &fps_num, &fps_denom) == 2) {
      cfg->framerate_num = fps_num;
      cfg->framerate_denom = fps_denom;
    } else {
      // Accept decimal notation, making sure not to round 0 to 1.
      cfg->framerate_num = (int)(atof(value) * 1000 + 0.49);
      cfg->framerate_denom = 1000;
    }
  }
  else if OPT("qp")
    cfg->qp = atoi(value);
  else if OPT("period")
    cfg->intra_period = atoi(value);
  else if OPT("vps-period")
    cfg->vps_period = atoi(value);
  else if OPT("ref")
    cfg->ref_frames = atoi(value);
  else if OPT("deblock") {
    int beta, tc;
    if (2 == sscanf(value, "%d:%d", &beta, &tc)) {
      cfg->deblock_enable = 1;
      cfg->deblock_beta   = beta;
      cfg->deblock_tc     = tc;
    } else {
      cfg->deblock_enable = atobool(value);
    }
  }
  else if OPT("sao") {
    int8_t sao_type = 0;
    if (!parse_enum(value, sao_names, &sao_type)) sao_type = atobool(value) ? 3 : 0;
    cfg->sao_type = sao_type;
  }
  else if OPT("rdoq")
    cfg->rdoq_enable = atobool(value);
  else if OPT("signhide")
    cfg->signhide_enable = (bool)atobool(value);
  else if OPT("smp")
    cfg->smp_enable = (bool)atobool(value);
  else if OPT("amp")
    cfg->amp_enable = (bool)atobool(value);
  else if OPT("rd")
    cfg->rdo = atoi(value);
  else if OPT("full-intra-search")
    cfg->full_intra_search = atobool(value);
  else if OPT("transform-skip")
    cfg->trskip_enable = atobool(value);
  else if OPT("tr-depth-intra")
    cfg->tr_depth_intra = atoi(value);
  else if OPT("me") {
    int8_t ime_algorithm = 0;
    if (!parse_enum(value, me_names, &ime_algorithm)) return 0;
    cfg->ime_algorithm = ime_algorithm;
  }
  else if OPT("subme")
    cfg->fme_level = atoi(value);
  else if OPT("source-scan-type")
    return parse_enum(value, source_scan_type_names, &cfg->source_scan_type);
  else if OPT("mv-constraint")
  {
    int8_t constraint = KVZ_MV_CONSTRAIN_NONE;
    int result = parse_enum(value, mv_constraint_names, &constraint);
    cfg->mv_constraint = constraint;
    return result;
  }
  else if OPT("sar")
    return sscanf(value, "%d:%d", &cfg->vui.sar_width, &cfg->vui.sar_height) == 2;
  else if OPT("overscan")
    return parse_enum(value, overscan_names, &cfg->vui.overscan);
  else if OPT("videoformat")
    return parse_enum(value, videoformat_names, &cfg->vui.videoformat);
  else if OPT("range")
    return parse_enum(value, range_names, &cfg->vui.fullrange);
  else if OPT("colorprim")
    return parse_enum(value, colorprim_names, &cfg->vui.colorprim);
  else if OPT("transfer")
    return parse_enum(value, transfer_names, &cfg->vui.transfer);
  else if OPT("colormatrix")
    return parse_enum(value, colormatrix_names, &cfg->vui.colormatrix);
  else if OPT("chromaloc")
    cfg->vui.chroma_loc = atoi(value);
  else if OPT("aud")
    cfg->aud_enable = atobool(value);
  else if OPT("cqmfile") {
    char* cqmfile = strdup(value);
    if (!cqmfile) {
      fprintf(stderr, "Failed to allocate memory for CQM file name.\n");
      return 0;
    }
    FREE_POINTER(cfg->cqmfile);
    cfg->cqmfile = cqmfile;
  }
  else if OPT("tiles-width-split") {
    int retval = parse_tiles_specification(value, &cfg->tiles_width_count, &cfg->tiles_width_split);
    
    if (cfg->tiles_width_count > 1 && cfg->tmvp_enable) {
      cfg->tmvp_enable = false;
      fprintf(stderr, "Disabling TMVP because tiles are used.\n");
    }

    if (cfg->wpp) {
      cfg->wpp = false;
      fprintf(stderr, "Disabling WPP because tiles were enabled.\n");
    }

    return retval;
  }
  else if OPT("tiles-height-split") {
    int retval = parse_tiles_specification(value, &cfg->tiles_height_count, &cfg->tiles_height_split);
    
    if (cfg->tiles_height_count > 1 && cfg->tmvp_enable) {
      cfg->tmvp_enable = false;
      fprintf(stderr, "Disabling TMVP because tiles are used.\n");
    }

    if (cfg->wpp) {
      cfg->wpp = false;
      fprintf(stderr, "Disabling WPP because tiles were enabled.\n");
    }

    return retval;
  }
  else if OPT("tiles")
  {
    // A simpler interface for setting tiles, accepting only uniform split.
    unsigned width;
    unsigned height;
    if (2 != sscanf(value, "%ux%u", &width, &height)) {
      fprintf(stderr, "Wrong format for tiles. Expected \"%%ux%%u\", but got \"%s\"\n", value);
      return 0;
    }

    if (MAX_TILES_PER_DIM <= width || 1 > width) {
      fprintf(stderr, "Invalid number of tiles (0 < %d <= %d = MAX_TILES_PER_DIM)!\n", width, MAX_TILES_PER_DIM);
      return 0;
    }
    if (MAX_TILES_PER_DIM <= height || 1 > height) {
      fprintf(stderr, "Invalid number of tiles (0 < %d <= %d = MAX_TILES_PER_DIM)!\n", height, MAX_TILES_PER_DIM);
      return 0;
    }

    // Free split arrays incase they have already been set by another parameter.
    FREE_POINTER(cfg->tiles_width_split);
    FREE_POINTER(cfg->tiles_height_split);
    cfg->tiles_width_count = width;
    cfg->tiles_height_count = height;

    if (cfg->tmvp_enable) {
      cfg->tmvp_enable = false;
      fprintf(stderr, "Disabling TMVP because tiles are used.\n");
    }

    if (cfg->wpp) {
      cfg->wpp = false;
      fprintf(stderr, "Disabling WPP because tiles were enabled.\n");
    }

    return 1;
  }
  else if OPT("wpp")
    cfg->wpp = atobool(value);
  else if OPT("owf") {
    cfg->owf = atoi(value);
    if (cfg->owf == 0 && !strcmp(value, "auto")) {
      // -1 means automatic selection
      cfg->owf = -1;
    }
  } else if OPT("slices") {
    if (!strcmp(value, "tiles")) {
      cfg->slices = KVZ_SLICES_TILES;
      return 1;
    } else if (!strcmp(value, "wpp")) {
      cfg->slices = KVZ_SLICES_WPP;
      return 1;
    } else if (!strcmp(value, "tiles+wpp")) {
      cfg->slices = KVZ_SLICES_TILES | KVZ_SLICES_WPP;
      return 1;
    } else {
      return parse_slice_specification(value, &cfg->slice_count, &cfg->slice_addresses_in_ts);
    }

  } else if OPT("threads") {
    cfg->threads = atoi(value);
    if (cfg->threads == 0 && !strcmp(value, "auto")) {
      // -1 means automatic selection
      cfg->threads = -1;
    }
  }
  else if OPT("cpuid")
    cfg->cpuid = atoi(value);
  else if OPT("pu-depth-inter")
    return sscanf(value, "%d-%d", &cfg->pu_depth_inter.min, &cfg->pu_depth_inter.max) == 2;
  else if OPT("pu-depth-intra")
    return sscanf(value, "%d-%d", &cfg->pu_depth_intra.min, &cfg->pu_depth_intra.max) == 2;
  else if OPT("info")
    cfg->add_encoder_info = atobool(value);
  else if OPT("gop") {
    if (!strncmp(value, "lp-", 3)) {  // Handle GOPs starting with "lp-".
      struct {
        unsigned g;  // length
        unsigned d;  // depth
        unsigned t;  // temporal
      } gop = { 0, 0, 0 };

      // Parse --gop=lp-g#d#t#
      if (sscanf(value, "lp-g%ud%ut%u", &gop.g, &gop.d, &gop.t) != 3) {
        fprintf(stderr, "Error in GOP syntax. Example: lp-g8d4t2\n");
        return 0;
      }

      if (gop.g < 1 || gop.g > 32) {
        fprintf(stderr, "gop.g must be between 1 and 32.\n");
        return 0;
      }
      if (gop.d < 1 || gop.d > 8) {
        fprintf(stderr, "gop.d must be between 1 and 8.\n");
        return 0;
      }
      if (gop.t < 1 || gop.t > 15) {
        fprintf(stderr, "gop.t must be between 1 and 15.\n");
        return 0;
      }

      cfg->gop_lowdelay = true;
      cfg->gop_len = gop.g;
      cfg->gop_lp_definition.d = gop.d;
      cfg->gop_lp_definition.t = gop.t;
    } else if (atoi(value) == 8) {
      cfg->gop_lowdelay = 0;
      // GOP
      cfg->gop_len = 8;
      cfg->gop[0].poc_offset = 8; cfg->gop[0].qp_offset = 1; cfg->gop[0].layer = 1; cfg->gop[0].qp_factor = 0.442;  cfg->gop[0].is_ref = 1;
      cfg->gop[0].ref_pos_count = 0;
      cfg->gop[0].ref_neg_count = 3; cfg->gop[0].ref_neg[0] = 8; cfg->gop[0].ref_neg[1] = 12; cfg->gop[0].ref_neg[2] = 16;

      cfg->gop[1].poc_offset = 4; cfg->gop[1].qp_offset = 2; cfg->gop[1].layer = 2; cfg->gop[1].qp_factor = 0.3536; cfg->gop[1].is_ref = 1;
      cfg->gop[1].ref_neg_count = 2; cfg->gop[1].ref_neg[0] = 4; cfg->gop[1].ref_neg[1] = 8;
      cfg->gop[1].ref_pos_count = 1; cfg->gop[1].ref_pos[0] = 4;

      cfg->gop[2].poc_offset = 2; cfg->gop[2].qp_offset = 3; cfg->gop[2].layer = 3; cfg->gop[2].qp_factor = 0.3536; cfg->gop[2].is_ref = 1;
      cfg->gop[2].ref_neg_count = 2; cfg->gop[2].ref_neg[0] = 2; cfg->gop[2].ref_neg[1] = 6;
      cfg->gop[2].ref_pos_count = 2; cfg->gop[2].ref_pos[0] = 2; cfg->gop[2].ref_pos[1] = 6;

      cfg->gop[3].poc_offset = 1; cfg->gop[3].qp_offset = 4; cfg->gop[3].layer = 4; cfg->gop[3].qp_factor = 0.68;   cfg->gop[3].is_ref = 0;
      cfg->gop[3].ref_neg_count = 1; cfg->gop[3].ref_neg[0] = 1;
      cfg->gop[3].ref_pos_count = 3; cfg->gop[3].ref_pos[0] = 1; cfg->gop[3].ref_pos[1] = 3; cfg->gop[3].ref_pos[2] = 7;

      cfg->gop[4].poc_offset = 3; cfg->gop[4].qp_offset = 4; cfg->gop[4].layer = 4; cfg->gop[4].qp_factor = 0.68;   cfg->gop[4].is_ref = 0;
      cfg->gop[4].ref_neg_count = 2; cfg->gop[4].ref_neg[0] = 1; cfg->gop[4].ref_neg[1] = 3;
      cfg->gop[4].ref_pos_count = 2; cfg->gop[4].ref_pos[0] = 1; cfg->gop[4].ref_pos[1] = 5;

      cfg->gop[5].poc_offset = 6; cfg->gop[5].qp_offset = 3; cfg->gop[5].layer = 3; cfg->gop[5].qp_factor = 0.3536; cfg->gop[5].is_ref = 1;
      cfg->gop[5].ref_neg_count = 2; cfg->gop[5].ref_neg[0] = 2; cfg->gop[5].ref_neg[1] = 6;
      cfg->gop[5].ref_pos_count = 1; cfg->gop[5].ref_pos[0] = 2;

      cfg->gop[6].poc_offset = 5; cfg->gop[6].qp_offset = 4; cfg->gop[6].layer = 4; cfg->gop[6].qp_factor = 0.68;   cfg->gop[6].is_ref = 0;
      cfg->gop[6].ref_neg_count = 2;  cfg->gop[6].ref_neg[0] = 1; cfg->gop[6].ref_neg[1] = 5;
      cfg->gop[6].ref_pos_count = 2; cfg->gop[6].ref_pos[0] = 1; cfg->gop[6].ref_pos[1] = 3;

      cfg->gop[7].poc_offset = 7; cfg->gop[7].qp_offset = 4; cfg->gop[7].layer = 4; cfg->gop[7].qp_factor = 0.68;   cfg->gop[7].is_ref = 0;
      cfg->gop[7].ref_neg_count = 3; cfg->gop[7].ref_neg[0] = 1; cfg->gop[7].ref_neg[1] = 3; cfg->gop[7].ref_neg[2] = 7;
      cfg->gop[7].ref_pos_count = 1; cfg->gop[7].ref_pos[0] = 1;
    } else if (atoi(value) == 0) {
      //Disable gop
      cfg->gop_len = 0;
      cfg->gop_lowdelay = 0;
      cfg->gop_lp_definition.d = 0;
      cfg->gop_lp_definition.t = 0;
    } else if (atoi(value)) {
      fprintf(stderr, "Input error: unsupported gop length, must be 0 or 8\n");
      return 0;
    }
  }
  else if OPT("bipred")
    cfg->bipred = atobool(value);
  else if OPT("bitrate")
    cfg->target_bitrate = atoi(value);
  else if OPT("preset") {
    int preset_line = 0;

    // Accept numbers from 0 to 9.
    if ((atoi(value) == 0 && !strcmp(value, "0")) || (atoi(value) >= 1 && atoi(value) <= 9)) {
      preset_line = atoi(value);
    } else {
      // Find the selected preset from the list
      while (preset_values[preset_line][0] != NULL) {
        if (!strcmp(value, preset_values[preset_line][0])) {
          break;
        }
        preset_line++;
      }
    }

    if (preset_values[preset_line][0] != NULL) {
      fprintf(stderr, "Using preset %s: ", value);
      // Loop all the name and value pairs and push to the config parser
      for (int preset_value = 1; preset_values[preset_line][preset_value] != NULL; preset_value += 2) {
        fprintf(stderr, "--%s=%s ", preset_values[preset_line][preset_value], preset_values[preset_line][preset_value + 1]);
        kvz_config_parse(cfg, preset_values[preset_line][preset_value], preset_values[preset_line][preset_value + 1]);
      }
      fprintf(stderr, "\n");
    } else {
      fprintf(stderr, "Input error: unknown preset \"%s\"\n", value);
      return 0;
    }
  }
  else if OPT("mv-rdo")
    cfg->mv_rdo = atobool(value);
  else if OPT("psnr")
    cfg->calc_psnr = (bool)atobool(value);
  else if OPT("hash")
  {
    int8_t hash;
    int result;
    if ((result = parse_enum(value, hash_names, &hash))) {
      cfg->hash = hash;
    }
    return result;
  }
  else if OPT("cu-split-termination")
  {
    int8_t mode = KVZ_CU_SPLIT_TERMINATION_ZERO;
    int result = parse_enum(value, cu_split_termination_names, &mode);
    cfg->cu_split_termination = mode;
    return result;
  }
  else if OPT("crypto")
  {
    // on, off, feature1+feature2

    const char *token_begin = value;
    const char *cur = token_begin;

    cfg->crypto_features = KVZ_CRYPTO_OFF;

    // If value is on or off, set all features to on or off.
    int8_t toggle = 0;
    if (parse_enum(token_begin, crypto_toggle_names, &toggle)) {
      if (toggle == 1) {
        cfg->crypto_features = KVZ_CRYPTO_ON;
      }
    } else {
      // Try and parse "feature1+feature2" type list.
      for (;;) {
        if (*cur == '+' || *cur == '\0') {
          int8_t feature = 0;
          int num_chars = cur - token_begin;
          if (parse_enum_n(token_begin, num_chars, crypto_feature_names, &feature)) {
            cfg->crypto_features |= (1 << feature);
          } else {
            cfg->crypto_features = KVZ_CRYPTO_OFF;
            return 0;
          }
          token_begin = cur + 1;
        }

        if (*cur == '\0') {
          break;
        } else {
          ++cur;
        }
      }
    }

    // Disallow turning on the encryption when it's not compiled in.
    bool encryption_compiled_in = false;
#ifdef KVZ_SEL_ENCRYPTION
    encryption_compiled_in = true;
#endif
    if (!encryption_compiled_in && cfg->crypto_features) {
      fprintf(stderr, "--crypto cannot be enabled because it's not compiled in.\n");
      cfg->crypto_features = KVZ_CRYPTO_OFF;
      return 0;
    }

    return 1;
  }
  else if OPT("key"){
    int size_key = 16;
    FREE_POINTER(cfg->optional_key);
    cfg->optional_key = (uint8_t *)malloc(sizeof(uint8_t)*size_key);
    return parse_array(value, cfg->optional_key, size_key, 0, 255);
  }
  else if OPT("me-early-termination"){
    int8_t mode = 0;
    int result = parse_enum(value, me_early_termination_names, &mode);
    cfg->me_early_termination = mode;
    return result;
  }
  else if OPT("lossless")
    cfg->lossless = (bool)atobool(value);
  else if OPT("tmvp") {
    cfg->tmvp_enable = atobool(value);
    if (cfg->tiles_width_count > 1 || cfg->tiles_height_count > 1) {
      fprintf(stderr, "Cannot enable TMVP because tiles are used.\n");
      cfg->tmvp_enable = false;
    }
  }
  else if OPT("rdoq-skip"){
    cfg->rdoq_skip = atobool(value);
  }
  else if OPT("input-format") {
    static enum kvz_input_format const formats[] = { KVZ_FORMAT_P400, KVZ_FORMAT_P420 };
    static const char * const format_names[] = { "P400", "P420", NULL };

    int8_t format = 0;
    if (!parse_enum(value, format_names, &format)) {
      fprintf(stderr, "input-format not recognized.\n");
      return 0;
    }

    cfg->input_format = formats[format];
  }
  else if OPT("input-bitdepth") {
    cfg->input_bitdepth = atoi(value);
    if (cfg->input_bitdepth < 8 || cfg->input_bitdepth > 16) {
      fprintf(stderr, "input-bitdepth not between 8 and 16.\n");
      return 0;
    }
    if (cfg->input_bitdepth > 8 && KVZ_BIT_DEPTH == 8) {
      // Because the image is read straight into the reference buffers,
      // reading >8 bit samples doesn't work when sizeof(kvz_pixel)==1.
      fprintf(stderr, "input-bitdepth can't be set to larger than 8 because"
                      " Kvazaar is compiled with KVZ_BIT_DEPTH=8.\n");
      return 0;
    }
  }
  else if OPT("implicit-rdpcm")
    cfg->implicit_rdpcm = (bool)atobool(value);
  else if OPT("roi") {
    // The ROI description is as follows:
    // First number is width, second number is height,
    // then follows width * height number of dqp values.
    FILE* f = fopen(value, "rb");
    if (!f) {
      fprintf(stderr, "Could not open ROI file.\n");
      return 0;
    }

    int width = 0;
    int height = 0;
    if (!fscanf(f, "%d", &width) || !fscanf(f, "%d", &height)) {
      fprintf(stderr, "Failed to read ROI size.\n");
      fclose(f);
      return 0;
    }

    if (width <= 0 || height <= 0) {
      fprintf(stderr, "Invalid ROI size: %dx%d.\n", width, height);
      fclose(f);
      return 0;
    }

    if (width > 10000 || height > 10000) {
      fprintf(stderr, "ROI dimensions exceed arbitrary value of 10000.\n");
      return 0;
    }

    const unsigned size = width * height;
    int8_t *dqp_array  = calloc((size_t)size, sizeof(cfg->roi.dqps[0]));
    if (!dqp_array) {
      fprintf(stderr, "Failed to allocate memory for ROI table.\n");
      fclose(f);
      return 0;
    }

    FREE_POINTER(cfg->roi.dqps);
    cfg->roi.dqps   = dqp_array;
    cfg->roi.width  = width;
    cfg->roi.height = height;

    for (int i = 0; i < size; ++i) {
      int number; // Need a pointer to int for fscanf
      if (fscanf(f, "%d", &number) != 1) {
        fprintf(stderr, "Reading ROI file failed.\n");
        fclose(f);
        return 0;
      }
      dqp_array[i] = CLIP(-51, 51, number);
    }

    fclose(f);
  }
  else if OPT("erp-aqp")
    cfg->erp_aqp = (bool)atobool(value);
  else
    return 0;
#undef OPT

  return 1;
}

void kvz_config_process_lp_gop(kvz_config *cfg)
{
  struct {
    unsigned g;
    unsigned d;
    unsigned t;
  } gop;

  gop.g = cfg->gop_len;
  gop.d = cfg->gop_lp_definition.d;
  gop.t = cfg->gop_lp_definition.t;

  // Initialize modulos for testing depth.
  // The picture belong to the lowest depth in which (poc % modulo) == 0.
  unsigned depth_modulos[8] = { 0 };
  for (int d = 0; d < gop.d; ++d) {
    depth_modulos[gop.d - 1 - d] = 1 << d;
  }
  depth_modulos[0] = gop.g;

  cfg->gop_lowdelay = 1;
  cfg->gop_len = gop.g;
  for (int g = 1; g <= gop.g; ++g) {
    kvz_gop_config *gop_pic = &cfg->gop[g - 1];

    // Find gop depth for picture.
    int gop_layer = 1;
    while (gop_layer < gop.d && (g % depth_modulos[gop_layer - 1])) {
      ++gop_layer;
    }

    gop_pic->poc_offset = g;
    gop_pic->layer = gop_layer;
    gop_pic->qp_offset = gop_layer;
    gop_pic->ref_pos_count = 0;
    gop_pic->ref_neg_count = cfg->ref_frames;
    gop_pic->is_ref = 0;

    // Set first ref to point to previous frame, and the rest to previous
    // key-frames.
    // If gop.t > 1, have (poc % gop.t) == 0 point gop.t frames away,
    // instead of the previous frame. Set the frames in between to
    // point to the nearest frame with a lower gop-depth.
    if (gop.t > 1) {
      if (gop_pic->poc_offset % gop.t == 0) {
        gop_pic->ref_neg[0] = gop.t;
      } else {
        int r = gop_pic->poc_offset - 1;
        while (r > 0) {
          if (cfg->gop[r].layer < gop_pic->layer) break;
          --r;
        }
        // Var r is now 0 or index of the pic with layer < depth.
        if (cfg->gop[r].layer < gop_pic->layer) {
          gop_pic->ref_neg[0] = gop_pic->poc_offset - cfg->gop[r].poc_offset;
          cfg->gop[r].is_ref = 1;
        } else {
          // No ref was found, just refer to the previous key-frame.
          gop_pic->ref_neg[0] = gop_pic->poc_offset % gop.g;
        }
      }
    } else {
      gop_pic->ref_neg[0] = 1;
      if (gop_pic->poc_offset >= 2) {
        cfg->gop[gop_pic->poc_offset - 2].is_ref = 1;
      }
    }

    int keyframe = gop_pic->poc_offset;
    for (int i = 1; i < gop_pic->ref_neg_count; ++i) {
      while (keyframe == gop_pic->ref_neg[i - 1]) {
        keyframe += gop.g;
      }
      gop_pic->ref_neg[i] = keyframe;
    }

    gop_pic->qp_factor = 0.4624;  // from HM
  }

  for (int g = 0; g < gop.g; ++g) {
    kvz_gop_config *gop_pic = &cfg->gop[g];
    if (!gop_pic->is_ref) {
      gop_pic->qp_factor = 0.68 * 1.31;  // derived from HM
    }
  }

  // Key-frame is always a reference.
  cfg->gop[gop.g - 1].is_ref = 1;
  cfg->gop[gop.g - 1].qp_factor = 0.578;  // from HM
}

/**
 * \brief Check that configuration is sensible.
 *
 * \param cfg   config to check
 * \return      1 if the config is ok, otherwise 1
 */
int kvz_config_validate(const kvz_config *const cfg)
{
  int error = 0;

  if (cfg->width <= 0) {
    fprintf(stderr, "Input error: width must be positive\n");
    error = 1;
  }

  if (cfg->height <= 0) {
    fprintf(stderr, "Input error: height must be positive\n");
    error = 1;
  }

  if (cfg->width % 2 != 0) {
    fprintf(stderr, "Input error: width must be a multiple of two\n");
    error = 1;
  }

  if (cfg->height % 2 != 0) {
    fprintf(stderr, "Input error: height must be a multiple of two\n");
    error = 1;
  }

  if (cfg->width > 0 && cfg->height > 0) {
    // We must be able to store the total number of luma and chroma pixels
    // in an int32_t. For 4:4:4 chroma mode, the number of pixels is
    // 3 * width * height. Width and height are rounded up to a multiple of
    // LCU size.
    const uint32_t max_lcus = INT_MAX / (3 * LCU_WIDTH * LCU_WIDTH);
    const uint64_t num_lcus = CEILDIV((uint64_t)cfg->width,  LCU_WIDTH) *
                              CEILDIV((uint64_t)cfg->height, LCU_WIDTH);
    if (num_lcus > max_lcus) {
      fprintf(stderr, "Input error: resolution %dx%d too large (max %u CTUs)\n",
              cfg->width, cfg->height, max_lcus);
      error = 1;
    }
  }

  if (cfg->framerate < 0.0) {
    fprintf(stderr, "Input error: --input-fps must be positive\n");
    error = 1;
  }
  if (cfg->framerate_num < 0) {
    fprintf(stderr, "Input error: --input-fps must >=0\n");
    error = 1;
  }
  if (cfg->framerate_denom <= 0) {
    fprintf(stderr, "Input error: --input-fps denominator must be >0\n");
    error = 1;
  }

  if (cfg->gop_len && cfg->intra_period && !cfg->gop_lowdelay &&
      cfg->intra_period % cfg->gop_len != 0)
  {
    fprintf(stderr,
            "Input error: intra period (%d) not a multiple of B-gop length (%d)\n",
            cfg->intra_period,
            cfg->gop_len);
    error = 1;
  }

  if (cfg->ref_frames  < 1 || cfg->ref_frames >= MAX_REF_PIC_COUNT) {
    fprintf(stderr, "Input error: --ref out of range [1..%d]\n", MAX_REF_PIC_COUNT - 1);
    error = 1;
  }

  if (cfg->deblock_beta  < -6 || cfg->deblock_beta  > 6) {
    fprintf(stderr, "Input error: deblock beta parameter out of range [-6..6]\n");
    error = 1;
  }
  if (cfg->deblock_tc < -6 || cfg->deblock_tc > 6) {
    fprintf(stderr, "Input error: deblock tc parameter out of range [-6..6]\n");
    error = 1;
  }

  if (cfg->rdo < 0 || cfg->rdo > 3) {
    fprintf(stderr, "Input error: --rd parameter out of range [0..3]\n");
    error = 1;
  }

  if (cfg->tr_depth_intra < 0 || cfg->tr_depth_intra > 4) {
    // range is 0 .. CtbLog2SizeY - Log2MinTrafoSize
    fprintf(stderr, "Input error: --tr-depth-intra is out of range [0..4]\n");
    error = 1;
  }

  if (cfg->fme_level != 0 && cfg->fme_level > 4) {
    fprintf(stderr, "Input error: invalid --subme parameter (must be in range 0-4)\n");
    error = 1;
  }

  if (cfg->vui.chroma_loc < 0 || cfg->vui.chroma_loc > 5) {
    fprintf(stderr, "Input error: --chromaloc parameter out of range [0..5]\n");
    error = 1;
  }

  if (cfg->owf < -1) {
    fprintf(stderr, "Input error: --owf must be nonnegative or -1\n");
    error = 1;
  }

  if (cfg->qp != CLIP_TO_QP(cfg->qp)) {
      fprintf(stderr, "Input error: --qp parameter out of range [0..51]\n");
      error = 1;
  }

  if (cfg->target_bitrate < 0) {
      fprintf(stderr, "Input error: --bitrate must be nonnegative\n");
      error = 1;
  }

  if (!WITHIN(cfg->pu_depth_inter.min, PU_DEPTH_INTER_MIN, PU_DEPTH_INTER_MAX) ||
      !WITHIN(cfg->pu_depth_inter.max, PU_DEPTH_INTER_MIN, PU_DEPTH_INTER_MAX)) 
  {
    fprintf(stderr, "Input error: illegal value for --pu-depth-inter (%d-%d)\n",
            cfg->pu_depth_inter.min, cfg->pu_depth_inter.max);
    error = 1;
  } else if (cfg->pu_depth_inter.min > cfg->pu_depth_inter.max) {
    fprintf(stderr, "Input error: Inter PU depth min (%d) > max (%d)\n",
            cfg->pu_depth_inter.min, cfg->pu_depth_inter.max);
    error = 1;
  }

  if (!WITHIN(cfg->pu_depth_intra.min, PU_DEPTH_INTRA_MIN, PU_DEPTH_INTRA_MAX) ||
      !WITHIN(cfg->pu_depth_intra.max, PU_DEPTH_INTRA_MIN, PU_DEPTH_INTRA_MAX))
  {
    fprintf(stderr, "Input error: illegal value for --pu-depth-intra (%d-%d)\n",
      cfg->pu_depth_intra.min, cfg->pu_depth_intra.max);
    error = 1;
  } else if (cfg->pu_depth_intra.min > cfg->pu_depth_intra.max) {
    fprintf(stderr, "Input error: Intra PU depth min (%d) > max (%d)\n",
            cfg->pu_depth_intra.min, cfg->pu_depth_intra.max);
    error = 1;
  }

  // Tile separation should be at round position in terms of LCU, should be monotonic, and should not start by 0
  if (cfg->tiles_width_split) {
    int i;
    int32_t prev_tile_split = 0;
    for (i=0; i < cfg->tiles_width_count - 1; ++i) {
      if (cfg->tiles_width_split[i] <= prev_tile_split) {
        fprintf(stderr, "Input error: tile separations in width should be strictly monotonic (%d <= %d)\n", cfg->tiles_width_split[i], prev_tile_split);
        error = 1;
        break;
      }
      if ((cfg->tiles_width_split[i] % LCU_WIDTH) != 0) {
        fprintf(stderr, "Input error: tile separation in width %d (at %d) is not at a multiple of LCU_WIDTH (%d)\n", i, cfg->tiles_width_split[i], LCU_WIDTH);
        error = 1;
        break;
      }
      prev_tile_split = cfg->tiles_width_split[i];
    }
    if (cfg->tiles_width_split[cfg->tiles_width_count - 2] >= cfg->width) {
      fprintf(stderr, "Input error: last x tile separation in width (%d) should smaller than image width (%d)\n", cfg->tiles_width_split[cfg->tiles_width_count - 2], cfg->width);
      error = 1;
    }
  }

  if (cfg->tiles_height_split) {
    int i;
    int32_t prev_tile_split = 0;
    for (i=0; i < cfg->tiles_height_count - 1; ++i) {
      if (cfg->tiles_height_split[i] <= prev_tile_split) {
        fprintf(stderr, "Input error: tile separations in height should be strictly monotonic (%d <= %d)\n", cfg->tiles_height_split[i], prev_tile_split);
        error = 1;
        break;
      }
      if ((cfg->tiles_height_split[i] % LCU_WIDTH) != 0) {
        fprintf(stderr, "Input error: tile separation in height %d (at %d) is not at a multiple of LCU_WIDTH (%d)\n", i, cfg->tiles_height_split[i], LCU_WIDTH);
        error = 1;
        break;
      }
      prev_tile_split = cfg->tiles_height_split[i];
    }

    if (cfg->tiles_height_split[cfg->tiles_height_count - 2] >= cfg->height) {
      fprintf(stderr, "Input error: last tile separation in height (%d) should smaller than image height (%d)\n", cfg->tiles_height_split[cfg->tiles_height_count - 2], cfg->height);
      error = 1;
    }
  }

  if (cfg->implicit_rdpcm && !cfg->lossless) {
    fprintf(stderr, "Input error: --implicit-rdpcm is not suppoted without --lossless\n");
    error = 1;
  }

  if ((cfg->slices & KVZ_SLICES_WPP) && !cfg->wpp) {
    fprintf(stderr, "Input error: --slices=wpp does not work without --wpp.\n");
    error = 1;
  }

  return !error;
}
