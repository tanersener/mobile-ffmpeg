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

#include "strategies/generic/dct-generic.h"

#include "strategyselector.h"
#include "tables.h"

const int16_t kvz_g_dst_4[4][4] =
{
  { 29, 55, 74, 84 },
  { 74, 74, 0, -74 },
  { 84, -29, -74, 55 },
  { 55, -84, 74, -29 }
};

const int16_t kvz_g_dct_4[4][4] =
{
  { 64, 64, 64, 64 },
  { 83, 36, -36, -83 },
  { 64, -64, -64, 64 },
  { 36, -83, 83, -36 }
};

const int16_t kvz_g_dct_8[8][8] =
{
  { 64, 64, 64, 64, 64, 64, 64, 64 },
  { 89, 75, 50, 18, -18, -50, -75, -89 },
  { 83, 36, -36, -83, -83, -36, 36, 83 },
  { 75, -18, -89, -50, 50, 89, 18, -75 },
  { 64, -64, -64, 64, 64, -64, -64, 64 },
  { 50, -89, 18, 75, -75, -18, 89, -50 },
  { 36, -83, 83, -36, -36, 83, -83, 36 },
  { 18, -50, 75, -89, 89, -75, 50, -18 }
};

const int16_t kvz_g_dct_16[16][16] =
{
  { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 },
  { 90, 87, 80, 70, 57, 43, 25, 9, -9, -25, -43, -57, -70, -80, -87, -90 },
  { 89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89 },
  { 87, 57, 9, -43, -80, -90, -70, -25, 25, 70, 90, 80, 43, -9, -57, -87 },
  { 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83 },
  { 80, 9, -70, -87, -25, 57, 90, 43, -43, -90, -57, 25, 87, 70, -9, -80 },
  { 75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75 },
  { 70, -43, -87, 9, 90, 25, -80, -57, 57, 80, -25, -90, -9, 87, 43, -70 },
  { 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64 },
  { 57, -80, -25, 90, -9, -87, 43, 70, -70, -43, 87, 9, -90, 25, 80, -57 },
  { 50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50 },
  { 43, -90, 57, 25, -87, 70, 9, -80, 80, -9, -70, 87, -25, -57, 90, -43 },
  { 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36 },
  { 25, -70, 90, -80, 43, 9, -57, 87, -87, 57, -9, -43, 80, -90, 70, -25 },
  { 18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18 },
  { 9, -25, 43, -57, 70, -80, 87, -90, 90, -87, 80, -70, 57, -43, 25, -9 }
};

const int16_t kvz_g_dct_32[32][32] =
{
  { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 },
  { 90, 90, 88, 85, 82, 78, 73, 67, 61, 54, 46, 38, 31, 22, 13, 4, -4, -13, -22, -31, -38, -46, -54, -61, -67, -73, -78, -82, -85, -88, -90, -90 },
  { 90, 87, 80, 70, 57, 43, 25, 9, -9, -25, -43, -57, -70, -80, -87, -90, -90, -87, -80, -70, -57, -43, -25, -9, 9, 25, 43, 57, 70, 80, 87, 90 },
  { 90, 82, 67, 46, 22, -4, -31, -54, -73, -85, -90, -88, -78, -61, -38, -13, 13, 38, 61, 78, 88, 90, 85, 73, 54, 31, 4, -22, -46, -67, -82, -90 },
  { 89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89, 89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89 },
  { 88, 67, 31, -13, -54, -82, -90, -78, -46, -4, 38, 73, 90, 85, 61, 22, -22, -61, -85, -90, -73, -38, 4, 46, 78, 90, 82, 54, 13, -31, -67, -88 },
  { 87, 57, 9, -43, -80, -90, -70, -25, 25, 70, 90, 80, 43, -9, -57, -87, -87, -57, -9, 43, 80, 90, 70, 25, -25, -70, -90, -80, -43, 9, 57, 87 },
  { 85, 46, -13, -67, -90, -73, -22, 38, 82, 88, 54, -4, -61, -90, -78, -31, 31, 78, 90, 61, 4, -54, -88, -82, -38, 22, 73, 90, 67, 13, -46, -85 },
  { 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83 },
  { 82, 22, -54, -90, -61, 13, 78, 85, 31, -46, -90, -67, 4, 73, 88, 38, -38, -88, -73, -4, 67, 90, 46, -31, -85, -78, -13, 61, 90, 54, -22, -82 },
  { 80, 9, -70, -87, -25, 57, 90, 43, -43, -90, -57, 25, 87, 70, -9, -80, -80, -9, 70, 87, 25, -57, -90, -43, 43, 90, 57, -25, -87, -70, 9, 80 },
  { 78, -4, -82, -73, 13, 85, 67, -22, -88, -61, 31, 90, 54, -38, -90, -46, 46, 90, 38, -54, -90, -31, 61, 88, 22, -67, -85, -13, 73, 82, 4, -78 },
  { 75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75, 75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75 },
  { 73, -31, -90, -22, 78, 67, -38, -90, -13, 82, 61, -46, -88, -4, 85, 54, -54, -85, 4, 88, 46, -61, -82, 13, 90, 38, -67, -78, 22, 90, 31, -73 },
  { 70, -43, -87, 9, 90, 25, -80, -57, 57, 80, -25, -90, -9, 87, 43, -70, -70, 43, 87, -9, -90, -25, 80, 57, -57, -80, 25, 90, 9, -87, -43, 70 },
  { 67, -54, -78, 38, 85, -22, -90, 4, 90, 13, -88, -31, 82, 46, -73, -61, 61, 73, -46, -82, 31, 88, -13, -90, -4, 90, 22, -85, -38, 78, 54, -67 },
  { 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64 },
  { 61, -73, -46, 82, 31, -88, -13, 90, -4, -90, 22, 85, -38, -78, 54, 67, -67, -54, 78, 38, -85, -22, 90, 4, -90, 13, 88, -31, -82, 46, 73, -61 },
  { 57, -80, -25, 90, -9, -87, 43, 70, -70, -43, 87, 9, -90, 25, 80, -57, -57, 80, 25, -90, 9, 87, -43, -70, 70, 43, -87, -9, 90, -25, -80, 57 },
  { 54, -85, -4, 88, -46, -61, 82, 13, -90, 38, 67, -78, -22, 90, -31, -73, 73, 31, -90, 22, 78, -67, -38, 90, -13, -82, 61, 46, -88, 4, 85, -54 },
  { 50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50, 50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50 },
  { 46, -90, 38, 54, -90, 31, 61, -88, 22, 67, -85, 13, 73, -82, 4, 78, -78, -4, 82, -73, -13, 85, -67, -22, 88, -61, -31, 90, -54, -38, 90, -46 },
  { 43, -90, 57, 25, -87, 70, 9, -80, 80, -9, -70, 87, -25, -57, 90, -43, -43, 90, -57, -25, 87, -70, -9, 80, -80, 9, 70, -87, 25, 57, -90, 43 },
  { 38, -88, 73, -4, -67, 90, -46, -31, 85, -78, 13, 61, -90, 54, 22, -82, 82, -22, -54, 90, -61, -13, 78, -85, 31, 46, -90, 67, 4, -73, 88, -38 },
  { 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36 },
  { 31, -78, 90, -61, 4, 54, -88, 82, -38, -22, 73, -90, 67, -13, -46, 85, -85, 46, 13, -67, 90, -73, 22, 38, -82, 88, -54, -4, 61, -90, 78, -31 },
  { 25, -70, 90, -80, 43, 9, -57, 87, -87, 57, -9, -43, 80, -90, 70, -25, -25, 70, -90, 80, -43, -9, 57, -87, 87, -57, 9, 43, -80, 90, -70, 25 },
  { 22, -61, 85, -90, 73, -38, -4, 46, -78, 90, -82, 54, -13, -31, 67, -88, 88, -67, 31, 13, -54, 82, -90, 78, -46, 4, 38, -73, 90, -85, 61, -22 },
  { 18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18, 18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18 },
  { 13, -38, 61, -78, 88, -90, 85, -73, 54, -31, 4, 22, -46, 67, -82, 90, -90, 82, -67, 46, -22, -4, 31, -54, 73, -85, 90, -88, 78, -61, 38, -13 },
  { 9, -25, 43, -57, 70, -80, 87, -90, 90, -87, 80, -70, 57, -43, 25, -9, -9, 25, -43, 57, -70, 80, -87, 90, -90, 87, -80, 70, -57, 43, -25, 9 },
  { 4, -13, 22, -31, 38, -46, 54, -61, 67, -73, 78, -82, 85, -88, 90, -90, 90, -90, 88, -85, 82, -78, 73, -67, 61, -54, 46, -38, 31, -22, 13, -4 }
};

