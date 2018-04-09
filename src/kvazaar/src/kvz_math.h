#ifndef MATH_H_
#define MATH_H_
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
* Generic math functions
*/

#include "global.h" // IWYU pragma: keep


static INLINE unsigned kvz_math_floor_log2(unsigned value)
{
  assert(value > 0);

  unsigned result = 0;

  for (int i = 4; i >= 0; --i) {
    unsigned bits = 1ull << i;
    unsigned shift = value >= (1 << bits) ? bits : 0;
    result += shift;
    value >>= shift;
  }

  return result;
}

static INLINE unsigned kvz_math_ceil_log2(unsigned value)
{
  assert(value > 0);

  // The ceil_log2 is just floor_log2 + 1, except for exact powers of 2.
  return kvz_math_floor_log2(value) + ((value & (value - 1)) ? 1 : 0);
}

#endif //CHECKPOINT_H_
