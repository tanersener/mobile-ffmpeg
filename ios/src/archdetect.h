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

#ifndef MOBILEFFMPEG_ARCHDETECT_H
#define MOBILEFFMPEG_ARCHDETECT_H

#include "log.h"
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/machine.h>

/** Represents armv7 architecture. */
#define MF_ARCH_ARMV7 "armv7"

/** Represents armv7s architecture. */
#define MF_ARCH_ARMV7S "armv7s"

/** Represents arm64 architecture. */
#define MF_ARCH_ARM64 "arm64"

/** Represents i386 architecture. */
#define MF_ARCH_I386 "i386"

/** Represents x86_64 architecture. */
#define MF_ARCH_X86_64 "x86_64"

/** Represents not supported architectures. */
#define MF_ABI_UNKNOWN "unknown"

const char *mobileffmpeg_get_arch(void);

#endif /* MOBILEFFMPEG_ARCHDETECT_H */