const int16_t kvz_g_dst_4_t[4][4] =
{
  { 29, 74, 84, 55 },
  { 55, 74, -29, -84 },
  { 74, 0, -74, 74 },
  { 84, -74, 55, -29 }
};

const int16_t kvz_g_dct_4_t[4][4] =
{
  { 64, 83, 64, 36, },
  { 64, 36, -64, -83, },
  { 64, -36, -64, 83, },
  { 64, -83, 64, -36 }
};

const int16_t kvz_g_dct_8_t[8][8] =
{
  { 64, 89, 83, 75, 64, 50, 36, 18, },
  { 64, 75, 36, -18, -64, -89, -83, -50, },
  { 64, 50, -36, -89, -64, 18, 83, 75, },
  { 64, 18, -83, -50, 64, 75, -36, -89, },
  { 64, -18, -83, 50, 64, -75, -36, 89, },
  { 64, -50, -36, 89, -64, -18, 83, -75, },
  { 64, -75, 36, 18, -64, 89, -83, 50, },
  { 64, -89, 83, -75, 64, -50, 36, -18 }
};

const int16_t kvz_g_dct_16_t[16][16] =
{
  { 64, 90, 89, 87, 83, 80, 75, 70, 64, 57, 50, 43, 36, 25, 18, 9, },
  { 64, 87, 75, 57, 36, 9, -18, -43, -64, -80, -89, -90, -83, -70, -50, -25, },
  { 64, 80, 50, 9, -36, -70, -89, -87, -64, -25, 18, 57, 83, 90, 75, 43, },
  { 64, 70, 18, -43, -83, -87, -50, 9, 64, 90, 75, 25, -36, -80, -89, -57, },
  { 64, 57, -18, -80, -83, -25, 50, 90, 64, -9, -75, -87, -36, 43, 89, 70, },
  { 64, 43, -50, -90, -36, 57, 89, 25, -64, -87, -18, 70, 83, 9, -75, -80, },
  { 64, 25, -75, -70, 36, 90, 18, -80, -64, 43, 89, 9, -83, -57, 50, 87, },
  { 64, 9, -89, -25, 83, 43, -75, -57, 64, 70, -50, -80, 36, 87, -18, -90, },
  { 64, -9, -89, 25, 83, -43, -75, 57, 64, -70, -50, 80, 36, -87, -18, 90, },
  { 64, -25, -75, 70, 36, -90, 18, 80, -64, -43, 89, -9, -83, 57, 50, -87, },
  { 64, -43, -50, 90, -36, -57, 89, -25, -64, 87, -18, -70, 83, -9, -75, 80, },
  { 64, -57, -18, 80, -83, 25, 50, -90, 64, 9, -75, 87, -36, -43, 89, -70, },
  { 64, -70, 18, 43, -83, 87, -50, -9, 64, -90, 75, -25, -36, 80, -89, 57, },
  { 64, -80, 50, -9, -36, 70, -89, 87, -64, 25, 18, -57, 83, -90, 75, -43, },
  { 64, -87, 75, -57, 36, -9, -18, 43, -64, 80, -89, 90, -83, 70, -50, 25, },
  { 64, -90, 89, -87, 83, -80, 75, -70, 64, -57, 50, -43, 36, -25, 18, -9 }
};

