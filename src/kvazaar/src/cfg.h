#ifndef CONFIG_H_
#define CONFIG_H_
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
 * Runtime configuration through defaults and parsing of arguments.
 */

#include "global.h" // IWYU pragma: keep

#include "kvazaar.h"


/* Function definitions */
kvz_config *kvz_config_alloc(void);
int kvz_config_init(kvz_config *cfg);
int kvz_config_destroy(kvz_config *cfg);
int kvz_config_parse(kvz_config *cfg, const char *name, const char *value);
void kvz_config_process_lp_gop(kvz_config *cfg);
int kvz_config_validate(const kvz_config *cfg);

#endif
