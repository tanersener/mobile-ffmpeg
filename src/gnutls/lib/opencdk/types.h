/* types.h - Some type definitions
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 *
 * Author: Timo Schulz
 *
 * This file is part of OpenCDK.
 *
 * The OpenCDK library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef CDK_TYPES_H
#define CDK_TYPES_H

#ifndef HAVE_BYTE_TYPEDEF
#undef byte
typedef unsigned char byte;
#define HAVE_BYTE_TYPEDEF
#endif

#ifndef HAVE_U16_TYPEDEF
#undef u16
typedef unsigned short u16;
#define HAVE_U16_TYPEDEF
#endif

#ifndef HAVE_U32_TYPEDEF
#undef u32
typedef unsigned int u32;
#define HAVE_U32_TYPEDEF
#endif

#ifndef DIM
#define DIM(v) (sizeof (v)/sizeof ((v)[0]))
#define DIMof(type, member)   DIM(((type *)0)->member)
#endif

#endif				/* CDK_TYPES_H */