const int16_t kvz_g_dct_32_t[32][32] =
{
  { 64, 90, 90, 90, 89, 88, 87, 85, 83, 82, 80, 78, 75, 73, 70, 67, 64, 61, 57, 54, 50, 46, 43, 38, 36, 31, 25, 22, 18, 13, 9, 4, },
  { 64, 90, 87, 82, 75, 67, 57, 46, 36, 22, 9, -4, -18, -31, -43, -54, -64, -73, -80, -85, -89, -90, -90, -88, -83, -78, -70, -61, -50, -38, -25, -13, },
  { 64, 88, 80, 67, 50, 31, 9, -13, -36, -54, -70, -82, -89, -90, -87, -78, -64, -46, -25, -4, 18, 38, 57, 73, 83, 90, 90, 85, 75, 61, 43, 22, },
  { 64, 85, 70, 46, 18, -13, -43, -67, -83, -90, -87, -73, -50, -22, 9, 38, 64, 82, 90, 88, 75, 54, 25, -4, -36, -61, -80, -90, -89, -78, -57, -31, },
  { 64, 82, 57, 22, -18, -54, -80, -90, -83, -61, -25, 13, 50, 78, 90, 85, 64, 31, -9, -46, -75, -90, -87, -67, -36, 4, 43, 73, 89, 88, 70, 38, },
  { 64, 78, 43, -4, -50, -82, -90, -73, -36, 13, 57, 85, 89, 67, 25, -22, -64, -88, -87, -61, -18, 31, 70, 90, 83, 54, 9, -38, -75, -90, -80, -46, },
  { 64, 73, 25, -31, -75, -90, -70, -22, 36, 78, 90, 67, 18, -38, -80, -90, -64, -13, 43, 82, 89, 61, 9, -46, -83, -88, -57, -4, 50, 85, 87, 54, },
  { 64, 67, 9, -54, -89, -78, -25, 38, 83, 85, 43, -22, -75, -90, -57, 4, 64, 90, 70, 13, -50, -88, -80, -31, 36, 82, 87, 46, -18, -73, -90, -61, },
  { 64, 61, -9, -73, -89, -46, 25, 82, 83, 31, -43, -88, -75, -13, 57, 90, 64, -4, -70, -90, -50, 22, 80, 85, 36, -38, -87, -78, -18, 54, 90, 67, },
  { 64, 54, -25, -85, -75, -4, 70, 88, 36, -46, -90, -61, 18, 82, 80, 13, -64, -90, -43, 38, 89, 67, -9, -78, -83, -22, 57, 90, 50, -31, -87, -73, },
  { 64, 46, -43, -90, -50, 38, 90, 54, -36, -90, -57, 31, 89, 61, -25, -88, -64, 22, 87, 67, -18, -85, -70, 13, 83, 73, -9, -82, -75, 4, 80, 78, },
  { 64, 38, -57, -88, -18, 73, 80, -4, -83, -67, 25, 90, 50, -46, -90, -31, 64, 85, 9, -78, -75, 13, 87, 61, -36, -90, -43, 54, 89, 22, -70, -82, },
  { 64, 31, -70, -78, 18, 90, 43, -61, -83, 4, 87, 54, -50, -88, -9, 82, 64, -38, -90, -22, 75, 73, -25, -90, -36, 67, 80, -13, -89, -46, 57, 85, },
  { 64, 22, -80, -61, 50, 85, -9, -90, -36, 73, 70, -38, -89, -4, 87, 46, -64, -78, 25, 90, 18, -82, -57, 54, 83, -13, -90, -31, 75, 67, -43, -88, },
  { 64, 13, -87, -38, 75, 61, -57, -78, 36, 88, -9, -90, -18, 85, 43, -73, -64, 54, 80, -31, -89, 4, 90, 22, -83, -46, 70, 67, -50, -82, 25, 90, },
  { 64, 4, -90, -13, 89, 22, -87, -31, 83, 38, -80, -46, 75, 54, -70, -61, 64, 67, -57, -73, 50, 78, -43, -82, 36, 85, -25, -88, 18, 90, -9, -90, },
  { 64, -4, -90, 13, 89, -22, -87, 31, 83, -38, -80, 46, 75, -54, -70, 61, 64, -67, -57, 73, 50, -78, -43, 82, 36, -85, -25, 88, 18, -90, -9, 90, },
  { 64, -13, -87, 38, 75, -61, -57, 78, 36, -88, -9, 90, -18, -85, 43, 73, -64, -54, 80, 31, -89, -4, 90, -22, -83, 46, 70, -67, -50, 82, 25, -90, },
  { 64, -22, -80, 61, 50, -85, -9, 90, -36, -73, 70, 38, -89, 4, 87, -46, -64, 78, 25, -90, 18, 82, -57, -54, 83, 13, -90, 31, 75, -67, -43, 88, },
  { 64, -31, -70, 78, 18, -90, 43, 61, -83, -4, 87, -54, -50, 88, -9, -82, 64, 38, -90, 22, 75, -73, -25, 90, -36, -67, 80, 13, -89, 46, 57, -85, },
  { 64, -38, -57, 88, -18, -73, 80, 4, -83, 67, 25, -90, 50, 46, -90, 31, 64, -85, 9, 78, -75, -13, 87, -61, -36, 90, -43, -54, 89, -22, -70, 82, },
  { 64, -46, -43, 90, -50, -38, 90, -54, -36, 90, -57, -31, 89, -61, -25, 88, -64, -22, 87, -67, -18, 85, -70, -13, 83, -73, -9, 82, -75, -4, 80, -78, },
  { 64, -54, -25, 85, -75, 4, 70, -88, 36, 46, -90, 61, 18, -82, 80, -13, -64, 90, -43, -38, 89, -67, -9, 78, -83, 22, 57, -90, 50, 31, -87, 73, },
  { 64, -61, -9, 73, -89, 46, 25, -82, 83, -31, -43, 88, -75, 13, 57, -90, 64, 4, -70, 90, -50, -22, 80, -85, 36, 38, -87, 78, -18, -54, 90, -67, },
  { 64, -67, 9, 54, -89, 78, -25, -38, 83, -85, 43, 22, -75, 90, -57, -4, 64, -90, 70, -13, -50, 88, -80, 31, 36, -82, 87, -46, -18, 73, -90, 61, },
  { 64, -73, 25, 31, -75, 90, -70, 22, 36, -78, 90, -67, 18, 38, -80, 90, -64, 13, 43, -82, 89, -61, 9, 46, -83, 88, -57, 4, 50, -85, 87, -54, },
  { 64, -78, 43, 4, -50, 82, -90, 73, -36, -13, 57, -85, 89, -67, 25, 22, -64, 88, -87, 61, -18, -31, 70, -90, 83, -54, 9, 38, -75, 90, -80, 46, },
  { 64, -82, 57, -22, -18, 54, -80, 90, -83, 61, -25, -13, 50, -78, 90, -85, 64, -31, -9, 46, -75, 90, -87, 67, -36, -4, 43, -73, 89, -88, 70, -38, },
  { 64, -85, 70, -46, 18, 13, -43, 67, -83, 90, -87, 73, -50, 22, 9, -38, 64, -82, 90, -88, 75, -54, 25, 4, -36, 61, -80, 90, -89, 78, -57, 31, },
  { 64, -88, 80, -67, 50, -31, 9, 13, -36, 54, -70, 82, -89, 90, -87, 78, -64, 46, -25, 4, 18, -38, 57, -73, 83, -90, 90, -85, 75, -61, 43, -22, },
  { 64, -90, 87, -82, 75, -67, 57, -46, 36, -22, 9, 4, -18, 31, -43, 54, -64, 73, -80, 85, -89, 90, -90, 88, -83, 78, -70, 61, -50, 38, -25, 13, },
  { 64, -90, 90, -90, 89, -88, 87, -85, 83, -82, 80, -78, 75, -73, 70, -67, 64, -61, 57, -54, 50, -46, 43, -38, 36, -31, 25, -22, 18, -13, 9, -4 }
};

