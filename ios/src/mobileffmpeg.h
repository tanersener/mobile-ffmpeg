/*
 * Copyright (c) 2018 Taner Sener
 *
 * This file is part of MobileFFmpeg.
 *
 * MobileFFmpeg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MobileFFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MOBILEFFMPEG_H
#define MOBILEFFMPEG_H

#include <string.h>
#include <stdlib.h>

#include "libavutil/ffversion.h"
#include "log.h"

/** Library version string */
#define MOBILE_FFMPEG_VERSION "1.2"

const char *mobileffmpeg_get_ffmpeg_version(void);

const char *mobileffmpeg_get_version(void);

int mobileffmpeg_execute(int argc, char **argv);

#endif /* MOBILEFFMPEG_H */
