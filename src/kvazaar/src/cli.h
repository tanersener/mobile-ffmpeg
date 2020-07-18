#ifndef CLI_H_
#define CLI_H_
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
 * \file
 * Command line interface
 */

#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"

typedef struct cmdline_opts_t {
  /** \brief Input filename */
  char *input;
  /** \brief Output filename */
  char *output;
  /** \brief Debug output */
  char *debug;
  /** \brief Number of input frames to skip */
  int32_t seek;
  /** \brief Number of frames to encode */
  int32_t frames;
  /** \brief Encoder configuration */
  kvz_config *config;
  /** \brief Encoder configuration */
  bool help;
  /** \brief Encoder configuration */
  bool version;
  /** \brief Whether to loop input */
  bool loop_input;
} cmdline_opts_t;

cmdline_opts_t* cmdline_opts_parse(const kvz_api *api, int argc, char *argv[]);
void cmdline_opts_free(const kvz_api *api, cmdline_opts_t *opts);

void print_usage(void);
void print_version(void);
void print_help(void);
void print_frame_info(const kvz_frame_info *const info,
                      const double frame_psnr[3],
                      const uint32_t bytes,
                      const bool print_psnr,
                      const double avg_qp);

#endif