/**
 * \brief Generic partial butterfly functions
 *
 * TODO: description
 *
 * \param TODO   
 *
 * \returns TODO
 */

// Fast DST Algorithm. Full matrix multiplication for DST and Fast DST algorithm
// gives identical results
static void fast_forward_dst_4_generic(const short *block, short *coeff, int32_t shift)  // input block, output coeff
{
  int32_t i, c[4];
  int32_t rnd_factor = 1 << (shift - 1);
  for (i = 0; i < 4; i++) {
    // int32_termediate Variables
    c[0] = block[4 * i + 0] + block[4 * i + 3];
    c[1] = block[4 * i + 1] + block[4 * i + 3];
    c[2] = block[4 * i + 0] - block[4 * i + 1];
    c[3] = 74 * block[4 * i + 2];

    coeff[i] = (short)((29 * c[0] + 55 * c[1] + c[3] + rnd_factor) >> shift);
    coeff[4 + i] = (short)((74 * (block[4 * i + 0] + block[4 * i + 1] - block[4 * i + 3]) + rnd_factor) >> shift);
    coeff[8 + i] = (short)((29 * c[2] + 55 * c[0] - c[3] + rnd_factor) >> shift);
    coeff[12 + i] = (short)((55 * c[2] - 29 * c[1] + c[3] + rnd_factor) >> shift);
  }
}

static void fast_inverse_dst_4_generic(const short *tmp, short *block, int shift)  // input tmp, output block
{
  int i, c[4];
  int rnd_factor = 1 << (shift - 1);
  for (i = 0; i < 4; i++) {
    // Intermediate Variables
    c[0] = tmp[i] + tmp[8 + i];
    c[1] = tmp[8 + i] + tmp[12 + i];
    c[2] = tmp[i] - tmp[12 + i];
    c[3] = 74 * tmp[4 + i];

    block[4 * i + 0] = (short)CLIP(-32768, 32767, (29 * c[0] + 55 * c[1] + c[3] + rnd_factor) >> shift);
    block[4 * i + 1] = (short)CLIP(-32768, 32767, (55 * c[2] - 29 * c[1] + c[3] + rnd_factor) >> shift);
    block[4 * i + 2] = (short)CLIP(-32768, 32767, (74 * (tmp[i] - tmp[8 + i] + tmp[12 + i]) + rnd_factor) >> shift);
    block[4 * i + 3] = (short)CLIP(-32768, 32767, (55 * c[0] + 29 * c[2] - c[3] + rnd_factor) >> shift);
  }
}


static void partial_butterfly_4_generic(const short *src, short *dst,
  int32_t shift)
{
  int32_t j;
  int32_t e[2], o[2];
  int32_t add = 1 << (shift - 1);
  const int32_t line = 4;

  for (j = 0; j < line; j++) {
    // E and O
    e[0] = src[0] + src[3];
    o[0] = src[0] - src[3];
    e[1] = src[1] + src[2];
    o[1] = src[1] - src[2];

    dst[0] = (short)((kvz_g_dct_4[0][0] * e[0] + kvz_g_dct_4[0][1] * e[1] + add) >> shift);
    dst[2 * line] = (short)((kvz_g_dct_4[2][0] * e[0] + kvz_g_dct_4[2][1] * e[1] + add) >> shift);
    dst[line] = (short)((kvz_g_dct_4[1][0] * o[0] + kvz_g_dct_4[1][1] * o[1] + add) >> shift);
    dst[3 * line] = (short)((kvz_g_dct_4[3][0] * o[0] + kvz_g_dct_4[3][1] * o[1] + add) >> shift);

    src += 4;
    dst++;
  }
}


static void partial_butterfly_inverse_4_generic(const short *src, short *dst,
  int shift)
{
  int j;
  int e[2], o[2];
  int add = 1 << (shift - 1);
  const int32_t line = 4;

  for (j = 0; j < line; j++) {
    // Utilizing symmetry properties to the maximum to minimize the number of multiplications
    o[0] = kvz_g_dct_4[1][0] * src[line] + kvz_g_dct_4[3][0] * src[3 * line];
    o[1] = kvz_g_dct_4[1][1] * src[line] + kvz_g_dct_4[3][1] * src[3 * line];
    e[0] = kvz_g_dct_4[0][0] * src[0] + kvz_g_dct_4[2][0] * src[2 * line];
    e[1] = kvz_g_dct_4[0][1] * src[0] + kvz_g_dct_4[2][1] * src[2 * line];

    // Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector
    dst[0] = (short)CLIP(-32768, 32767, (e[0] + o[0] + add) >> shift);
    dst[1] = (short)CLIP(-32768, 32767, (e[1] + o[1] + add) >> shift);
    dst[2] = (short)CLIP(-32768, 32767, (e[1] - o[1] + add) >> shift);
    dst[3] = (short)CLIP(-32768, 32767, (e[0] - o[0] + add) >> shift);

    src++;
    dst += 4;
  }
}


static void partial_butterfly_8_generic(const short *src, short *dst,
  int32_t shift)
{
  int32_t j, k;
  int32_t e[4], o[4];
  int32_t ee[2], eo[2];
  int32_t add = 1 << (shift - 1);
  const int32_t line = 8;

  for (j = 0; j < line; j++) {
    // E and O
    for (k = 0; k < 4; k++) {
      e[k] = src[k] + src[7 - k];
      o[k] = src[k] - src[7 - k];
    }
    // EE and EO
    ee[0] = e[0] + e[3];
    eo[0] = e[0] - e[3];
    ee[1] = e[1] + e[2];
    eo[1] = e[1] - e[2];

    dst[0] = (short)((kvz_g_dct_8[0][0] * ee[0] + kvz_g_dct_8[0][1] * ee[1] + add) >> shift);
    dst[4 * line] = (short)((kvz_g_dct_8[4][0] * ee[0] + kvz_g_dct_8[4][1] * ee[1] + add) >> shift);
    dst[2 * line] = (short)((kvz_g_dct_8[2][0] * eo[0] + kvz_g_dct_8[2][1] * eo[1] + add) >> shift);
    dst[6 * line] = (short)((kvz_g_dct_8[6][0] * eo[0] + kvz_g_dct_8[6][1] * eo[1] + add) >> shift);

    dst[line] = (short)((kvz_g_dct_8[1][0] * o[0] + kvz_g_dct_8[1][1] * o[1] + kvz_g_dct_8[1][2] * o[2] + kvz_g_dct_8[1][3] * o[3] + add) >> shift);
    dst[3 * line] = (short)((kvz_g_dct_8[3][0] * o[0] + kvz_g_dct_8[3][1] * o[1] + kvz_g_dct_8[3][2] * o[2] + kvz_g_dct_8[3][3] * o[3] + add) >> shift);
    dst[5 * line] = (short)((kvz_g_dct_8[5][0] * o[0] + kvz_g_dct_8[5][1] * o[1] + kvz_g_dct_8[5][2] * o[2] + kvz_g_dct_8[5][3] * o[3] + add) >> shift);
    dst[7 * line] = (short)((kvz_g_dct_8[7][0] * o[0] + kvz_g_dct_8[7][1] * o[1] + kvz_g_dct_8[7][2] * o[2] + kvz_g_dct_8[7][3] * o[3] + add) >> shift);

    src += 8;
    dst++;
  }
}


static void partial_butterfly_inverse_8_generic(const int16_t *src, int16_t *dst,
  int32_t shift)
{
  int32_t j, k;
  int32_t e[4], o[4];
  int32_t ee[2], eo[2];
  int32_t add = 1 << (shift - 1);
  const int32_t line = 8;

  for (j = 0; j < line; j++) {
    // Utilizing symmetry properties to the maximum to minimize the number of multiplications
    for (k = 0; k < 4; k++) {
      o[k] = kvz_g_dct_8[1][k] * src[line] + kvz_g_dct_8[3][k] * src[3 * line] + kvz_g_dct_8[5][k] * src[5 * line] + kvz_g_dct_8[7][k] * src[7 * line];
    }

    eo[0] = kvz_g_dct_8[2][0] * src[2 * line] + kvz_g_dct_8[6][0] * src[6 * line];
    eo[1] = kvz_g_dct_8[2][1] * src[2 * line] + kvz_g_dct_8[6][1] * src[6 * line];
    ee[0] = kvz_g_dct_8[0][0] * src[0] + kvz_g_dct_8[4][0] * src[4 * line];
    ee[1] = kvz_g_dct_8[0][1] * src[0] + kvz_g_dct_8[4][1] * src[4 * line];

    // Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector
    e[0] = ee[0] + eo[0];
    e[3] = ee[0] - eo[0];
    e[1] = ee[1] + eo[1];
    e[2] = ee[1] - eo[1];
    for (k = 0; k < 4; k++) {
      dst[k] = (int16_t)MAX(-32768, MIN(32767, (e[k] + o[k] + add) >> shift));
      dst[k + 4] = (int16_t)MAX(-32768, MIN(32767, (e[3 - k] - o[3 - k] + add) >> shift));
    }
    src++;
    dst += 8;
  }
}


static void partial_butterfly_16_generic(const short *src, short *dst,
  int32_t shift)
{
  int32_t j, k;
  int32_t e[8], o[8];
  int32_t ee[4], eo[4];
  int32_t eee[2], eeo[2];
  int32_t add = 1 << (shift - 1);
  const int32_t line = 16;

  for (j = 0; j < line; j++) {
    // E and O
    for (k = 0; k < 8; k++) {
      e[k] = src[k] + src[15 - k];
      o[k] = src[k] - src[15 - k];
    }
    // EE and EO
    for (k = 0; k < 4; k++) {
      ee[k] = e[k] + e[7 - k];
      eo[k] = e[k] - e[7 - k];
    }
    // EEE and EEO
    eee[0] = ee[0] + ee[3];
    eeo[0] = ee[0] - ee[3];
    eee[1] = ee[1] + ee[2];
    eeo[1] = ee[1] - ee[2];

    dst[0] = (short)((kvz_g_dct_16[0][0] * eee[0] + kvz_g_dct_16[0][1] * eee[1] + add) >> shift);
    dst[8 * line] = (short)((kvz_g_dct_16[8][0] * eee[0] + kvz_g_dct_16[8][1] * eee[1] + add) >> shift);
    dst[4 * line] = (short)((kvz_g_dct_16[4][0] * eeo[0] + kvz_g_dct_16[4][1] * eeo[1] + add) >> shift);
    dst[12 * line] = (short)((kvz_g_dct_16[12][0] * eeo[0] + kvz_g_dct_16[12][1] * eeo[1] + add) >> shift);

    for (k = 2; k < 16; k += 4) {
      dst[k*line] = (short)((kvz_g_dct_16[k][0] * eo[0] + kvz_g_dct_16[k][1] * eo[1] + kvz_g_dct_16[k][2] * eo[2] + kvz_g_dct_16[k][3] * eo[3] + add) >> shift);
    }

    for (k = 1; k < 16; k += 2) {
      dst[k*line] = (short)((kvz_g_dct_16[k][0] * o[0] + kvz_g_dct_16[k][1] * o[1] + kvz_g_dct_16[k][2] * o[2] + kvz_g_dct_16[k][3] * o[3] +
        kvz_g_dct_16[k][4] * o[4] + kvz_g_dct_16[k][5] * o[5] + kvz_g_dct_16[k][6] * o[6] + kvz_g_dct_16[k][7] * o[7] + add) >> shift);
    }

    src += 16;
    dst++;
  }
}


static void partial_butterfly_inverse_16_generic(const int16_t *src, int16_t *dst,
  int32_t shift)
{
  int32_t j, k;
  int32_t e[8], o[8];
  int32_t ee[4], eo[4];
  int32_t eee[2], eeo[2];
  int32_t add = 1 << (shift - 1);
  const int32_t line = 16;

  for (j = 0; j < line; j++) {
    // Utilizing symmetry properties to the maximum to minimize the number of multiplications
    for (k = 0; k < 8; k++)  {
      o[k] = kvz_g_dct_16[1][k] * src[line] + kvz_g_dct_16[3][k] * src[3 * line] + kvz_g_dct_16[5][k] * src[5 * line] + kvz_g_dct_16[7][k] * src[7 * line] +
        kvz_g_dct_16[9][k] * src[9 * line] + kvz_g_dct_16[11][k] * src[11 * line] + kvz_g_dct_16[13][k] * src[13 * line] + kvz_g_dct_16[15][k] * src[15 * line];
    }
    for (k = 0; k < 4; k++) {
      eo[k] = kvz_g_dct_16[2][k] * src[2 * line] + kvz_g_dct_16[6][k] * src[6 * line] + kvz_g_dct_16[10][k] * src[10 * line] + kvz_g_dct_16[14][k] * src[14 * line];
    }
    eeo[0] = kvz_g_dct_16[4][0] * src[4 * line] + kvz_g_dct_16[12][0] * src[12 * line];
    eee[0] = kvz_g_dct_16[0][0] * src[0] + kvz_g_dct_16[8][0] * src[8 * line];
    eeo[1] = kvz_g_dct_16[4][1] * src[4 * line] + kvz_g_dct_16[12][1] * src[12 * line];
    eee[1] = kvz_g_dct_16[0][1] * src[0] + kvz_g_dct_16[8][1] * src[8 * line];

    // Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector
    for (k = 0; k < 2; k++) {
      ee[k] = eee[k] + eeo[k];
      ee[k + 2] = eee[1 - k] - eeo[1 - k];
    }
    for (k = 0; k < 4; k++) {
      e[k] = ee[k] + eo[k];
      e[k + 4] = ee[3 - k] - eo[3 - k];
    }
    for (k = 0; k < 8; k++) {
      dst[k] = (short)MAX(-32768, MIN(32767, (e[k] + o[k] + add) >> shift));
      dst[k + 8] = (short)MAX(-32768, MIN(32767, (e[7 - k] - o[7 - k] + add) >> shift));
    }
    src++;
    dst += 16;
  }
}


static void partial_butterfly_32_generic(const short *src, short *dst,
  int32_t shift)
{
  int32_t j, k;
  int32_t e[16], o[16];
  int32_t ee[8], eo[8];
  int32_t eee[4], eeo[4];
  int32_t eeee[2], eeeo[2];
  int32_t add = 1 << (shift - 1);
  const int32_t line = 32;

  for (j = 0; j < line; j++) {
    // E and O
    for (k = 0; k < 16; k++) {
      e[k] = src[k] + src[31 - k];
      o[k] = src[k] - src[31 - k];
    }
    // EE and EO
    for (k = 0; k < 8; k++) {
      ee[k] = e[k] + e[15 - k];
      eo[k] = e[k] - e[15 - k];
    }
    // EEE and EEO
    for (k = 0; k < 4; k++) {
      eee[k] = ee[k] + ee[7 - k];
      eeo[k] = ee[k] - ee[7 - k];
    }
    // EEEE and EEEO
    eeee[0] = eee[0] + eee[3];
    eeeo[0] = eee[0] - eee[3];
    eeee[1] = eee[1] + eee[2];
    eeeo[1] = eee[1] - eee[2];

    dst[0] = (short)((kvz_g_dct_32[0][0] * eeee[0] + kvz_g_dct_32[0][1] * eeee[1] + add) >> shift);
    dst[16 * line] = (short)((kvz_g_dct_32[16][0] * eeee[0] + kvz_g_dct_32[16][1] * eeee[1] + add) >> shift);
    dst[8 * line] = (short)((kvz_g_dct_32[8][0] * eeeo[0] + kvz_g_dct_32[8][1] * eeeo[1] + add) >> shift);
    dst[24 * line] = (short)((kvz_g_dct_32[24][0] * eeeo[0] + kvz_g_dct_32[24][1] * eeeo[1] + add) >> shift);
    for (k = 4; k < 32; k += 8) {
      dst[k*line] = (short)((kvz_g_dct_32[k][0] * eeo[0] + kvz_g_dct_32[k][1] * eeo[1] + kvz_g_dct_32[k][2] * eeo[2] + kvz_g_dct_32[k][3] * eeo[3] + add) >> shift);
    }
    for (k = 2; k < 32; k += 4) {
      dst[k*line] = (short)((kvz_g_dct_32[k][0] * eo[0] + kvz_g_dct_32[k][1] * eo[1] + kvz_g_dct_32[k][2] * eo[2] + kvz_g_dct_32[k][3] * eo[3] +
        kvz_g_dct_32[k][4] * eo[4] + kvz_g_dct_32[k][5] * eo[5] + kvz_g_dct_32[k][6] * eo[6] + kvz_g_dct_32[k][7] * eo[7] + add) >> shift);
    }
    for (k = 1; k < 32; k += 2) {
      dst[k*line] = (short)((kvz_g_dct_32[k][0] * o[0] + kvz_g_dct_32[k][1] * o[1] + kvz_g_dct_32[k][2] * o[2] + kvz_g_dct_32[k][3] * o[3] +
        kvz_g_dct_32[k][4] * o[4] + kvz_g_dct_32[k][5] * o[5] + kvz_g_dct_32[k][6] * o[6] + kvz_g_dct_32[k][7] * o[7] +
        kvz_g_dct_32[k][8] * o[8] + kvz_g_dct_32[k][9] * o[9] + kvz_g_dct_32[k][10] * o[10] + kvz_g_dct_32[k][11] * o[11] +
        kvz_g_dct_32[k][12] * o[12] + kvz_g_dct_32[k][13] * o[13] + kvz_g_dct_32[k][14] * o[14] + kvz_g_dct_32[k][15] * o[15] + add) >> shift);
    }
    src += 32;
    dst++;
  }
}


static void partial_butterfly_inverse_32_generic(const int16_t *src, int16_t *dst,
  int32_t shift)
{
  int32_t j, k;
  int32_t e[16], o[16];
  int32_t ee[8], eo[8];
  int32_t eee[4], eeo[4];
  int32_t eeee[2], eeeo[2];
  int32_t add = 1 << (shift - 1);
  const int32_t line = 32;

  for (j = 0; j<line; j++) {
    // Utilizing symmetry properties to the maximum to minimize the number of multiplications
    for (k = 0; k < 16; k++) {
      o[k] = kvz_g_dct_32[1][k] * src[line] + kvz_g_dct_32[3][k] * src[3 * line] + kvz_g_dct_32[5][k] * src[5 * line] + kvz_g_dct_32[7][k] * src[7 * line] +
        kvz_g_dct_32[9][k] * src[9 * line] + kvz_g_dct_32[11][k] * src[11 * line] + kvz_g_dct_32[13][k] * src[13 * line] + kvz_g_dct_32[15][k] * src[15 * line] +
        kvz_g_dct_32[17][k] * src[17 * line] + kvz_g_dct_32[19][k] * src[19 * line] + kvz_g_dct_32[21][k] * src[21 * line] + kvz_g_dct_32[23][k] * src[23 * line] +
        kvz_g_dct_32[25][k] * src[25 * line] + kvz_g_dct_32[27][k] * src[27 * line] + kvz_g_dct_32[29][k] * src[29 * line] + kvz_g_dct_32[31][k] * src[31 * line];
    }
    for (k = 0; k < 8; k++) {
      eo[k] = kvz_g_dct_32[2][k] * src[2 * line] + kvz_g_dct_32[6][k] * src[6 * line] + kvz_g_dct_32[10][k] * src[10 * line] + kvz_g_dct_32[14][k] * src[14 * line] +
        kvz_g_dct_32[18][k] * src[18 * line] + kvz_g_dct_32[22][k] * src[22 * line] + kvz_g_dct_32[26][k] * src[26 * line] + kvz_g_dct_32[30][k] * src[30 * line];
    }
    for (k = 0; k < 4; k++) {
      eeo[k] = kvz_g_dct_32[4][k] * src[4 * line] + kvz_g_dct_32[12][k] * src[12 * line] + kvz_g_dct_32[20][k] * src[20 * line] + kvz_g_dct_32[28][k] * src[28 * line];
    }
    eeeo[0] = kvz_g_dct_32[8][0] * src[8 * line] + kvz_g_dct_32[24][0] * src[24 * line];
    eeeo[1] = kvz_g_dct_32[8][1] * src[8 * line] + kvz_g_dct_32[24][1] * src[24 * line];
    eeee[0] = kvz_g_dct_32[0][0] * src[0] + kvz_g_dct_32[16][0] * src[16 * line];
    eeee[1] = kvz_g_dct_32[0][1] * src[0] + kvz_g_dct_32[16][1] * src[16 * line];

    // Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector
    eee[0] = eeee[0] + eeeo[0];
    eee[3] = eeee[0] - eeeo[0];
    eee[1] = eeee[1] + eeeo[1];
    eee[2] = eeee[1] - eeeo[1];
    for (k = 0; k < 4; k++) {
      ee[k] = eee[k] + eeo[k];
      ee[k + 4] = eee[3 - k] - eeo[3 - k];
    }
    for (k = 0; k < 8; k++) {
      e[k] = ee[k] + eo[k];
      e[k + 8] = ee[7 - k] - eo[7 - k];
    }
    for (k = 0; k<16; k++) {
      dst[k] = (short)MAX(-32768, MIN(32767, (e[k] + o[k] + add) >> shift));
      dst[k + 16] = (short)MAX(-32768, MIN(32767, (e[15 - k] - o[15 - k] + add) >> shift));
    }
    src++;
    dst += 32;
  }
}

#define DCT_NXN_GENERIC(n) \
static void dct_ ## n ## x ## n ## _generic(int8_t bitdepth, const int16_t *input, int16_t *output) { \
\
  int16_t tmp[ n * n ]; \
  int32_t shift_1st = kvz_g_convert_to_bit[ n ] + 1 + (bitdepth - 8); \
  int32_t shift_2nd = kvz_g_convert_to_bit[ n ] + 8; \
\
  partial_butterfly_ ## n ## _generic(input, tmp, shift_1st); \
  partial_butterfly_ ## n ## _generic(tmp, output, shift_2nd); \
}

#define IDCT_NXN_GENERIC(n) \
static void idct_ ## n ## x ## n ## _generic(int8_t bitdepth, const int16_t *input, int16_t *output) { \
\
  int16_t tmp[ n * n ]; \
  int32_t shift_1st = 7; \
  int32_t shift_2nd = 12 - (bitdepth - 8); \
\
  partial_butterfly_inverse_ ## n ## _generic(input, tmp, shift_1st); \
  partial_butterfly_inverse_ ## n ## _generic(tmp, output, shift_2nd); \
}

DCT_NXN_GENERIC(4);
DCT_NXN_GENERIC(8);
DCT_NXN_GENERIC(16);
DCT_NXN_GENERIC(32);

IDCT_NXN_GENERIC(4);
IDCT_NXN_GENERIC(8);
IDCT_NXN_GENERIC(16);
IDCT_NXN_GENERIC(32);

static void fast_forward_dst_4x4_generic(int8_t bitdepth, const int16_t *input, int16_t *output)
{
  int16_t tmp[4*4]; 
  int32_t shift_1st = kvz_g_convert_to_bit[4] + 1 + (bitdepth - 8);
  int32_t shift_2nd = kvz_g_convert_to_bit[4] + 8;

  fast_forward_dst_4_generic(input, tmp, shift_1st); 
  fast_forward_dst_4_generic(tmp, output, shift_2nd);
}

static void fast_inverse_dst_4x4_generic(int8_t bitdepth, const int16_t *input, int16_t *output)
{
  int16_t tmp[4 * 4];
  int32_t shift_1st = 7;
  int32_t shift_2nd = 12 - (bitdepth - 8);

  fast_inverse_dst_4_generic(input, tmp, shift_1st);
  fast_inverse_dst_4_generic(tmp, output, shift_2nd);
}

int kvz_strategy_register_dct_generic(void* opaque, uint8_t bitdepth)
{
  bool success = true;

  success &= kvz_strategyselector_register(opaque, "fast_forward_dst_4x4", "generic", 0, &fast_forward_dst_4x4_generic);
  
  success &= kvz_strategyselector_register(opaque, "dct_4x4", "generic", 0, &dct_4x4_generic);
  success &= kvz_strategyselector_register(opaque, "dct_8x8", "generic", 0, &dct_8x8_generic);
  success &= kvz_strategyselector_register(opaque, "dct_16x16", "generic", 0, &dct_16x16_generic);
  success &= kvz_strategyselector_register(opaque, "dct_32x32", "generic", 0, &dct_32x32_generic);

  success &= kvz_strategyselector_register(opaque, "fast_inverse_dst_4x4", "generic", 0, &fast_inverse_dst_4x4_generic);

  success &= kvz_strategyselector_register(opaque, "idct_4x4", "generic", 0, &idct_4x4_generic);
  success &= kvz_strategyselector_register(opaque, "idct_8x8", "generic", 0, &idct_8x8_generic);
  success &= kvz_strategyselector_register(opaque, "idct_16x16", "generic", 0, &idct_16x16_generic);
  success &= kvz_strategyselector_register(opaque, "idct_32x32", "generic", 0, &idct_32x32_generic);
  return success;
}
